// Guest heap: TLSF over the zone [G_HEAP_BASE, end of g_mem), guest addresses.
#include "rt.h"
#include "third_party/tlsf/tlsf.h"
#include <SDL.h>
#include <stdlib.h>
#include "hk/hitch.h"

static tlsf_t g_tlsf;
static SDL_mutex *g_heap_lock;

typedef struct HeapFreeStats {
    size_t total, largest;
    unsigned blocks;
} HeapFreeStats;
static void count_free_block(void *ptr, size_t bytes, int used, void *arg) {
    (void)ptr;
    if (used) return;
    HeapFreeStats *stats = arg;
    stats->total += bytes;
    if (bytes > stats->largest) stats->largest = bytes;
    stats->blocks++;
}
static void log_heap_block(void *ptr, size_t bytes, int used, void *arg) {
    (void)arg;
    if (!used || bytes >= 65536)
        rt_dlog("[heap-map] address=%08x size=%llu used=%d\n", GADDR(ptr),
               (unsigned long long)bytes, used);
}

// Live guest-heap blocks grouped by exact size (HK_MEMLOG=2 logs it per cycle; compare two reports to see what accumulates).
typedef struct { size_t size; unsigned n; } SizeCount;
static SizeCount g_hist[1024];
static unsigned g_hist_n, g_hist_overflow;
static void hist_block(void *ptr, size_t bytes, int used, void *arg) {
    (void)ptr; (void)arg;
    if (!used) return;
    for (unsigned i = 0; i < g_hist_n; i++) if (g_hist[i].size == bytes) { g_hist[i].n++; return; }
    if (g_hist_n < 1024) g_hist[g_hist_n++] = (SizeCount){bytes, 1};
    else g_hist_overflow++;
}
extern uint64_t g_heap_used, g_heap_peak;
// Free-space summary for the memory log: total free, largest free block, number of free blocks, live bytes.
void gheap_stats(GHeapStats *st) {
    HeapFreeStats fs = {0};
    memset(st, 0, sizeof *st);
    if (!g_heap_lock) return;
    SDL_LockMutex(g_heap_lock);
    tlsf_walk_pool(tlsf_get_pool(g_tlsf), count_free_block, &fs);
    st->used_bytes = g_heap_used; st->peak_bytes = g_heap_peak;
    SDL_UnlockMutex(g_heap_lock);
    st->free_total = fs.total; st->free_largest = fs.largest; st->free_blocks = fs.blocks;
}
void gheap_report(void) {
    if (!g_heap_lock) return;
    SDL_LockMutex(g_heap_lock);
    g_hist_n = 0; g_hist_overflow = 0;
    tlsf_walk_pool(tlsf_get_pool(g_tlsf), hist_block, NULL);
    SDL_UnlockMutex(g_heap_lock);
    unsigned long long total = 0, blocks = 0;
    for (unsigned i = 0; i < g_hist_n; i++) { total += (unsigned long long)g_hist[i].size * g_hist[i].n; blocks += g_hist[i].n; }
    rt_dlog("[gheap] live %llu KB in %llu blocks, %u distinct sizes (+%u uncounted)\n", total >> 10, blocks, g_hist_n, g_hist_overflow);
    for (int k = 0; k < 18; k++) {
        unsigned best = 0; unsigned long long bb = 0;
        for (unsigned i = 0; i < g_hist_n; i++) { unsigned long long v = (unsigned long long)g_hist[i].size * g_hist[i].n; if (v > bb) { bb = v; best = i; } }
        if (!bb) break;
        rt_dlog("[gheap] size=%llu n=%u total=%llu KB\n", (unsigned long long)g_hist[best].size, g_hist[best].n, bb >> 10);
        g_hist[best].n = 0;
    }
}


// ---------- allocation-site accounting (HK_TRACE_HEAP=1, hkport.cfg heaptrace=1) ----------
// Every live block is attributed to the code that asked for it: for the import entry points that know the guest CPU
// (HeapAlloc, GlobalAlloc, VirtualAlloc ...) the guest return addresses (the import's caller plus two callers up the
// EBP chain), for host-side allocations (ddraw/gdi/dsound/mmio ...) the host return address of gheap_alloc. The log
// prints &gheap_alloc next to them so a Vita build can be resolved offline (addr2line on the ELF: ra - &gheap_alloc +
// the symbol's link address). Cost: a hash update per allocation and free; nothing is recorded when off.
// gheap_site_report (called by the [mem] log) prints the 10 sites that grew most since the previous report.
#if HK_DEVTOOLS
int g_hk_heaptrace;
#endif
#define HS_MAX 2048
typedef struct { uint32_t k[3]; uintptr_t host; uint64_t live, prev; uint32_t n, total_n; int used; } HeapSite;
static HeapSite g_hs[HS_MAX];
static unsigned g_hs_n;
typedef struct { uint32_t a, size; uint16_t site; } HeapLive;   // a == 0 empty, 1 deleted
static HeapLive *g_hl;
static uint32_t g_hl_cap, g_hl_used;
static _Thread_local uint32_t t_site[3];
static _Thread_local int t_site_set;
// Called by an import handler right before it allocates: records who is calling (ESP -> return address, then the EBP chain).
void gheap_site(CPU *c) {
    HOSTFN;
    if (!g_hk_heaptrace) return;
    uint32_t fp = c->ebp;
    t_site[0] = R32(c->esp);
    for (int i = 1; i < 3; i++) {
        uint32_t ra = 0;
        if (fp >= 0x1000 && fp + 8 < g_mem_size && !(fp & 3)) { ra = R32(fp + 4); fp = R32(fp); } else fp = 0;
        t_site[i] = ra;
    }
    t_site_set = 1;
}
static unsigned site_of(uintptr_t host_ra) {
    uint32_t k[3] = {0, 0, 0};
    uintptr_t h = 0;
    if (t_site_set) { memcpy(k, t_site, sizeof k); t_site_set = 0; } else h = host_ra;
    uint32_t hash = (k[0] * 2654435761u) ^ (k[1] * 40503u) ^ (k[2] * 69069u) ^ ((uint32_t)h * 2246822519u);
    for (unsigned j = 0; j < HS_MAX; j++) {
        HeapSite *s = &g_hs[(hash + j) & (HS_MAX - 1)];
        if (!s->used) { s->used = 1; memcpy(s->k, k, sizeof k); s->host = h; g_hs_n++; return (unsigned)(s - g_hs); }
        if (s->host == h && !memcmp(s->k, k, sizeof k)) return (unsigned)(s - g_hs);
    }
    return 0;   // table full: lumped into slot 0
}
static void hl_grow(void) {
    uint32_t ncap = g_hl_cap ? g_hl_cap * 2 : 1u << 16;
    HeapLive *n = calloc(ncap, sizeof *n);
    if (!n) return;
    g_hl_used = 0;
    for (uint32_t i = 0; i < g_hl_cap; i++)
        if (g_hl[i].a > 1) {
            for (uint32_t j = (g_hl[i].a >> 2) & (ncap - 1);; j = (j + 1) & (ncap - 1)) if (!n[j].a) { n[j] = g_hl[i]; g_hl_used++; break; }
        }
    free(g_hl); g_hl = n; g_hl_cap = ncap;
}
static void site_add(void *p, uintptr_t host_ra) {   // heap lock held
    if (!g_hk_heaptrace || !p) return;
    if ((g_hl_used + 1) * 2 > g_hl_cap) hl_grow();
    if (!g_hl_cap) return;
    uint32_t a = GADDR(p), size = (uint32_t)tlsf_block_size(p);
    unsigned si = site_of(host_ra);
    HeapSite *s = &g_hs[si];
    s->live += size; s->n++; s->total_n++;
    for (uint32_t j = (a >> 2) & (g_hl_cap - 1);; j = (j + 1) & (g_hl_cap - 1))
        if (g_hl[j].a <= 1) { if (!g_hl[j].a) g_hl_used++; g_hl[j].a = a; g_hl[j].size = size; g_hl[j].site = (uint16_t)si; return; }
}
static void site_del(void *p) {   // heap lock held, before tlsf_free
    if (!g_hk_heaptrace || !p || !g_hl_cap) return;
    uint32_t a = GADDR(p);
    for (uint32_t j = (a >> 2) & (g_hl_cap - 1); g_hl[j].a; j = (j + 1) & (g_hl_cap - 1))
        if (g_hl[j].a == a) { HeapSite *s = &g_hs[g_hl[j].site]; s->live -= g_hl[j].size; s->n--; g_hl[j].a = 1; return; }
}
void gheap_site_report(const char *tag, int cycle) {
    if (!g_hk_heaptrace || !g_heap_lock) return;
    SDL_LockMutex(g_heap_lock);
    unsigned idx[HS_MAX], n = 0;
    for (unsigned i = 0; i < HS_MAX; i++) if (g_hs[i].live != g_hs[i].prev) idx[n++] = i;
    for (unsigned a = 0; a < n && a < 10; a++) {   // selection sort by growth, top 10
        unsigned b = a;
        for (unsigned i = a + 1; i < n; i++) if ((int64_t)(g_hs[idx[i]].live - g_hs[idx[i]].prev) > (int64_t)(g_hs[idx[b]].live - g_hs[idx[b]].prev)) b = i;
        unsigned t = idx[a]; idx[a] = idx[b]; idx[b] = t;
    }
    rt_dlog("[hsite] %s cyc=%d sites=%u &gheap_alloc=%p\n", tag, cycle, g_hs_n, (void *)gheap_alloc);
    for (unsigned a = 0; a < n && a < 10; a++) {
        HeapSite *s = &g_hs[idx[a]];
        long long d = (long long)(int64_t)(s->live - s->prev) / 1024;
        if (d <= 0) break;
        if (s->host) rt_dlog("[hsite]   host ra=%p live=%lluK n=%u (+%lldK) total_n=%u\n", (void *)s->host, (unsigned long long)(s->live >> 10), s->n, d, s->total_n);
        else rt_dlog("[hsite]   guest ra=%08x<%08x<%08x live=%lluK n=%u (+%lldK) total_n=%u\n", s->k[0], s->k[1], s->k[2], (unsigned long long)(s->live >> 10), s->n, d, s->total_n);
    }
    for (unsigned i = 0; i < HS_MAX; i++) g_hs[i].prev = g_hs[i].live;
    SDL_UnlockMutex(g_heap_lock);
}

// Live-block bitmap granule. TLSF aligns blocks to sizeof(size_t): 8 bytes on the 64-bit PC, but only 4 on the 32-bit
// Vita. The bitmap used one bit per 8 bytes, so a block at an address = 4 (mod 8) was never marked live: every free of
// it was refused as "non-allocated" (and its neighbour's bit could be cleared by mistake) and the block leaked for the
// whole run (~4 MB per stage load: bitmaps, surfaces, tables). heapfix=1 (default; HK_HEAPFIX=0 / cfg heapfix=0 turns
// it off for A/B runs) uses one bit per 4 bytes.
int g_hk_heapfix = 1;
static unsigned g_live_shift = 2;
static uint8_t *g_live;   // see live_set below
void gheap_init(uint32_t base, uint32_t size) {
    g_tlsf = tlsf_create_with_pool(g_mem + base, size);
    if (getenv("HK_HEAPFIX")) g_hk_heapfix = atoi(getenv("HK_HEAPFIX"));   // hkport.cfg heapfix= is passed on as this variable
#if HK_DEVTOOLS
    if (getenv("HK_TRACE_HEAP")) g_hk_heaptrace = 1;                       // likewise heaptrace=1
#endif
    g_live_shift = g_hk_heapfix ? 2 : 3;
    g_live = calloc(((uint64_t)base + size) / ((8u << g_live_shift)) + 2, 1);
    g_heap_lock = SDL_CreateMutex();
}

static uint32_t to_g(void *p) { return p ? GADDR(p) : 0; }

// Live block starts (one bit per 8-byte granule, TLSF's alignment). Windows' HeapFree fails on a block that is not
// allocated instead of corrupting the heap; HDKWIN.EXE frees one block twice while loading the title screen, so a
// free of a non-live address is logged and ignored rather than handed to TLSF.
static void live_set(void *p, int on) {
    if (!p || !g_live) return;
    uint32_t i = GADDR(p) >> g_live_shift;
    if (on) g_live[i >> 3] |= (uint8_t)(1u << (i & 7)); else g_live[i >> 3] &= (uint8_t)~(1u << (i & 7));
}
static int live_get(uint32_t a) {
    uint32_t i = a >> g_live_shift;
    return (a & ((1u << g_live_shift) - 1)) == 0 && g_live && (g_live[i >> 3] >> (i & 7)) & 1;
}
uint64_t g_heap_used, g_heap_peak;
uint64_t gheap_used_snapshot(void) {
    if (!g_heap_lock) return 0;
    SDL_LockMutex(g_heap_lock);
    uint64_t used = g_heap_used;
    SDL_UnlockMutex(g_heap_lock);
    return used;
}
static void track(void *p, int sign) {
    if (!p) return;
    size_t n = tlsf_block_size(p);
    if (sign > 0) { g_heap_used += n; if (g_heap_used > g_heap_peak) g_heap_peak = g_heap_used; }
    else g_heap_used -= n;
}

static void host_trace(uint32_t size, void *ra) {
    static int on = -1;
    if (on < 0) on = HK_ENV("SH3_TRACE_HEAP") != NULL;
    if (on && size >= (1u << 20)) rt_dlog("[heap-host] %u KB from %p\n", size >> 10, ra);
}

// A request larger than the whole guest memory fails at once, as GlobalAlloc/HeapAlloc/malloc do on Windows. TLSF
// cannot be trusted with it on a 32-bit host: adjust_request_size() rounds the size up with align_up(), which wraps
// 0xFFFFFFFF to 0 when size_t is 32 bits (the Vita), and then hands out a minimum-size block. HDKWIN.EXE relies on
// this failure: its file loader 0x45BDFC checks CreateFileA's result against 0 instead of INVALID_HANDLE_VALUE, so a
// missing WAV (12 of the names it asks for are not in the game data: A_BOMB_1, A_MGUN_0/1, B_LSER_0, C_BUND_0, F_TANK,
// I_NOT, I_USE, J_SCRM_0/1, L_INVI ...) goes on to GetFileSize(-1) = 0xFFFFFFFF and GlobalAlloc(0x2002, 0xFFFFFFFF)
// at 0x45BE51. On Windows (and the 64-bit PC build) that allocation fails and the voice is never loaded. On the Vita
// it "succeeded": the voice loader 0x45C4C1 then read the data size and WAVEFORMATEX from uninitialised stack (its
// mmioDescend calls on the bogus memory file fail unchecked) and created a 2.2 MB sound buffer of zeros in a garbage
// format, mixed as 8-bit data: a full-scale DC voice (console log: six copies of a 571344-frame buffer, each at
// exactly gain x master, in the a0 house explosion and the bazooka fights).
static int too_big(uint32_t size) { return size > g_mem_size; }

static uint32_t heap_alloc(uint32_t size, void *ra);
__attribute__((noinline)) uint32_t gheap_alloc(uint32_t size) {
    uint64_t t = hc_t0();
    uint32_t a = heap_alloc(size, __builtin_return_address(0));
    hc_end(HC_HEAP, t, size);
    return a;
}
static uint32_t heap_alloc(uint32_t size, void *ra) {
    host_trace(size, ra);
    if (too_big(size)) {
        if (RT_TRACE("HK_TRACE_HEAP")) rt_dlog("[heap] allocation of %u bytes refused (larger than guest memory)\n", size);
        return 0;
    }
    HeapFreeStats free_stats = {0};
    SDL_LockMutex(g_heap_lock);
    void *p = tlsf_malloc(g_tlsf, size ? size : 1);
    track(p, 1);
    live_set(p, 1);
    site_add(p, (uintptr_t)ra);
    if (!p) {
        tlsf_walk_pool(tlsf_get_pool(g_tlsf), count_free_block, &free_stats);
        /* Diagnostic only: capture the first fragmented pool on demand.*/
        static int map_dumped;
        if (!map_dumped && HK_ENV("SH3_HEAP_MAP")) {
            map_dumped = 1;
            tlsf_walk_pool(tlsf_get_pool(g_tlsf), log_heap_block, NULL);
        }
    }
    SDL_UnlockMutex(g_heap_lock);
    if (!p && (size < 0xF0000000u || RT_TRACE("HK_TRACE_HEAP"))) rt_log("[heap] allocation of %u bytes failed: used %llu MB, free %llu KB, largest block %llu KB in %u blocks\n",
                   size, (unsigned long long)(g_heap_used >> 20),
                   (unsigned long long)(free_stats.total >> 10),
                   (unsigned long long)(free_stats.largest >> 10), free_stats.blocks);
    return to_g(p);
}

uint32_t gheap_calloc(uint32_t size) {
    host_trace(size | 1u, __builtin_return_address(0)); // |1: marks the call via calloc in the trace
    uint64_t t = hc_t0();
    uint32_t a = heap_alloc(size, __builtin_return_address(0));
    if (a) memset(g_mem + a, 0, size);
    hc_end(HC_HEAP, t, size);
    return a;
}

uint32_t gheap_alloc_aligned(uint32_t size, uint32_t align) {
    host_trace(size, __builtin_return_address(0));
    if (too_big(size)) return 0;
    SDL_LockMutex(g_heap_lock);
    void *p = tlsf_memalign(g_tlsf, align, size ? size : 1);
    track(p, 1);
    live_set(p, 1);
    site_add(p, (uintptr_t)__builtin_return_address(0));
    SDL_UnlockMutex(g_heap_lock);
    return to_g(p);
}

void gheap_free(uint32_t a) {
    if (!a) return;
    SDL_LockMutex(g_heap_lock);
    if (!live_get(a)) {
        SDL_UnlockMutex(g_heap_lock);
        static int n;
        if (n++ < 20) rt_log("[heap] free of non-allocated 0x%08x ignored\n", a);
        return;
    }
    track(g_mem + a, -1);
    live_set(g_mem + a, 0);
    site_del(g_mem + a);
    tlsf_free(g_tlsf, g_mem + a);
    SDL_UnlockMutex(g_heap_lock);
}

uint32_t gheap_realloc(uint32_t a, uint32_t size) {
    if (too_big(size)) return 0;   // the block is left as it is, like a failed HeapReAlloc
    SDL_LockMutex(g_heap_lock);
    if (a && !live_get(a)) { SDL_UnlockMutex(g_heap_lock); rt_log("[heap] realloc of non-allocated 0x%08x\n", a); return 0; }
    if (a) { track(g_mem + a, -1); live_set(g_mem + a, 0); site_del(g_mem + a); }
    void *p = tlsf_realloc(g_tlsf, a ? g_mem + a : NULL, size);
    live_set(p ? p : (a && size ? g_mem + a : NULL), 1);
    /* TLSF frees the original block and returns NULL for size zero.*/
    track(p ? p : (a && size ? g_mem + a : NULL), 1);
    site_add(p ? p : (a && size ? g_mem + a : NULL), (uintptr_t)__builtin_return_address(0));
    SDL_UnlockMutex(g_heap_lock);
    return to_g(p);
}

uint32_t gheap_size(uint32_t a) {
    return a ? (uint32_t)tlsf_block_size(g_mem + a) : 0;
}

uint32_t gstrdup(const char *s) {
    uint32_t n = (uint32_t)strlen(s) + 1, a = gheap_alloc(n);
    memcpy(g_mem + a, s, n);
    return a;
}
