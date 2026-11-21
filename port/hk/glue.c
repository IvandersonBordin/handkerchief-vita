// Neutral definitions for symbols the sh3vita-derived Win32 layer expects from Silent Hill 3 features
// (virtual puzzle cursor, DirectShow movies, CPU profiler). Handkerchief does not use them.
#include "rt.h"

unsigned g_frame;   // presented frame counter (read by file and timer diagnostics)
uint64_t g_pause_qpc;   // performance-counter ticks spent in the port's pause screen (Vita Start)

int rt_cpu_profile_thread(const char *label) { (void)label; return 0; }
int movie_cocreate(CPU *c, uint32_t clsid, uint32_t riid, uint32_t out, uint32_t *hr) {
    (void)c; (void)clsid; (void)riid; (void)out; (void)hr;
    return 0;
}
int sh3_vcursor_on(void) { return 0; }
void sh3_vcursor_pixels(int *px, int *py) { *px = 0; *py = 0; }
int sh3_vcursor_click(void) { return 0; }

// Load-time data patches of HDKWIN.EXE (guest memory, after rt_load_exe).
// [0x4A88EC] is the CRT small-block-heap threshold (0x1E0; malloc at 0x45E2E0 sub-allocates blocks below it).
// With sound on, that heap handed out a live sound object a second time (0x45CB29, then 0x459D13: docs/status.md).
// A zero threshold sends every allocation to HeapAlloc, i.e. the port's heap, which tracks live blocks.
void hk_patch_data(void) {
    uint8_t *const mb = g_mem;
    if (R32(0x4A88ECu) == 0x1E0u) W32(0x4A88ECu, 0);
    else rt_log("[patch] unexpected small-block threshold %08x, left alone\n", R32(0x4A88ECu));
}
