// DirectSound (DirectX 5 interfaces) for the recompiled Handkerchief: SDL software mixer, 48 kHz stereo.
// Derived from sh3vita port/src/dx/dsound.c (GPL-3.0): the mixer, Lock/Play/position logic and buffer duplication
// are kept; 3D sound, effects and movie audio (Silent Hill 3 only) are removed.
//
// COM objects live in guest memory as {vtable, tag} like port/hk/ddraw.c. Buffer samples stay in guest memory,
// so the game writes them through Lock() and the SDL audio thread reads them directly.
// Also PlaySoundA (WINMM): a WAV file or memory image played once through the same mixer.
#include "hitch.h"
#include "win32.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __vita__
#include <psp2/kernel/threadmgr.h>
#endif

#define OUT_RATE 48000
#define MIX_FRAMES 2048u   // ~43 ms at 48 kHz: headroom when many voices play at once
#define DS_OK 0u
#define DSERR_INVALIDPARAM 0x80070057u
#define DSERR_INVALIDCALL 0x88780032u
#define E_NOINTERFACE_ 0x80004002u
#define E_OUTOFMEMORY_ 0x8007000Eu

extern unsigned g_frame;   // presented frames (port/hk/glue.c)
enum { K_DS = 1, K_BUF, K_NOTIFY };
typedef struct DSBuf {
    int kind, refs;
    uint32_t guest;             // guest COM object
    uint32_t mem, size, flags;
    uint32_t ch, rate, bits, align;
    int primary, playing, looping, oneshot;   // oneshot: PlaySoundA buffer, freed after it ends
    double pos;                 // play position in frames
    uint32_t freq;
    int32_t volume, pan;
    int nnotify;
    struct { uint32_t off, ev; } notify[32];
    uint32_t gen;               // changed by Play/Stop/SetCurrentPosition: the mixer drops stale results
    float ramp;                 // declick gain 0..1, rises over RAMP_FR after a (re)start
    float lp[4];                // anti-imaging low-pass state (two one-pole stages, left and right)
    int *shared_refs;           // sample memory shared between duplicates
    struct DSBuf *parent;       // K_NOTIFY: the buffer it belongs to
} DSBuf;

#define MAX_OBJ 512
static DSBuf *g_obj[MAX_OBJ];
static SDL_mutex *mx;
static SDL_AudioDeviceID audio_dev;
static float mix_acc[MIX_FRAMES * 2];
static uint32_t g_vt_ds, g_vt_buf, g_vt_notify;

static uint32_t g_gen;   // under mx
float g_hk_volume;   // master gain chosen by the player (0 = default)
// Music gain (multiplies the master for music buffers). The game's music buffers are the Y_*.wav tracks, several MB
// long (effects are well under 1 MB), so a buffer above MUSIC_BYTES is music. 1.5 keeps the music clearly above
// effects in busy scenes; the Vita pause screen sets it (Up/Down, 1..10 -> 0.25..2.5).
float g_hk_music_volume = 1.5f;
#define MUSIC_BYTES (1u << 20)

static int now_ok(Uint64 *t_last) {   // true at most once every 5 s
    Uint64 now = SDL_GetPerformanceCounter();
    if (now - *t_last < 5 * SDL_GetPerformanceFrequency()) return 0;
    *t_last = now;
    return 1;
}
static float db_gain(int32_t v) { return v <= -10000 ? 0.f : powf(10.f, (float)v / 2000.f); }

static uint32_t obj_wrap(DSBuf *b, int kind, uint32_t vt) {
    HOSTFN;
    for (int i = 1; i < MAX_OBJ; i++)
        if (!g_obj[i]) {
            g_obj[i] = b;
            b->kind = kind;
            b->refs = 1;
            b->guest = gheap_calloc(16);
            W32(b->guest, vt);
            W32(b->guest + 4, 0x44530000u | (uint32_t)i);   // 'DS' tag + index
            return b->guest;
        }
    rt_fatal("DirectSound: too many objects");
}
static DSBuf *obj_of(uint32_t guest, int kind) {
    HOSTFN;
    if (!guest) return NULL;
    uint32_t tag = R32(guest + 4);
    int i = (int)(tag & 0xffffu);
    if ((tag >> 16) != 0x4453u || i <= 0 || i >= MAX_OBJ || !g_obj[i] || g_obj[i]->guest != guest) return NULL;
    return kind && g_obj[i]->kind != kind ? NULL : g_obj[i];
}
// The 16-byte guest object is kept (tag cleared) instead of freed: HDKWIN.EXE calls Stop (vtable +0x48, from
// 0x45C921) on a buffer it has already released; with the block reused by the game heap that call jumped to
// garbage. A dead object keeps its vtable, and every method answers DSERR_INVALIDPARAM.
static unsigned g_ds_dead;   // guest 16-byte objects kept dead (never freed, see above): memory log
static void obj_unwrap(DSBuf *b) {
    HOSTFN;
    g_ds_dead++;
    uint32_t i = R32(b->guest + 4) & 0xffffu;
    if (i < MAX_OBJ && g_obj[i] == b) g_obj[i] = NULL;
    W32(b->guest + 4, 0x44530000u);   // dead: index 0
}

static int n_ghost;
// Live DirectSound state for the memory log (HK_MEMLOG). Sample bytes count each shared sample once.
typedef struct { int buffers, notifies, playing, dups, ghosts, top_slot; uint64_t sample_bytes; unsigned dead_guest_objs; } HkDsStats;
void hk_ds_stats(HkDsStats *o) {
    memset(o, 0, sizeof *o);
    if (!mx) return;
    SDL_LockMutex(mx);
    for (int k = 1; k < MAX_OBJ; k++) {
        DSBuf *b = g_obj[k];
        if (!b) continue;
        o->top_slot = k;
        if (b->kind == K_NOTIFY) { o->notifies++; continue; }
        if (b->kind != K_BUF) continue;
        o->buffers++;
        o->playing += b->playing != 0;
        int first = 1;   // the first buffer of a sample owns its bytes; later ones are DuplicateSoundBuffer copies
        for (int j = 1; j < k; j++) if (g_obj[j] && g_obj[j]->kind == K_BUF && g_obj[j]->mem == b->mem) { first = 0; break; }
        o->dups += !first;
        if (first && b->mem) o->sample_bytes += b->size;
    }
    o->ghosts = n_ghost;
    o->dead_guest_objs = g_ds_dead;
    SDL_UnlockMutex(mx);
}

// Callback timing: a callback that comes much later than one buffer after the previous one means the device ran
// dry (silence or crackle). Late callbacks are counted and logged at most every 10 s with the game frame, so
// console logs show whether the audio thread is starved. On the Vita the audio thread is moved to core 1, away
// from the game's main thread (core 0, port/vita/main.c), which can use 100% of its core in heavy scenes.
#if !HK_DEVTOOLS
static void late_check(int n) {   // player build: only moves the audio thread to core 1 (the Vita) on the first callback
    (void)n;
#ifdef __vita__
    static int moved;
    if (!moved) { moved = 1; sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_1); }
#endif
}
#else
static void late_check(int n) {
    static Uint64 last, t_log; static unsigned late, calls; static double worst;
    Uint64 now = SDL_GetPerformanceCounter(), hz = SDL_GetPerformanceFrequency();
    if (!last) {
#ifdef __vita__
        int r = sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_1);
        rt_dlog("[mix] audio thread on core 1 (%d)\n", r);
#endif
        last = t_log = now;
        return;
    }
    double gap = (double)(now - last) * 1000.0 / (double)hz, period = n * 1000.0 / OUT_RATE;
    last = now;
    calls++;
    if (gap > period * 1.5) { late++; if (gap > worst) worst = gap; }
    if (late && now - t_log > 10 * hz) {
        extern unsigned g_frame;
        rt_dlog("[mix] %u of %u callbacks late (worst gap %.0f ms, period %.0f ms) frame %u\n", late, calls, worst, period, g_frame);
        late = calls = 0; worst = 0; t_log = now;
    }
}
#endif

static void free_buffer(DSBuf *b);
#define RAMP_FR 144            // 3 ms at 48 kHz: fade-in after a start/restart and fade-out tail after Stop/restart
#define MAX_VOICES MAX_OBJ
struct Voice {
    DSBuf *b; uint32_t gen, mem, frames; int stereo, bits16, looping, ghost;
    uint64_t pos, st; float gl, gr, a, lp[4], vpk, ramp, dr; double before; int ended;
};
static struct Voice ghosts[64];   // under mx: stopped/restarted voices still fading out, consumed by the next mix()
static int n_ghost;
// Under mx: events counted for the 5 s "[mix] 5 s" console line (buf_play / ghost_add).
static unsigned st_starts, st_ghost_drop;
static void fill_voice(struct Voice *v, DSBuf *b) {
    float g = db_gain(b->volume);
    if (b->size > MUSIC_BYTES) g *= g_hk_music_volume;
    v->gl = v->gr = g;
    if (b->pan < 0) v->gr *= db_gain(b->pan); else if (b->pan > 0) v->gl *= db_gain(-b->pan);
    double step = (double)(b->freq ? b->freq : b->rate) / OUT_RATE;
    // Linear interpolation of 11/22 kHz samples up to 48 kHz leaves images above the source's Nyquist
    // frequency, heard as hiss. Two one-pole low-pass stages at 0.45 x the voice's playback rate remove most
    // of them (a sound card's reconstruction filter does this on Windows).
    float rate_hz = (float)(b->freq ? b->freq : b->rate);
    v->a = rate_hz < 40000.f ? 1.f - expf(-6.2831853f * 0.45f * rate_hz / OUT_RATE) : 1.f;
    v->b = b; v->gen = b->gen; v->mem = b->mem; v->frames = b->size / b->align;
    v->stereo = b->ch > 1; v->bits16 = b->bits == 16; v->looping = b->looping;
    v->before = b->pos;
    v->pos = (uint64_t)(b->pos * 65536.0); v->st = (uint64_t)(step * 65536.0);
    memcpy(v->lp, b->lp, sizeof v->lp);
    v->ended = 0; v->ghost = 0;
}
// Under mx. The voice leaves the game's view now (Stop, SetCurrentPosition), but its last 3 ms keep sounding with a
// linear fade-out so the cut does not click. Buffers that are not playing leave no tail.
static void ghost_add(DSBuf *b) {
    if (!b->playing || b->primary || !b->align) return;
    if (n_ghost >= 64) { st_ghost_drop++; return; }
    struct Voice *v = &ghosts[n_ghost];
    fill_voice(v, b);
    v->b = NULL; v->ghost = 1; v->ramp = b->ramp; v->dr = -1.f / (float)RAMP_FR;
    n_ghost++;
}
#if HK_DEVTOOLS
// ---------- Output recorder (hkport.cfg rec=1 on the Vita, HK_REC=1 or HK_REC=<file> on PC) ----------
// The audio callback only copies each output block into a ring (REC_RING frames, 4 s); a low-priority thread (core 2
// on the Vita) writes it every 200 ms to g_hk_rec_path as a 48 kHz 16-bit stereo WAV whose header is kept valid after
// every write, so the file is usable even if the game is closed abruptly. After REC_FILE_S seconds the file becomes
// <name>.prev.wav and a new one starts: the last 60-120 s are always on disk.
const char *g_hk_rec_path;   // set by the platform (port/vita/main.c) or from HK_REC in audio_start
#define REC_RING (OUT_RATE * 4)
#define REC_FILE_S 60
static int16_t *rec_ring;
static SDL_atomic_t rec_w, rec_r, rec_drop;   // frames written by the callback / taken by the writer / dropped
static void hk_rec_push(const int16_t *s, int n) {
    if (!rec_ring) return;
    unsigned w = (unsigned)SDL_AtomicGet(&rec_w), r = (unsigned)SDL_AtomicGet(&rec_r);
    if (w - r + (unsigned)n > REC_RING) { SDL_AtomicAdd(&rec_drop, n); return; }   // writer behind: drop this block
    for (int i = 0; i < n; i++) {
        unsigned k = (w + (unsigned)i) % REC_RING;
        rec_ring[2 * k] = s[2 * i]; rec_ring[2 * k + 1] = s[2 * i + 1];
    }
    SDL_AtomicSet(&rec_w, (int)(w + (unsigned)n));
}
static void rec_header(FILE *f, uint32_t frames) {
    uint32_t data = frames * 4, riff = 36 + data, fmt_sz = 16, rate = OUT_RATE, bps = OUT_RATE * 4;
    uint16_t pcm = 1, ch = 2, al = 4, bits = 16;
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmt_sz, 4, 1, f);
    fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&bps, 4, 1, f);
    fwrite(&al, 2, 1, f); fwrite(&bits, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&data, 4, 1, f);
}
static int rec_thread(void *arg) {
    (void)arg;
#ifdef __vita__
    sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_2);
#endif
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
    char prev[600];
    snprintf(prev, sizeof prev, "%.*s.prev.wav", (int)(strlen(g_hk_rec_path) > 4 ? strlen(g_hk_rec_path) - 4 : strlen(g_hk_rec_path)), g_hk_rec_path);
    FILE *f = NULL; uint32_t frames = 0; unsigned drops_logged = 0;
    for (;;) {
        SDL_Delay(200);
        if (!f) {
            f = fopen(g_hk_rec_path, "w+b");
            if (!f) { rt_dlog("[rec] cannot create %s\n", g_hk_rec_path); return 0; }
            frames = 0; rec_header(f, 0);
        }
        unsigned w = (unsigned)SDL_AtomicGet(&rec_w), r = (unsigned)SDL_AtomicGet(&rec_r);
        if (w == r) continue;
        const uint64_t tw0 = SDL_GetPerformanceCounter();
        fseek(f, 44 + (long)frames * 4, SEEK_SET);
        while (r != w) {
            unsigned k = r % REC_RING, run = REC_RING - k;
            if (run > w - r) run = w - r;
            fwrite(rec_ring + 2 * k, 4, run, f);
            r += run; frames += run;
        }
        SDL_AtomicSet(&rec_r, (int)r);
        rec_header(f, frames);
        fflush(f);
        { extern uint64_t g_hk_rec_t; uint64_t d2 = SDL_GetPerformanceCounter() - tw0; if (d2 > g_hk_rec_t) g_hk_rec_t = d2; }   // hitch log: longest write (a ux0 flush can block other threads' I/O)
        unsigned d = (unsigned)SDL_AtomicGet(&rec_drop);
        if (d != drops_logged) { rt_dlog("[rec] %u frames dropped (writer too slow)\n", d - drops_logged); drops_logged = d; }
        if (frames >= (uint32_t)OUT_RATE * REC_FILE_S) {
            fclose(f); f = NULL;
            remove(prev);
            if (rename(g_hk_rec_path, prev)) rt_dlog("[rec] rename to %s failed\n", prev);
        }
    }
}
static void rec_start(void) {
    if (!g_hk_rec_path || rec_ring) return;
    int16_t *ring = calloc((size_t)REC_RING * 2, sizeof(int16_t));
    if (!ring) return;
    rec_ring = ring;
    SDL_Thread *t = SDL_CreateThread(rec_thread, "hkrec", NULL);
    if (!t) { rt_dlog("[rec] no thread\n"); rec_ring = NULL; free(ring); return; }
    SDL_DetachThread(t);
    rt_dlog("[rec] recording the output to %s (previous %d s in .prev.wav)\n", g_hk_rec_path, REC_FILE_S);
}

#else
static void hk_rec_push(const int16_t *s, int n) { (void)s; (void)n; }
static void rec_start(void) {}
#endif

static void mix(void *ud, Uint8 *stream, int len) {
    (void)ud;
    int n = len / 4;
    if (n <= 0) return;
    if ((unsigned)n > MIX_FRAMES) n = MIX_FRAMES;   // the device is opened with MIX_FRAMES samples
    late_check(n);
    Uint64 t_mix0 = SDL_GetPerformanceCounter();
    float *acc = mix_acc;
    memset(acc, 0, (size_t)n * 2 * sizeof(float));
    // The mixing itself runs without the lock: the game's DirectSound calls (Play, SetVolume, GetCurrentPosition,
    // Lock ...) take mx many times per frame, and holding it for the whole mix stalled the game's main thread on
    // the console (25 -> 18 fps once the low-pass filter made mixing slower). Voices are copied under the lock,
    // mixed, and their new state is written back under the lock only if nothing changed them meanwhile (gen).
    static struct Voice vs[MAX_VOICES];
    int nv = 0;
    SDL_LockMutex(mx);
    for (int k = 1; k < MAX_OBJ; k++) {
        DSBuf *b = g_obj[k];
        if (!b || b->kind != K_BUF || !b->playing || b->primary || !b->align) continue;
        if (nv >= MAX_VOICES) break;
        fill_voice(&vs[nv++], b);
        vs[nv - 1].ramp = b->ramp; vs[nv - 1].dr = b->ramp < 1.f ? 1.f / RAMP_FR : 0.f;
    }
    for (int g = 0; g < n_ghost && nv < MAX_VOICES; g++) vs[nv++] = ghosts[g];   // fade-out tails
    n_ghost = 0;
    SDL_UnlockMutex(mx);
    const float k16 = 1.f / 32768.f, k8 = 1.f / 128.f;
    for (int j = 0; j < nv; j++) {
        struct Voice *v = &vs[j];
        // Fixed-point 32.16 position and direct sample access (one format per voice).
        uint64_t pos = v->pos, st = v->st, end = (uint64_t)v->frames << 16;
        const uint8_t *base = g_mem + v->mem;
        uint32_t frames = v->frames;
        float a = v->a, gl = v->gl, gr = v->gr, z0 = v->lp[0], z1 = v->lp[1], z2 = v->lp[2], z3 = v->lp[3];
        int silent = gl == 0.f && gr == 0.f;
        float ramp = v->ramp, dr = v->dr;
        float vpk = 0.f;
        for (int i = 0; i < n; i++) {
            if (!silent) {
                uint32_t f = (uint32_t)(pos >> 16);
                float t = (float)(pos & 0xffffu) * (1.f / 65536.f);
                uint32_t f2 = f + 1 < frames ? f + 1 : (v->looping ? 0 : f);
                float l, r;
                if (v->bits16) {
                    const int16_t *s16 = (const int16_t *)base;
                    if (v->stereo) {
                        float x0 = s16[2 * f], x1 = s16[2 * f2], y0 = s16[2 * f + 1], y1 = s16[2 * f2 + 1];
                        l = (x0 + (x1 - x0) * t) * k16; r = (y0 + (y1 - y0) * t) * k16;
                    } else {
                        float x0 = s16[f], x1 = s16[f2];
                        l = r = (x0 + (x1 - x0) * t) * k16;
                    }
                } else {
                    if (v->stereo) {
                        float x0 = base[2 * f] - 128, x1 = base[2 * f2] - 128, y0 = base[2 * f + 1] - 128, y1 = base[2 * f2 + 1] - 128;
                        l = (x0 + (x1 - x0) * t) * k8; r = (y0 + (y1 - y0) * t) * k8;
                    } else {
                        float x0 = base[f] - 128, x1 = base[f2] - 128;
                        l = r = (x0 + (x1 - x0) * t) * k8;
                    }
                }
                z0 += a * (l - z0); z1 += a * (z0 - z1); z2 += a * (r - z2); z3 += a * (z2 - z3);
                float rm = ramp < 0.f ? 0.f : ramp;
                acc[2 * i] += z1 * gl * rm;
                acc[2 * i + 1] += z3 * gr * rm;
                float m = fabsf(z1 * gl * rm);
                if (m > vpk) vpk = m;
            }
            ramp += dr;
            if (dr > 0.f && ramp >= 1.f) { ramp = 1.f; dr = 0.f; }
            if (dr < 0.f && ramp <= 0.f) { ramp = 0.f; v->ended = 1; break; }   // tail finished
            pos += st;
            if (pos >= end) {
                if (v->looping) pos -= end;
                else { pos = 0; v->ended = 1; break; }
            }
        }
        v->pos = pos;
        v->ramp = ramp;
        v->vpk = vpk;
        v->lp[0] = z0; v->lp[1] = z1; v->lp[2] = z2; v->lp[3] = z3;
    }
    Uint64 t_mix1 = SDL_GetPerformanceCounter();   // voices mixed: [t_mix0, t_mix1] is the per-voice work
    SDL_LockMutex(mx);
    for (int j = 0; j < nv; j++) {
        struct Voice *v = &vs[j];
        DSBuf *b = v->b;
        if (v->ghost) {   // a fade-out tail longer than this block goes on in the next one
            if (!v->ended && v->ramp > 0.f && n_ghost < 64) ghosts[n_ghost++] = *v;
            continue;
        }
        int alive = 0;
        for (int k = 1; k < MAX_OBJ && !alive; k++) alive = g_obj[k] == b;
        if (!alive || b->gen != v->gen || !b->playing) continue;   // released, restarted, stopped or moved
        b->pos = (double)v->pos / 65536.0;
        b->ramp = v->ramp;
        memcpy(b->lp, v->lp, sizeof b->lp);
        if (v->ended) b->playing = 0;
        double before = v->before;
        for (int q = 0; q < b->nnotify; q++) {   // position notifications crossed during this block
            uint32_t off = b->notify[q].off;
            if (off == 0xffffffffu) { if (!b->playing) w32_event_set(b->notify[q].ev, 1); continue; }
            double fo = (double)off / b->align;
            int crossed = b->pos >= before ? (fo >= before && fo < b->pos) : (fo >= before || fo < b->pos);
            if (crossed) w32_event_set(b->notify[q].ev, 1);
        }
        if (b->oneshot && !b->playing) free_buffer(b);
    }
    {   // HK_TRACE_DS: most voices mixed at once and mixer time, every 5 s
        static int maxv; static Uint64 tsum, t0; int v = 0;
        for (int k = 1; k < MAX_OBJ; k++) if (g_obj[k] && g_obj[k]->kind == K_BUF && g_obj[k]->playing) v++;
        if (v > maxv) maxv = v;
        Uint64 now = SDL_GetPerformanceCounter();
        if (!t0) t0 = now;
        if (now - t0 > 5 * SDL_GetPerformanceFrequency() && HK_ENV("HK_TRACE_DS")) {
            rt_dlog("[mix] max voices %d\n", maxv); maxv = 0; t0 = now;
            for (int k = 1; k < MAX_OBJ; k++) { DSBuf *q = g_obj[k]; if (q && q->kind == K_BUF && q->playing)
                rt_dlog("[mix]  #%d vol %d pan %d freq %u rate %u pos %.0f/%u loop %d\n", k, q->volume, q->pan, q->freq, q->rate, q->pos, q->align ? q->size / q->align : 0, q->looping); }
        }
        (void)tsum;
        static uint64_t outn; extern unsigned g_frame;   // HK_TRACE_DS: audio time -> game frame, every second
        if (outn / OUT_RATE != (outn + (unsigned)n) / OUT_RATE && HK_ENV("HK_TRACE_DS")) rt_dlog("[mixt] %.1f s frame %u\n", (double)(outn + n) / OUT_RATE, g_frame);
        outn += (unsigned)n;
    }
    SDL_UnlockMutex(mx);
    // Output stage. DirectSound mixes every playing voice at the gain the game set (DSBVOLUME, 0 = unity) and adds
    // them; Windows saturates the 16-bit sum. Here the sum is scaled by a fixed master gain (the player's volume:
    // g_hk_volume from the Vita pause screen, 1..10 -> 0.06..0.6; on PC HK_MASTER or 0.22), so a scene keeps the
    // balance and loudness relations of the original. There is no ducking or pumping: a sample is changed only
    // above LIM_T (0.9 full scale), where a tanh curve takes the place of Windows' hard clip and reaches full scale
    // only asymptotically. Below LIM_T the output is the plain sum (bit-exact apart from rounding).
    extern float g_hk_volume;
    static float env_master; if (env_master == 0.f) { const char *e = HK_ENV("HK_MASTER"); env_master = e ? (float)atof(e) : 0.22f; }
    float master = g_hk_volume > 0.f ? g_hk_volume : env_master;
    int16_t *out = (int16_t *)stream;
    unsigned over = 0;
    float pk = 0.f;   // peak of the scaled sum before the limiter
#define LIM_T 0.9f
    for (int i = 0; i < n * 2; i++) {
        float x = acc[i] * master, m = fabsf(x);
        if (m > pk) pk = m;
        if (m > LIM_T) { x = copysignf(LIM_T + (1.f - LIM_T) * tanhf((m - LIM_T) / (1.f - LIM_T)), x); over++; }
        float v = x * 32767.f;
        out[i] = (int16_t)(v > 32767.f ? 32767 : v < -32768.f ? -32768 : lrintf(v));
    }
    hk_rec_push(out, n);   // rec=1 / HK_REC: copy of the output for the recorder thread (no I/O here)
#if HK_DEVTOOLS
    // Blocks that reached the limiter: the loudest one of each 5 s window is kept (peak of the scaled sum and its
    // voices louder than 0.05: sample address/length in frames, gain, own peak) and logged with the 5 s line, so a
    // console log names the sounds that pile up. HK_TRACE_DS=2 logs every such block.
    static char lim_worst[400]; static float lim_worst_pk;
    if (over) {
        static int t2 = -1; if (t2 < 0) { const char *e = HK_ENV("HK_TRACE_DS"); t2 = e && atoi(e) >= 2; }
        if (t2 || pk > lim_worst_pk) {
            extern unsigned g_frame;
            char line[400]; int len = snprintf(line, sizeof line, "f%u pk %.2f nv %d:", g_frame, pk, nv);
            for (int j = 0; j < nv && len < (int)sizeof line - 40; j++) if (vs[j].vpk * master > 0.05f)
                len += snprintf(line + len, sizeof line - len, " [%08x/%u g %.2f pk %.2f%s]", vs[j].mem, vs[j].frames,
                                vs[j].gl, vs[j].vpk * master, vs[j].ghost ? " gh" : "");
            if (t2) rt_dlog("[lim] %s\n", line);
            if (pk > lim_worst_pk) { lim_worst_pk = pk; memcpy(lim_worst, line, sizeof lim_worst); }
        }
    }
    {   // Console evidence, at most one line every 5 s: blocks that reached the limiter, blocks that came out (nearly)
        // silent although a voice louder than -26 dB was playing, mixed voices, peak of the scaled sum, mix time per
        // block (voice loop only, and the whole callback) and the number of starts (Play of a stopped buffer).
        static Uint64 t_last; static unsigned blocks, quiet, samples_over;
        if (over) blocks++;
        samples_over += over;
        int audible = 0;
        for (int j = 0; j < nv; j++) if (vs[j].vpk > 0.05f) audible++;
        if (audible && pk < 0.002f * master) {
            quiet++;
            if (quiet <= 3) {
                extern unsigned g_frame;
                rt_dlog("[mix] silent block with %d audible voices (of %d) frame %u:", audible, nv, g_frame);
                for (int j = 0; j < nv && j < 8; j++) rt_dlog(" [g %.2f pk %.2f pos %llu/%u]", vs[j].gl, vs[j].vpk, (unsigned long long)(vs[j].pos >> 16), vs[j].frames);
                rt_dlog("\n");
            }
        }
        static unsigned nblk, nvmax, namax, ns0, ng0; static float sumpk; static double ms_max, ms_tot, cb_max;
        int naud = 0;
        for (int j = 0; j < nv; j++) if (!vs[j].ghost && (vs[j].gl > 0.f || vs[j].gr > 0.f)) naud++;
        if ((unsigned)nv > nvmax) nvmax = (unsigned)nv;
        if ((unsigned)naud > namax) namax = (unsigned)naud;
        if (pk > sumpk) sumpk = pk;
        double hzms = (double)SDL_GetPerformanceFrequency() / 1000.0;
        double ms = (double)(t_mix1 - t_mix0) / hzms, cb = (double)(SDL_GetPerformanceCounter() - t_mix0) / hzms;
        if (ms > ms_max) ms_max = ms;
        if (cb > cb_max) cb_max = cb;
        ms_tot += ms; nblk++;
        // Printed when the window had something notable (limiter, silent block, >= 12 voices, a dropped tail, mix
        // time > 5 ms per block), otherwise once a minute as a heartbeat.
        static unsigned quiet_windows;
        if (nvmax && now_ok(&t_last)) {
            extern unsigned g_frame;
            SDL_LockMutex(mx);
            unsigned s0 = st_starts, g0 = st_ghost_drop;
            SDL_UnlockMutex(mx);
            int notable = blocks || quiet || namax >= 12 || g0 != ng0 || ms_max > 5.0 || HK_ENV("HK_TRACE_DS");
            if (!notable && ++quiet_windows < 12) goto window_done;
            quiet_windows = 0;
            rt_dlog("[mix] 5 s: %u blocks (%u samples) above the limiter threshold %.2f, %u silent blocks with audible voices, frame %u\n",
                   blocks, samples_over, LIM_T, quiet, g_frame);
            rt_dlog("[mix]  load: %u blocks, voices max %u (audible %u), sum peak %.2f (master %.2f), mix ms max %.2f avg %.2f (callback max %.2f, block %.1f ms), starts %u tails-dropped %u\n",
                   nblk, nvmax, namax, sumpk, master, ms_max, nblk ? ms_tot / nblk : 0.0, cb_max, n * 1000.0 / OUT_RATE,
                   s0 - ns0, g0 - ng0);
            if (lim_worst_pk > 0.f) rt_dlog("[mix]  loudest: %s\n", lim_worst);
        window_done:
            ns0 = s0; ng0 = g0;
            blocks = quiet = samples_over = 0; nblk = nvmax = namax = 0; sumpk = 0.f; ms_max = ms_tot = cb_max = 0.0; lim_worst_pk = 0.f;
        }
    }
#endif   // HK_DEVTOOLS: limiter and [mix] diagnostics
    if (n * 4 < len) memset(stream + n * 4, 0, (size_t)(len - n * 4));
}

static void audio_start(void) {
    if (audio_dev) return;
    if (!mx) mx = SDL_CreateMutex();
    if (HK_ENV("HK_NOSOUND")) return;
    { const char *e = HK_ENV("HK_MUSIC"); if (e) g_hk_music_volume = (float)atof(e); }   // test aid: music gain on PC
#if HK_DEVTOOLS
    { const char *e = getenv("HK_REC"); if (e && *e && !g_hk_rec_path) g_hk_rec_path = strcmp(e, "1") ? e : "rec.wav"; }
#endif
    SDL_AudioSpec want = {0}, have;
    want.freq = OUT_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = MIX_FRAMES;
    want.callback = mix;
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_dev) rt_log("[dsound] SDL audio: %s\n", SDL_GetError());
    else {
        rt_log("[dsound] audio %d Hz, %d frames\n", have.freq, have.samples);
        rec_start();
        SDL_PauseAudioDevice(audio_dev, 0);
    }
}

// Under mx (or before the buffer is visible to the mixer).
static void free_buffer(DSBuf *b) {
    if (b->shared_refs && --*b->shared_refs == 0) { gheap_free(b->mem); free(b->shared_refs); }
    obj_unwrap(b);
    free(b);
}

// ---------- IDirectSoundNotify ----------
static void nt_qi(CPU *c) { HOSTFN; if (ARG(3)) W32(ARG(3), 0); STDRET(3, E_NOINTERFACE_); }
static void nt_addref(CPU *c) { HOSTFN; DSBuf *s = obj_of(ARG(1), K_NOTIFY); STDRET(1, s ? (uint32_t)++s->refs : 0); }
static void buf_release_ref(DSBuf *b);
static void nt_release(CPU *c) {
    HOSTFN;
    DSBuf *s = obj_of(ARG(1), K_NOTIFY);
    if (!s) STDRET(1, 0);
    int r = --s->refs;
    if (r <= 0) { DSBuf *p = s->parent; obj_unwrap(s); free(s); if (p) buf_release_ref(p); r = 0; }
    STDRET(1, (uint32_t)r);
}
static void nt_set_positions(CPU *c) {   // (this, count, DSBPOSITIONNOTIFY {offset, event}[])
    HOSTFN;
    DSBuf *s = obj_of(ARG(1), K_NOTIFY);
    uint32_t n = ARG(2), p = ARG(3);
    if (s && s->parent) {
        DSBuf *b = s->parent;
        SDL_LockMutex(mx);
        b->nnotify = n > 32 ? 32 : (int)n;
        for (int i = 0; i < b->nnotify; i++) { b->notify[i].off = R32(p + 8 * i); b->notify[i].ev = R32(p + 8 * i + 4); }
        SDL_UnlockMutex(mx);
    }
    STDRET(3, DS_OK);
}

// ---------- IDirectSoundBuffer ----------
#define BUFN(n) DSBuf *b = obj_of(ARG(1), K_BUF); if (!b) STDRET(n, DSERR_INVALIDPARAM)
static void buf_qi(CPU *c) {
    HOSTFN;
    DSBuf *b = obj_of(ARG(1), K_BUF);
    uint32_t iid = ARG(2) ? R32(ARG(2)) : 0, out = ARG(3);
    if (out) W32(out, 0);
    if (!b || !out) STDRET(3, DSERR_INVALIDPARAM);
    if (iid == 0xb0210783u) {   // IID_IDirectSoundNotify
        DSBuf *s = calloc(1, sizeof *s);
        if (!s) STDRET(3, E_OUTOFMEMORY_);
        s->parent = b;
        b->refs++;
        W32(out, obj_wrap(s, K_NOTIFY, g_vt_notify));
        STDRET(3, DS_OK);
    }
    rt_dlog("[dsound] buffer QueryInterface %08x not supported\n", iid);
    STDRET(3, E_NOINTERFACE_);
}
static void buf_addref(CPU *c) { HOSTFN; BUFN(1); STDRET(1, (uint32_t)++b->refs); }
static void buf_release_ref(DSBuf *b) {
    SDL_LockMutex(mx);
    if (--b->refs <= 0) free_buffer(b);
    SDL_UnlockMutex(mx);
}
static void buf_release(CPU *c) {
    HOSTFN;
    if (HK_ENV("HK_TRACE_DS")) rt_dlog("[dsound] Release %08x (ret %08x)\n", ARG(1), R32(c->esp));
    BUFN(1);
    int r = b->refs - 1;
    buf_release_ref(b);
    STDRET(1, (uint32_t)(r < 0 ? 0 : r));
}
static void buf_get_caps(CPU *c) {   // DSBCAPS {size, flags, bufferBytes, unlockTransferRate, playCpuOverhead}
    HOSTFN; BUFN(2);
    W32(ARG(2) + 4, b->flags); W32(ARG(2) + 8, b->size); W32(ARG(2) + 12, 0); W32(ARG(2) + 16, 0);
    STDRET(2, DS_OK);
}
static void buf_get_position(CPU *c) {
    HOSTFN; BUFN(3);
    SDL_LockMutex(mx);
    uint32_t play = b->align ? (uint32_t)b->pos * b->align : 0;
    SDL_UnlockMutex(mx);
    uint32_t lead = b->align ? (b->rate / 50) * b->align : 0;   // ~20 ms write-ahead
    if (ARG(2)) W32(ARG(2), play);
    if (ARG(3)) W32(ARG(3), b->size ? (play + lead) % b->size : 0);
    STDRET(3, DS_OK);
}
static void buf_get_format(CPU *c) {
    HOSTFN; BUFN(4);
    uint32_t p = ARG(2);
    if (p) {
        W16(p, 1); W16(p + 2, b->ch); W32(p + 4, b->rate); W32(p + 8, b->rate * b->align);
        W16(p + 12, b->align); W16(p + 14, b->bits); if (ARG(3) >= 18) W16(p + 16, 0);
    }
    if (ARG(4)) W32(ARG(4), 18);
    STDRET(4, DS_OK);
}
static void buf_get_volume(CPU *c) { HOSTFN; BUFN(2); W32(ARG(2), (uint32_t)b->volume); STDRET(2, DS_OK); }
static void buf_get_pan(CPU *c) { HOSTFN; BUFN(2); W32(ARG(2), (uint32_t)b->pan); STDRET(2, DS_OK); }
static void buf_get_frequency(CPU *c) { HOSTFN; BUFN(2); W32(ARG(2), b->freq ? b->freq : b->rate); STDRET(2, DS_OK); }
static void buf_get_status(CPU *c) {
    HOSTFN; BUFN(2);
    W32(ARG(2), (b->playing ? 1u : 0u) | (b->playing && b->looping ? 4u : 0u));
    STDRET(2, DS_OK);
}
static void buf_initialize(CPU *c) { HOSTFN; STDRET(3, 0x88780082u); }   // DSERR_ALREADYINITIALIZED
// Lock(this, offset, bytes, ptr1*, bytes1*, ptr2*, bytes2*, flags): FROMWRITECURSOR 1, ENTIREBUFFER 2.
HC_WRAP(HC_SND, buf_lock) {
    HOSTFN; BUFN(8);
    uint32_t off = ARG(2), n = ARG(3), flags = ARG(8);
    if (b->primary || !b->size) STDRET(8, DSERR_INVALIDCALL);
    if (flags & 2) { off = 0; n = b->size; }
    if (flags & 1) {
        SDL_LockMutex(mx);
        off = b->align ? ((uint32_t)b->pos * b->align + (b->rate / 50) * b->align) % b->size : 0;
        SDL_UnlockMutex(mx);
    }
    off %= b->size;
    if (n > b->size) n = b->size;
    uint32_t n1 = n < b->size - off ? n : b->size - off;
    W32(ARG(4), b->mem + off);
    W32(ARG(5), n1);
    if (ARG(6)) W32(ARG(6), n1 < n ? b->mem : 0);
    if (ARG(7)) W32(ARG(7), n - n1);
    STDRET(8, DS_OK);
}
// Play/Stop/SetCurrentPosition/SetVolume/SetPan/SetFrequency follow DirectSound 5 exactly; every voice that plays is
// mixed at the gain the game set. Until 2026-11-23 buf_play also decided which copies of a sample were heard
// (copies started together mixed silent, at most 6 audible copies with the oldest faded out, a louder copy taking
// over, 250 ms fade-in of ambience loops). Those rules answered console logs of block peaks of 2-9x full scale in
// the a0 house explosion and the bazooka/a2 fights, which were caused by full-scale DC voices created for missing
// WAVs on the Vita (port/rt/heap.c, too_big), not by the game's own copies: on PC, a0 with every copy audible
// peaks at 0.4-0.5 of full scale (master 0.22). The game bounds its copies itself (voice pool, table 0x4A5BA0).
#define TR2(name, val) do { static int t2 = -1; if (t2 < 0) { const char *e = HK_ENV("HK_TRACE_DS"); t2 = e && atoi(e) >= 2; } \
    if (t2) { extern unsigned g_frame; rt_dlog("[ds2] f%u %s #%u %d pl %d pos %.0f/%u rate %u vol %d (ret %08x)\n", g_frame, name, R32(b->guest + 4) & 0xffffu, (int)(val), b->playing, b->pos, b->align ? b->size / b->align : 0, b->rate, b->volume, R32(c->esp)); } } while (0)
HC_WRAP(HC_SND, buf_play) {   // (this, reserved, priority, flags): DSBPLAY_LOOPING 1
    HOSTFN; BUFN(4);
    TR2("Play", ARG(4));
    if (b->primary) STDRET(4, DS_OK);   // the primary buffer "plays" whenever a secondary one does
    SDL_LockMutex(mx);
    // Play on a playing buffer does not restart it: it keeps its position and only takes the new looping flag.
    // Play on a stopped buffer resumes from its current position (0 after a one-shot ended, kept after Stop).
    if (!b->playing) {
        b->gen = ++g_gen;
        b->ramp = 0.f;
        memset(b->lp, 0, sizeof b->lp);
        st_starts++;
        {   // HK_TRACE_DS=2: other copies of this sample already playing (age in ms) when it starts
            static int t2 = -1; if (t2 < 0) { const char *e = HK_ENV("HK_TRACE_DS"); t2 = e && atoi(e) >= 2; }
            if (t2 && b->mem) {
                extern unsigned g_frame; int cnt = 0;
                for (int k = 1; k < MAX_OBJ; k++) { DSBuf *o = g_obj[k];
                    if (o && o != b && o->kind == K_BUF && o->playing && o->mem == b->mem && o->rate) {
                        if (!cnt++) rt_dlog("[copies] f%u %08x/%u vol %d:", g_frame, b->mem, b->size / b->align, b->volume);
                        rt_dlog(" %.0fms", o->pos * 1000.0 / o->rate);
                    } }
                if (cnt) rt_dlog("\n");
            }
        }
    }
    b->playing = 1;
    b->looping = ARG(4) & 1;
    SDL_UnlockMutex(mx);
    STDRET(4, DS_OK);
}
// SetCurrentPosition: playing or not, the buffer continues from the new offset (a playing one keeps playing).
static void buf_set_position(CPU *c) {
    HOSTFN; BUFN(2);
    TR2("SetPos", ARG(2));
    if (b->primary || !b->align || ARG(2) >= b->size) STDRET(2, DSERR_INVALIDPARAM);
    SDL_LockMutex(mx);
    ghost_add(b);   // the old position fades out while the new one fades in
    b->pos = (double)(ARG(2) / b->align);
    if (b->playing) { b->ramp = 0.f; memset(b->lp, 0, sizeof b->lp); }
    b->gen = ++g_gen;
    SDL_UnlockMutex(mx);
    STDRET(2, DS_OK);
}
static void buf_set_format(CPU *c) { HOSTFN; STDRET(2, DS_OK); }   // primary buffer format: the mixer resamples anyway
// Out-of-range values fail with DSERR_INVALIDPARAM and leave the setting as it was, as in DirectSound. Logged once.
static void bad_param(const char *what, int32_t v) {
    static int n;
    if (n++ < 8) rt_dlog("[dsound] %s(%d) out of range: DSERR_INVALIDPARAM\n", what, v);
}
static void buf_set_volume(CPU *c) {   // DSBVOLUME_MIN -10000 .. DSBVOLUME_MAX 0 (hundredths of dB)
    HOSTFN; BUFN(2); TR2("Vol", ARG(2));
    int32_t v = (int32_t)ARG(2);
    if (v < -10000 || v > 0) { bad_param("SetVolume", v); STDRET(2, DSERR_INVALIDPARAM); }
    b->volume = v;
    STDRET(2, DS_OK);
}
static void buf_set_pan(CPU *c) {   // DSBPAN_LEFT -10000 .. DSBPAN_RIGHT 10000
    HOSTFN; BUFN(2); TR2("Pan", ARG(2));
    int32_t v = (int32_t)ARG(2);
    if (v < -10000 || v > 10000) { bad_param("SetPan", v); STDRET(2, DSERR_INVALIDPARAM); }
    b->pan = v;
    STDRET(2, DS_OK);
}
static void buf_set_frequency(CPU *c) {   // DSBFREQUENCY_ORIGINAL 0, or DSBFREQUENCY_MIN 100 .. DSBFREQUENCY_MAX 100000
    HOSTFN; BUFN(2); TR2("Freq", ARG(2));
    uint32_t f = ARG(2);
    if (f && (f < 100 || f > 100000)) { bad_param("SetFrequency", (int32_t)f); STDRET(2, DSERR_INVALIDPARAM); }
    b->freq = f;
    STDRET(2, DS_OK);
}
// Stop keeps the position: a later Play resumes there.
HC_WRAP(HC_SND, buf_stop) {
    HOSTFN; BUFN(1);
    TR2("Stop", 0);
    SDL_LockMutex(mx);
    ghost_add(b);
    b->playing = 0;
    b->gen = ++g_gen;
    for (int k = 0; k < b->nnotify; k++)
        if (b->notify[k].off == 0xffffffffu) w32_event_set(b->notify[k].ev, 1);
    SDL_UnlockMutex(mx);
    STDRET(1, DS_OK);
}
static void buf_unlock(CPU *c) { HOSTFN; STDRET(5, DS_OK); }
static void buf_restore(CPU *c) { HOSTFN; STDRET(1, DS_OK); }

// ---------- IDirectSound ----------
static void ds_qi(CPU *c) { HOSTFN; if (ARG(3)) W32(ARG(3), 0); STDRET(3, E_NOINTERFACE_); }
static void ds_addref(CPU *c) { HOSTFN; DSBuf *o = obj_of(ARG(1), K_DS); STDRET(1, o ? (uint32_t)++o->refs : 0); }
static void ds_release(CPU *c) { HOSTFN; DSBuf *o = obj_of(ARG(1), K_DS); STDRET(1, o && o->refs > 1 ? (uint32_t)--o->refs : 0); }
// CreateSoundBuffer(this, DSBUFFERDESC {size, flags, bytes, reserved, wfx*}, out, outer)
HC_WRAP(HC_SND, ds_create_buffer) {
    HOSTFN;
    W32(ARG(3), 0);
    uint32_t d = ARG(2), flags = R32(d + 4), bytes = R32(d + 8), wfx = R32(d + 16);
    // A secondary buffer needs a valid PCM format and DSBSIZE_MIN 4 .. DSBSIZE_MAX 0x0FFFFFFF bytes, as in DirectSound
    // (DSERR_BADFORMAT / DSERR_INVALIDPARAM otherwise): the buffer is not created. HDKWIN.EXE's loader handles the
    // failure (0x45C59C: Release, no voice). A garbage format used to be accepted and mixed as 8-bit (see heap.c too_big).
    if (!(flags & 1)) {
        uint32_t tag = wfx ? R16(wfx) : 0, ch = wfx ? R16(wfx + 2) : 0, rate = wfx ? R32(wfx + 4) : 0;
        uint32_t al = wfx ? R16(wfx + 12) : 0, bits = wfx ? R16(wfx + 14) : 0;
        uint32_t err = !wfx || bytes < 4 || bytes > 0x0FFFFFFFu ? DSERR_INVALIDPARAM
                     : tag != 1 || ch < 1 || ch > 2 || (bits != 8 && bits != 16) || al != ch * bits / 8 ||
                       rate < 100 || rate > 100000 ? 0x88780064u : 0;   // DSERR_BADFORMAT
        if (err) {
            rt_log("[dsound] CreateSoundBuffer refused: bytes %u format tag %u ch %u rate %u align %u bits %u (ret %08x)\n",
                   bytes, tag, ch, rate, al, bits, R32(c->esp));
            STDRET(4, err);
        }
    }
    DSBuf *b = calloc(1, sizeof *b);
    if (!b) STDRET(4, E_OUTOFMEMORY_);
    b->flags = flags;
    b->primary = flags & 1;
    if (!b->primary && wfx) {
        b->ch = R16(wfx + 2); b->rate = R32(wfx + 4); b->align = R16(wfx + 12); b->bits = R16(wfx + 14);
        b->size = bytes;
        b->mem = gheap_calloc(bytes ? bytes : 4);
        b->shared_refs = calloc(1, sizeof(int));
        if (!b->mem || !b->shared_refs) { gheap_free(b->mem); free(b->shared_refs); free(b); STDRET(4, E_OUTOFMEMORY_); }
        *b->shared_refs = 1;
        if (bytes >= (1u << 20) && RT_TRACE("HK_MEMLOG")) rt_dlog("[dsound] big buffer %u KB (guest %08x) at frame %u\n", bytes >> 10, b->mem, g_frame);
        if (b->bits == 8) memset(g_mem + b->mem, 0x80, bytes);   // 8-bit silence
    }
    SDL_LockMutex(mx);
    W32(ARG(3), obj_wrap(b, K_BUF, g_vt_buf));
    SDL_UnlockMutex(mx);
    if (HK_ENV("HK_TRACE_DS")) rt_dlog("[dsound] CreateSoundBuffer flags=%08x bytes=%u ch=%u rate=%u bits=%u -> %08x (ret %08x)\n",
                                      flags, bytes, b->ch, b->rate, b->bits, R32(ARG(3)), R32(c->esp));
    STDRET(4, DS_OK);
}
// GetCaps(this, DSCAPS): an emulated (software) device, all sample formats, no hardware mixing; every field is
// written, since HDKWIN.EXE passes an uninitialized structure.
static void ds_get_caps(CPU *c) {
    HOSTFN;
    uint32_t p = ARG(2);
    if (!p) STDRET(2, DSERR_INVALIDPARAM);
    uint32_t size = R32(p) >= 8 && R32(p) <= 256 ? R32(p) : 96;
    memset(mb + p + 4, 0, size - 4);
    W32(p + 4, 0x0Fu | 0x10u | 0x20u);   // PRIMARYMONO|STEREO|8BIT|16BIT, CONTINUOUSRATE, EMULDRIVER
    W32(p + 8, 100); W32(p + 12, 100000);   // secondary sample rates
    W32(p + 16, 1);                          // primary buffers
    if (HK_ENV("HK_TRACE_DS")) rt_dlog("[dsound] GetCaps size %u (ret %08x)\n", R32(p), R32(c->esp));
    STDRET(2, DS_OK);
}
HC_WRAP(HC_SND, ds_duplicate) {
    HOSTFN;
    DSBuf *o = obj_of(ARG(2), K_BUF);
    if (!o) { W32(ARG(3), 0); STDRET(3, DSERR_INVALIDPARAM); }
    DSBuf *b = calloc(1, sizeof *b);
    if (!b) { W32(ARG(3), 0); STDRET(3, E_OUTOFMEMORY_); }
    SDL_LockMutex(mx);
    *b = *o;
    b->playing = 0; b->pos = 0; b->nnotify = 0; b->ramp = 0.f;
    memset(b->lp, 0, sizeof b->lp);
    if (b->shared_refs) ++*b->shared_refs;
    W32(ARG(3), obj_wrap(b, K_BUF, g_vt_buf));
    SDL_UnlockMutex(mx);
    if (HK_ENV("HK_TRACE_DS")) rt_dlog("[dsound] Duplicate %08x -> %08x (ret %08x)\n", ARG(2), R32(ARG(3)), R32(c->esp));
    STDRET(3, DS_OK);
}
static void ds_set_coop(CPU *c) { HOSTFN; STDRET(3, DS_OK); }
static void ds_compact(CPU *c) { HOSTFN; STDRET(1, DS_OK); }
static void ds_get_speaker(CPU *c) { HOSTFN; W32(ARG(2), 4); STDRET(2, DS_OK); }   // DSSPEAKER_STEREO
static void ds_set_speaker(CPU *c) { HOSTFN; STDRET(2, DS_OK); }
static void ds_initialize(CPU *c) { HOSTFN; STDRET(2, DS_OK); }

static uint32_t make_vtable(const gfn *fns, int n, const char *name) {
    HOSTFN;
    uint32_t vt = gheap_calloc((uint32_t)n * 4u);
    for (int i = 0; i < n; i++) W32(vt + 4u * i, rt_host_addr(fns[i], name));
    return vt;
}
static void build_vtables(void) {
    static const gfn ds[] = {ds_qi, ds_addref, ds_release, ds_create_buffer, ds_get_caps, ds_duplicate, ds_set_coop,
                             ds_compact, ds_get_speaker, ds_set_speaker, ds_initialize};
    static const gfn buf[] = {buf_qi, buf_addref, buf_release, buf_get_caps, buf_get_position, buf_get_format,
                              buf_get_volume, buf_get_pan, buf_get_frequency, buf_get_status, buf_initialize, buf_lock,
                              buf_play, buf_set_position, buf_set_format, buf_set_volume, buf_set_pan, buf_set_frequency,
                              buf_stop, buf_unlock, buf_restore};
    static const gfn nt[] = {nt_qi, nt_addref, nt_release, nt_set_positions};
    g_vt_ds = make_vtable(ds, 11, "IDirectSound");
    g_vt_buf = make_vtable(buf, 21, "IDirectSoundBuffer");
    g_vt_notify = make_vtable(nt, 4, "IDirectSoundNotify");
}

// DSOUND ordinal 1 = DirectSoundCreate(guid, out, outer)
// HK_NOSOUND=1 runs without a sound driver. Sound depends on the small-block-heap patch in port/hk/glue.c.
void imp_DSOUND_ord1(CPU *c) {
    HOSTFN;
    audio_start();
    if (!g_vt_ds) build_vtables();
    DSBuf *o = calloc(1, sizeof *o);
    if (!o || !audio_dev) { free(o); if (ARG(2)) W32(ARG(2), 0); STDRET(3, 0x88780078u); }   // DSERR_NODRIVER
    SDL_LockMutex(mx);
    W32(ARG(2), obj_wrap(o, K_DS, g_vt_ds));
    SDL_UnlockMutex(mx);
    rt_dlog("[dsound] DirectSoundCreate\n");
    STDRET(3, DS_OK);
}

// ---------- WINMM!PlaySoundA(sound, module, flags): SND_ASYNC 1, SND_MEMORY 4, SND_PURGE 0x40, SND_FILENAME 0x20000 ----------
// One sound at a time, as on Windows: a new call stops the previous one. NULL sound stops playback.
static DSBuf *g_playsound;
void imp_WINMM_PlaySoundA(CPU *c) {
    HOSTFN;
    uint32_t snd = ARG(1), flags = ARG(3);
    if (HK_ENV("HK_TRACE_DS")) rt_dlog("[winmm] PlaySoundA(%s, flags %08x)\n", snd && !(flags & 4u) ? GSTR(snd) : "(memory/null)", flags);
    audio_start();
    if (!g_vt_ds) build_vtables();
    SDL_LockMutex(mx);
    if (g_playsound && obj_of(g_playsound->guest, K_BUF) == g_playsound) { g_playsound->playing = 0; g_playsound->oneshot = 1; }
    g_playsound = NULL;
    SDL_UnlockMutex(mx);
    if (!snd || (flags & 0x40u) || !audio_dev) STDRET(3, 1);
    uint8_t *data = NULL;
    long n = 0;
    int owned = 0;
    if (flags & 4u) { data = mb + snd; n = 0x7fffffff; }
    else {
        char hp[600];
        w32_host_path(GSTR(snd), hp, sizeof hp);
        FILE *f = fopen(hp, "rb");
        if (!f) { rt_log("[winmm] PlaySoundA: cannot open %s\n", hp); STDRET(3, 0); }
        fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
        data = malloc((size_t)n);
        owned = 1;
        if (!data || fread(data, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(data); STDRET(3, 0); }
        fclose(f);
    }
    // RIFF/WAVE: find "fmt " and "data"
    uint32_t ch = 0, rate = 0, bits = 0, align = 0, doff = 0, dlen = 0;
    if (n >= 12 && !memcmp(data, "RIFF", 4) && !memcmp(data + 8, "WAVE", 4)) {
        uint32_t riff; memcpy(&riff, data + 4, 4);
        long end = (long)riff + 8 < n ? (long)riff + 8 : n;
        for (long p = 12; p + 8 <= end;) {
            uint32_t sz; memcpy(&sz, data + p + 4, 4);
            if (!memcmp(data + p, "fmt ", 4) && sz >= 16) {
                uint16_t v16; memcpy(&v16, data + p + 10, 2); ch = v16;
                memcpy(&rate, data + p + 12, 4);
                memcpy(&v16, data + p + 20, 2); align = v16;
                memcpy(&v16, data + p + 22, 2); bits = v16;
            } else if (!memcmp(data + p, "data", 4)) { doff = (uint32_t)p + 8; dlen = sz; break; }
            p += 8 + (long)((sz + 1) & ~1u);
        }
    }
    if (!align || !dlen || (bits != 8 && bits != 16)) { if (owned) free(data); rt_log("[winmm] PlaySoundA: unsupported WAV\n"); STDRET(3, 0); }
    DSBuf *b = calloc(1, sizeof *b);
    uint32_t mem = b ? gheap_alloc(dlen) : 0;
    int *refs = b ? calloc(1, sizeof(int)) : NULL;
    if (!b || !mem || !refs) { free(b); gheap_free(mem); free(refs); if (owned) free(data); STDRET(3, 0); }
    memcpy(g_mem + mem, data + doff, dlen);
    if (owned) free(data);
    b->ch = ch; b->rate = rate; b->bits = bits; b->align = align; b->size = dlen; b->mem = mem;
    b->shared_refs = refs; *refs = 1;
    b->oneshot = 1;
    b->looping = (flags & 8u) != 0;   // SND_LOOP
    SDL_LockMutex(mx);
    obj_wrap(b, K_BUF, g_vt_buf);
    b->playing = 1;
    g_playsound = b;
    SDL_UnlockMutex(mx);
    STDRET(3, 1);
}
