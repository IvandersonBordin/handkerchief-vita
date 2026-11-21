// Neutral definitions for symbols the sh3vita-derived Win32 layer expects from Silent Hill 3 features
// (virtual puzzle cursor, DirectShow movies, CPU profiler). Handkerchief does not use them.
#include "rt.h"

unsigned g_frame;   // presented frame counter (read by file and timer diagnostics)

int rt_cpu_profile_thread(const char *label) { (void)label; return 0; }
int movie_cocreate(CPU *c, uint32_t clsid, uint32_t riid, uint32_t out, uint32_t *hr) {
    (void)c; (void)clsid; (void)riid; (void)out; (void)hr;
    return 0;
}
int sh3_vcursor_on(void) { return 0; }
void sh3_vcursor_pixels(int *px, int *py) { *px = 0; *py = 0; }
int sh3_vcursor_click(void) { return 0; }
