// DirectSound (DirectX 5 interfaces) for the recompiled Handkerchief: SDL software mixer, 48 kHz stereo.
// Derived from sh3vita port/src/dx/dsound.c (GPL-3.0): the mixer, Lock/Play/position logic and buffer duplication
// are kept; 3D sound, effects and movie audio (Silent Hill 3 only) are removed.
//
// COM objects live in guest memory as {vtable, tag} like port/hk/ddraw.c. Buffer samples stay in guest memory,
// so the game writes them through Lock() and the SDL audio thread reads them directly.
// Also PlaySoundA (WINMM): a WAV file or memory image played once through the same mixer.
#include "win32.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define OUT_RATE 48000
#define MIX_FRAMES 1024u
#define DS_OK 0u
#define DSERR_INVALIDPARAM 0x80070057u
#define DSERR_INVALIDCALL 0x88780032u
#define E_NOINTERFACE_ 0x80004002u
#define E_OUTOFMEMORY_ 0x8007000Eu

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
    int *shared_refs;           // sample memory shared between duplicates
    struct DSBuf *parent;       // K_NOTIFY: the buffer it belongs to
} DSBuf;

#define MAX_OBJ 512
static DSBuf *g_obj[MAX_OBJ];
static SDL_mutex *mx;
static SDL_AudioDeviceID audio_dev;
static float mix_acc[MIX_FRAMES * 2];
static uint32_t g_vt_ds, g_vt_buf, g_vt_notify;

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
static void obj_unwrap(DSBuf *b) {
    HOSTFN;
    uint32_t i = R32(b->guest + 4) & 0xffffu;
    if (i < MAX_OBJ && g_obj[i] == b) g_obj[i] = NULL;
    W32(b->guest + 4, 0x44530000u);   // dead: index 0
}

static float sample(DSBuf *b, uint32_t frame, int chn) {
    if (frame * b->align >= b->size) return 0;
    const uint8_t *p = g_mem + b->mem + frame * b->align;
    int ch = b->ch > 1 ? chn : 0;
    if (b->bits == 16) { int16_t s; memcpy(&s, p + 2 * ch, 2); return s / 32768.f; }
    return ((int)p[ch] - 128) / 128.f;
}

static void free_buffer(DSBuf *b);
static void mix(void *ud, Uint8 *stream, int len) {
    (void)ud;
    int n = len / 4;
    if (n <= 0) return;
    if ((unsigned)n > MIX_FRAMES) n = MIX_FRAMES;   // the device is opened with MIX_FRAMES samples
    float *acc = mix_acc;
    memset(acc, 0, (size_t)n * 2 * sizeof(float));
    SDL_LockMutex(mx);
    for (int k = 1; k < MAX_OBJ; k++) {
        DSBuf *b = g_obj[k];
        if (!b || b->kind != K_BUF || !b->playing || b->primary || !b->align) continue;
        float g = db_gain(b->volume), gl = g, gr = g;
        if (b->pan < 0) gr *= db_gain(b->pan); else if (b->pan > 0) gl *= db_gain(-b->pan);
        double step = (double)(b->freq ? b->freq : b->rate) / OUT_RATE;
        uint32_t frames = b->size / b->align;
        double before = b->pos;
        for (int i = 0; i < n && b->playing; i++) {
            uint32_t f = (uint32_t)b->pos;
            float t = (float)(b->pos - f);
            uint32_t f2 = f + 1 < frames ? f + 1 : (b->looping ? 0 : f);
            acc[2 * i] += (sample(b, f, 0) * (1 - t) + sample(b, f2, 0) * t) * gl;
            acc[2 * i + 1] += (sample(b, f, 1) * (1 - t) + sample(b, f2, 1) * t) * gr;
            b->pos += step;
            if (b->pos >= frames) {
                if (b->looping) b->pos -= frames;
                else { b->pos = 0; b->playing = 0; }
            }
        }
        for (int q = 0; q < b->nnotify; q++) {   // position notifications crossed during this block
            uint32_t off = b->notify[q].off;
            if (off == 0xffffffffu) { if (!b->playing) w32_event_set(b->notify[q].ev, 1); continue; }
            double fo = (double)off / b->align;
            int crossed = b->pos >= before ? (fo >= before && fo < b->pos) : (fo >= before || fo < b->pos);
            if (crossed) w32_event_set(b->notify[q].ev, 1);
        }
        if (b->oneshot && !b->playing) free_buffer(b);
    }
    SDL_UnlockMutex(mx);
    int16_t *out = (int16_t *)stream;
    for (int i = 0; i < n * 2; i++) {
        float v = acc[i] * 32767.f;
        out[i] = (int16_t)(v > 32767.f ? 32767 : v < -32768.f ? -32768 : v);
    }
    if (n * 4 < len) memset(stream + n * 4, 0, (size_t)(len - n * 4));
}

static void audio_start(void) {
    if (audio_dev) return;
    if (!mx) mx = SDL_CreateMutex();
    if (getenv("HK_NOSOUND")) return;
    SDL_AudioSpec want = {0}, have;
    want.freq = OUT_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = MIX_FRAMES;
    want.callback = mix;
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_dev) rt_log("[dsound] SDL audio: %s\n", SDL_GetError());
    else { SDL_PauseAudioDevice(audio_dev, 0); rt_log("[dsound] audio %d Hz, %d frames\n", have.freq, have.samples); }
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
    rt_log("[dsound] buffer QueryInterface %08x not supported\n", iid);
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
    if (getenv("HK_TRACE_DS")) rt_log("[dsound] Release %08x (ret %08x)\n", ARG(1), R32(c->esp));
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
static void buf_lock(CPU *c) {
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
static void buf_play(CPU *c) {   // (this, reserved, priority, flags): DSBPLAY_LOOPING 1
    HOSTFN; BUFN(4);
    SDL_LockMutex(mx);
    b->playing = 1;
    b->looping = ARG(4) & 1;
    SDL_UnlockMutex(mx);
    STDRET(4, DS_OK);
}
static void buf_set_position(CPU *c) {
    HOSTFN; BUFN(2);
    SDL_LockMutex(mx);
    if (b->align) b->pos = (double)(ARG(2) / b->align);
    SDL_UnlockMutex(mx);
    STDRET(2, DS_OK);
}
static void buf_set_format(CPU *c) { HOSTFN; STDRET(2, DS_OK); }   // primary buffer format: the mixer resamples anyway
static void buf_set_volume(CPU *c) { HOSTFN; BUFN(2); b->volume = (int32_t)ARG(2); STDRET(2, DS_OK); }
static void buf_set_pan(CPU *c) { HOSTFN; BUFN(2); b->pan = (int32_t)ARG(2); STDRET(2, DS_OK); }
static void buf_set_frequency(CPU *c) { HOSTFN; BUFN(2); b->freq = ARG(2); STDRET(2, DS_OK); }
static void buf_stop(CPU *c) {
    HOSTFN; BUFN(1);
    SDL_LockMutex(mx);
    b->playing = 0;
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
static void ds_create_buffer(CPU *c) {
    HOSTFN;
    W32(ARG(3), 0);
    uint32_t d = ARG(2), flags = R32(d + 4), bytes = R32(d + 8), wfx = R32(d + 16);
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
        if (b->bits == 8) memset(g_mem + b->mem, 0x80, bytes);   // 8-bit silence
    }
    SDL_LockMutex(mx);
    W32(ARG(3), obj_wrap(b, K_BUF, g_vt_buf));
    SDL_UnlockMutex(mx);
    if (getenv("HK_TRACE_DS")) rt_log("[dsound] CreateSoundBuffer flags=%08x bytes=%u ch=%u rate=%u bits=%u -> %08x (ret %08x)\n",
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
    if (getenv("HK_TRACE_DS")) rt_log("[dsound] GetCaps size %u (ret %08x)\n", R32(p), R32(c->esp));
    STDRET(2, DS_OK);
}
static void ds_duplicate(CPU *c) {
    HOSTFN;
    DSBuf *o = obj_of(ARG(2), K_BUF);
    if (!o) { W32(ARG(3), 0); STDRET(3, DSERR_INVALIDPARAM); }
    DSBuf *b = calloc(1, sizeof *b);
    if (!b) { W32(ARG(3), 0); STDRET(3, E_OUTOFMEMORY_); }
    SDL_LockMutex(mx);
    *b = *o;
    b->playing = 0; b->pos = 0; b->nnotify = 0;
    if (b->shared_refs) ++*b->shared_refs;
    W32(ARG(3), obj_wrap(b, K_BUF, g_vt_buf));
    SDL_UnlockMutex(mx);
    if (getenv("HK_TRACE_DS")) rt_log("[dsound] Duplicate %08x -> %08x (ret %08x)\n", ARG(2), R32(ARG(3)), R32(c->esp));
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
// Off unless HK_SOUND is set: with sound on, HDKWIN.EXE's sound list (0x45CE4C) ends up holding a sound object whose
// small-block-heap memory was reused, and the next Stop call jumps to garbage (docs/status.md). Until that is
// understood, the game runs without a sound driver, as before.
void imp_DSOUND_ord1(CPU *c) {
    HOSTFN;
    if (!getenv("HK_SOUND")) { if (ARG(2)) W32(ARG(2), 0); STDRET(3, 0x88780078u); }   // DSERR_NODRIVER
    audio_start();
    if (!g_vt_ds) build_vtables();
    DSBuf *o = calloc(1, sizeof *o);
    if (!o || !audio_dev) { free(o); if (ARG(2)) W32(ARG(2), 0); STDRET(3, 0x88780078u); }   // DSERR_NODRIVER
    SDL_LockMutex(mx);
    W32(ARG(2), obj_wrap(o, K_DS, g_vt_ds));
    SDL_UnlockMutex(mx);
    rt_log("[dsound] DirectSoundCreate\n");
    STDRET(3, DS_OK);
}

// ---------- WINMM!PlaySoundA(sound, module, flags): SND_ASYNC 1, SND_MEMORY 4, SND_PURGE 0x40, SND_FILENAME 0x20000 ----------
// One sound at a time, as on Windows: a new call stops the previous one. NULL sound stops playback.
static DSBuf *g_playsound;
void imp_WINMM_PlaySoundA(CPU *c) {
    HOSTFN;
    uint32_t snd = ARG(1), flags = ARG(3);
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
