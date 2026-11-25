// Handles Win32, attentes et chemins.
#include "win32.h"
#include "handle_ref.h"
#include <ctype.h>

// ref: calls running on the handle (handle_ref.h). A closed handle with a call still running keeps its slot (so its
// number is not reused) until that call releases it; h_get / h_type no longer see it.
// The table grows in pages of H_PAGE slots that are allocated on demand and never moved or freed, so the lock-free
// readers (h_get, h_type, w32_has_waiters) never see a slot move. Windows has no practical handle limit, and
// HDKWIN.EXE relies on that: it never calls mmioClose on the memory files it opens for every sound (0x45BCD1), so
// each stage load leaks about 9 handles and each save/load about 12. With the old fixed table of 1024 slots the port
// stopped with "handle table full" after about 85 stage loads or an hour on the title demo (docs/memory-stress.md).
// HK_HANDLE_PAGES=<n> (cfg handle_pages) limits the table to n pages; 1 gives the old 1024-slot behaviour.
#define H_PAGE 1024
#define H_MAXPAGES 64
typedef struct { int type; void *obj; HandleRef ref; uint16_t hwait; } HSlot;
static HSlot h_page0[H_PAGE];
static HSlot *h_pages[H_MAXPAGES] = {h_page0};
static int h_npages = 1;              // pages allocated (published after the page is zeroed)
int g_h_maxpages = H_MAXPAGES;        // cap (HK_HANDLE_PAGES / cfg handle_pages)
#define HT(i) (h_pages[(i) / H_PAGE][(i) % H_PAGE])
static SDL_mutex *hlock;
SDL_mutex *g_sync;
SDL_cond *g_sync_cv;

static void lazy_init(void) {
    if (!hlock) {
        hlock = SDL_CreateMutex();
        g_sync = SDL_CreateMutex();
        g_sync_cv = SDL_CreateCond();
    }
}

uint32_t h_new(int type, void *obj) {
    lazy_init();
    SDL_LockMutex(hlock);
    int np = __atomic_load_n(&h_npages, __ATOMIC_ACQUIRE);
    for (int i = 1;; i++) {
        if (i == np * H_PAGE) {   // every allocated slot is in use: add a page, or stop at the cap
            if (np >= g_h_maxpages || np >= H_MAXPAGES) break;
            HSlot *pg = SDL_calloc(H_PAGE, sizeof(HSlot));
            if (!pg) break;
            h_pages[np] = pg;
            __atomic_store_n(&h_npages, ++np, __ATOMIC_RELEASE);
            rt_dlog("[handles] table grown to %d slots\n", np * H_PAGE);
        }
        if (HT(i).type == H_FREE) {
            HT(i).type = type;
            HT(i).obj = obj;
            HT(i).ref = (HandleRef){0, 0};
            SDL_UnlockMutex(hlock);
            return 0x1000u + 4u * i;
        }
    }
    SDL_UnlockMutex(hlock);
    rt_fatal("handle table full");
}

// Live handles per type (HK_MEMLOG): counts[H_*], returns the total.
int h_stats(int counts[], int n) {
    int tot = 0;
    for (int k = 0; k < n; k++) counts[k] = 0;
    lazy_init();
    SDL_LockMutex(hlock);
    int np = __atomic_load_n(&h_npages, __ATOMIC_ACQUIRE);
    for (int i = 1; i < np * H_PAGE; i++)
        if (HT(i).type != H_FREE) { tot++; if (HT(i).type < n) counts[HT(i).type]++; }
    SDL_UnlockMutex(hlock);
    return tot;
}

static int idx(uint32_t h) {
    int i = (int)((h - 0x1000u) / 4u);
    return h >= 0x1000u && (h & 3) == 0 && i < __atomic_load_n(&h_npages, __ATOMIC_ACQUIRE) * H_PAGE ? i : 0;
}

int h_type(uint32_t h) { int i = idx(h); return HT(i).ref.closed ? H_FREE : HT(i).type; }

void *h_get(uint32_t h, int type) {
    int i = idx(h);
    return i && HT(i).type == type && !HT(i).ref.closed ? HT(i).obj : NULL;
}

static void slot_free(int i) {   // under hlock
    HT(i).type = H_FREE;
    HT(i).obj = NULL;
    HT(i).ref = (HandleRef){0, 0};
}

// Every call that uses a handle's object while another thread may close it: h_acquire ... h_release (handle_ref.h).
void *h_acquire(uint32_t h, int type) {
    int i = idx(h);
    if (!i) return NULL;
    lazy_init();
    SDL_LockMutex(hlock);
    void *obj = HT(i).type == type && hr_acquire(&HT(i).ref) ? HT(i).obj : NULL;
    SDL_UnlockMutex(hlock);
    return obj;
}
int h_release(uint32_t h) {
    int i = idx(h), last = 0;
    if (!i) return 0;
    SDL_LockMutex(hlock);
    if (HT(i).type != H_FREE && HT(i).ref.refs > 0 && (last = hr_release(&HT(i).ref))) slot_free(i);
    SDL_UnlockMutex(hlock);
    return last;
}
void *h_retire(uint32_t h, int type, int *now) {
    int i = idx(h);
    void *obj = NULL;
    *now = 0;
    if (!i) return NULL;
    lazy_init();
    SDL_LockMutex(hlock);
    if (HT(i).type == type && !HT(i).ref.closed) {
        obj = HT(i).obj;
        if ((*now = hr_close(&HT(i).ref))) slot_free(i);
    }
    SDL_UnlockMutex(hlock);
    return obj;
}

void h_close(uint32_t h) {
    int i = idx(h), now;
    if (i) h_retire(h, HT(i).type, &now);
}

int w32_signaled(uint32_t h) {   // a handle closed meanwhile reads as signalled (no NULL object behind it)
    switch (h_type(h)) {
    case H_EVENT: { Event *e = h_get(h, H_EVENT); return !e || e->state; }
    case H_THREAD: { Thread *t = h_get(h, H_THREAD); return !t || t->done; }
    case H_MUTEX: return 1;
    case H_SEM: { Semaphore *s = h_get(h, H_SEM); return !s || __atomic_load_n(&s->count, __ATOMIC_SEQ_CST) > 0; }
    default: return 1;
    }
}

// Semaphore counts are atomic: the game's try-lock 0x6099c0 (WaitForSingleObject with timeout 0) and
// its unlock 0x609a60 (ReleaseSemaphore) run many times per frame on the main thread; through the global g_sync mutex
// they waited on the guest threads that hold it (3.1% + 3.7% of the main thread in the park). Taking or adding a count
// is a compare-and-swap; g_sync is only needed to wake a thread that waits.
int w32_sem_try(uint32_t h) {
    Semaphore *s = h_get(h, H_SEM);
    if (!s) return 0;
    int32_t v = __atomic_load_n(&s->count, __ATOMIC_SEQ_CST);
    while (v > 0)
        if (__atomic_compare_exchange_n(&s->count, &v, v - 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) return 1;
    return 0;
}
int w32_consume(uint32_t h) {
    Event *e = h_get(h, H_EVENT);
    if (e && !e->manual) { if (e->tokens) e->tokens--; else e->state = 0; }
    if (h_type(h) == H_SEM) return w32_sem_try(h);
    return 1;
}

// PulseEvent: releases the threads waiting right now (all of them for a manual-reset event, one for an
// auto-reset event) and leaves the event unsignalled. Setting then resetting the state lost the wake-up whenever a
// waiter re-checked after the reset. Waiters register in w32_wait; a manual-reset pulse
// bumps the epoch they recorded, an auto-reset pulse leaves one token for the first waiter to consume.
void w32_event_pulse(uint32_t h) {
    lazy_init();
    w32_trace_sync("event pulse", h, 0);
    SDL_LockMutex(g_sync);
    Event *e = h_get(h, H_EVENT);
    if (e && e->waiters) {
        if (e->manual) e->epoch++;
        else if (!e->state) e->tokens = 1;
        SDL_CondBroadcast(g_sync_cv);
    }
    SDL_UnlockMutex(g_sync);
}

// Signalled for this waiter: as w32_signaled, plus a pulse it was registered for (epoch0: its epoch at wait start).
static int signaled_for(uint32_t h, uint32_t epoch0) {
    Event *e = h_get(h, H_EVENT);
    if (e) return e->state || e->tokens || (e->manual && e->epoch != epoch0);
    return w32_signaled(h);
}
// Threads in w32_wait right now, per handle (under g_sync). A semaphore release or event set with nobody waiting on
// that handle does not wake every waiting thread ( ReleaseSemaphore called by the game's sound code
// took 3.7% of the main thread in the park, each call waking all the guest threads that share core 1 with rendering).
int w32_has_waiters(uint32_t h) { return __atomic_load_n(&HT(idx(h)).hwait, __ATOMIC_SEQ_CST) != 0; }
static void register_waiter(uint32_t n, const uint32_t *hs, uint32_t *epoch0, int delta) {   // under g_sync
    for (uint32_t i = 0; i < n; i++) {
        __atomic_add_fetch(&HT(idx(hs[i])).hwait, (uint16_t)delta, __ATOMIC_SEQ_CST);
        Event *e = h_get(hs[i], H_EVENT);
        if (!e) continue;
        e->waiters += delta;
        if (delta > 0) epoch0[i] = e->epoch;
    }
}

uint32_t w32_wait(uint32_t n, const uint32_t *hs, int all, uint32_t timeout_ms) {
    lazy_init();
    if (n == 1 && h_type(hs[0]) == H_SEM) {   // semaphore: lock-free when a count is there (or for a try with no wait)
        if (w32_sem_try(hs[0])) return 0;
        if (timeout_ms == 0) return WAIT_TIMEOUT_;
    }
    static unsigned s_wait_n;
    unsigned wait_id = ++s_wait_n;
    if (wait_id <= 5)   // first ones only: one line every 200 waits filled the log over hours of play
        rt_dlog("w32_wait #%u n=%u all=%d to=%u h0=%x\n", wait_id, n, all, timeout_ms, n > 0 ? hs[0] : 0);
    for (uint32_t i = 0; i < n && i < 4; i++) w32_trace_sync("wait", hs[i], timeout_ms);
    if (n > 64) n = 64;
    uint32_t epoch0[64] = {0};
    uint32_t start = SDL_GetTicks();
    SDL_LockMutex(g_sync);
    register_waiter(n, hs, epoch0, 1);
    for (;;) {
        uint32_t ok = 0, first = 0xffffffffu;
        for (uint32_t i = 0; i < n; i++)
            if (signaled_for(hs[i], epoch0[i])) { ok++; if (first == 0xffffffffu) first = i; }
        // a semaphore count can be taken by a lock-free try between the check and here: then wait again
        if (all ? ok == n : ok > 0 && w32_consume(hs[first])) {
            if (all) for (uint32_t i = 0; i < n; i++) w32_consume(hs[i]);
            register_waiter(n, hs, epoch0, -1);
            SDL_UnlockMutex(g_sync);
            if (wait_id <= 5) rt_dlog("w32_wait #%u -> %u\n", wait_id, all ? 0 : first);
            return all ? 0 : first;
        }
        uint32_t el = SDL_GetTicks() - start;
        if (timeout_ms != 0xffffffffu && el >= timeout_ms) break;
        SDL_CondWaitTimeout(g_sync_cv, g_sync, timeout_ms == 0xffffffffu ? 50 : (timeout_ms - el < 50 ? timeout_ms - el : 50));
    }
    register_waiter(n, hs, epoch0, -1);
    SDL_UnlockMutex(g_sync);
    if (wait_id <= 5) rt_dlog("w32_wait #%u -> TIMEOUT\n", wait_id);
    return WAIT_TIMEOUT_;
}

void w32_trace_sync(const char *what, uint32_t h, uint32_t extra) {
    static int on = -1;
    static SDL_atomic_t n;
    if (on < 0) on = HK_ENV("SH3_TRACE_SYNC") != NULL;
    if (!on || SDL_AtomicAdd(&n, 1) >= 600) return;
    static const char *names[] = {"free", "file", "event", "mutex", "thread", "regkey", "timer", "sem", "mmio", "find", "mapping"};
    int ty = h_type(h);
    rt_dlog("[sync] %-14s h=%x (%s) %x host=%lu\n", what, h, ty >= 0 && ty < 11 ? names[ty] : "?", extra, SDL_ThreadID());
}

void w32_set_last_error(CPU *c, uint32_t e) {
    HOSTFN;
    W32(c->fs_base + 0x34, e);
}

// SH3_VTIME=1 (PC profiling): virtual 30 Hz clock + 10 us per read, so the game never sleeps.
extern unsigned g_frame;
static int vtime(void) { static int v = -1; if (v < 0) v = HK_ENV("SH3_VTIME") != NULL; return v; }
unsigned w32_hz(void) {   // no game logic (30 originally); SH3_HZ=60 to test a 60 Hz loop
    static unsigned h;
    if (!h) { const char *e = HK_ENV("SH3_HZ"); h = e && atoi(e) >= 15 && atoi(e) <= 120 ? (unsigned)atoi(e) : 30; }
    return h;
}
uint64_t w32_qpc(void) {
    static uint64_t calls;
    extern uint64_t g_pause_qpc;
    if (!vtime()) return SDL_GetPerformanceCounter() - g_pause_qpc;
    return ((uint64_t)g_frame * 1000000 / w32_hz() + ++calls * 10) * SDL_GetPerformanceFrequency() / 1000000;
}
// Sleeps until QPC instant t to within ~1 ms (the caller finishes with a busy wait); nothing with the virtual clock.
void w32_sleep_until(uint64_t t) {
    if (vtime()) return;
    uint64_t now = SDL_GetPerformanceCounter(), f = SDL_GetPerformanceFrequency();
    if (t <= now || t - now >= f) return;
    uint32_t ms = (uint32_t)((t - now) * 1000 / f);
    if (ms >= 2) SDL_Delay(ms - 1);
}
uint32_t w32_tick_ms(void) { return (uint32_t)(w32_qpc() * 1000 / SDL_GetPerformanceFrequency()); }   // pause-aware

#ifndef __vita__
#include <dirent.h>
#include <sys/stat.h>
// Windows and the Vita ignore case in file names; Linux does not. Each missing path component is replaced by
// the directory entry that matches it ignoring case (the game asks for "bitmap/HdkSYS00.BMP", the copy has BITMAP/...).
static void host_fix_case(char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return;
    for (char *p = path + 1; ; p++) {
        if (*p != '/' && *p != 0) continue;
        char saved = *p;
        *p = 0;
        if (stat(path, &st) != 0) {
            char *slash = strrchr(path, '/');
            const char *dirpath = slash && slash != path ? path : ".";
            char *name = slash ? slash + 1 : path;
            if (slash) *slash = 0;
            DIR *d = opendir(slash ? dirpath : ".");
            struct dirent *e;
            int found = 0;
            while (d && (e = readdir(d)))
                if (!SDL_strcasecmp(e->d_name, name) && strlen(e->d_name) == strlen(name)) { memcpy(name, e->d_name, strlen(name)); found = 1; break; }
            if (d) closedir(d);
            if (slash) *slash = '/';
            if (!found) { *p = saved; return; }
        }
        *p = saved;
        if (!saved) return;
    }
}
#endif

// Language overlay (the PT-BR fan translation the player installs): files that exist under the overlay folder are
// served from there instead of the game folder. The list is read once at startup, so a lookup is a few strcmp.
#include <dirent.h>
#include <sys/stat.h>
int g_hk_lang;   // 1 when the PT-BR translation is loaded: the port's own texts (hk/i18n.h) follow
static char g_ovl_dir[400];
static char g_ovl_files[64][64];   // relative paths ("BITMAP/HDKCON00.BMP"), original case
static int g_ovl_n;
static void ovl_scan(const char *rel, int depth) {
    char dir[512];
    SDL_snprintf(dir, sizeof dir, "%s%s%s", g_ovl_dir, rel[0] ? "/" : "", rel);
    DIR *d = opendir(dir);
    struct dirent *e;
    while (d && (e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        char r[128], full[640];
        SDL_snprintf(r, sizeof r, "%s%s%s", rel, rel[0] ? "/" : "", e->d_name);
        SDL_snprintf(full, sizeof full, "%s/%s", g_ovl_dir, r);
        struct stat st;
        if (stat(full, &st)) continue;
        if (S_ISDIR(st.st_mode)) { if (depth < 3) ovl_scan(r, depth + 1); }
        else if (g_ovl_n < 64 && strlen(r) < 64) SDL_strlcpy(g_ovl_files[g_ovl_n++], r, 64);
    }
    if (d) closedir(d);
}
void w32_set_lang_overlay(const char *dir) {
    SDL_strlcpy(g_ovl_dir, dir, sizeof g_ovl_dir);
    g_ovl_n = 0;
    g_hk_lang = 1;
    ovl_scan("", 0);
    rt_log("[lang] overlay %s: %d files\n", dir, g_ovl_n);
}

void w32_host_path(const char *g, char *out, size_t n) {
    // NULL guest path: an empty host path, so the file call fails as on Windows instead of crashing.
    if (!g) { if (n) out[0] = 0; return; }
    char tmp[512];
    size_t k = 0;
    for (; *g && k < sizeof tmp - 1; g++) tmp[k++] = *g == '\\' ? '/' : *g;
    tmp[k] = 0;
    const char *p = tmp;
    if (!SDL_strncasecmp(p, "C:/SH3/", 7)) p += 7;
    // The Brazilian Portuguese fan translation's executable (same code, different data) looks for its files under
    // "../Hdkwin/" and "<current dir>/Hdkwin/" (another release's folder layout): map them to the game folder.
    // Strip leading "/", "./" (repeatedly, any order), then one "../Hdkwin/" or "Hdkwin/" prefix.
    for (;;) {
        if (*p == '/') p++;
        else if (p[0] == '.' && p[1] == '/') p += 2;
        else break;
    }
    if (!SDL_strncasecmp(p, "../Hdkwin/", 10)) p += 10;
    else if (!SDL_strncasecmp(p, "Hdkwin/", 7)) p += 7;
    if (p[0] && p[1] == ':') { SDL_snprintf(out, n, "%s", p); return; }
    for (int i = 0; i < g_ovl_n; i++)
        if (!SDL_strcasecmp(g_ovl_files[i], p)) { SDL_snprintf(out, n, "%s/%s", g_ovl_dir, g_ovl_files[i]); return; }
    SDL_snprintf(out, n, "%s/%s", g_game_dir, p);
#ifndef __vita__
    host_fix_case(out);
#endif
}
