// See hitch.h. The file counters (opens, reads, BMP decode) are the g_lt_* ones of kernel32.c.
#include "rt.h"
#include "hitch.h"
#if HK_DEVTOOLS
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int g_hitch;
uint64_t g_hc_t[HC_N], g_hc_n[HC_N], g_hc_b[HC_N], g_hc_max[HC_N];
extern uint64_t g_lt_open_t, g_lt_read_t, g_lt_dec_t, g_lt_read_n, g_lt_read_b, g_lt_open_n, g_t_sleep, g_t_wait;
extern uint64_t g_hc_ram_b, g_hc_ram_n;   // kernel32.c: bytes/reads served from the preload cache or the prefetcher
extern unsigned g_frame;
extern int g_hk_in_stage, g_hk_ws_w;

#define NFILES 24
typedef struct { char name[28]; uint32_t bytes, n, nram; double ms; } FEnt;
static FEnt s_f[NFILES];
static int s_nf, s_nf_over;
static char s_stage[16] = "-";
static double s_thr = 60.0;
static int s_nlist = 6;   // HK_HITCH_FILES: files listed per hitch

const char *hk_hitch_stage(void) { return s_stage; }
void hk_hitch_open(const char *path) {   // "./System/a0_a.HDK" -> stage label "a0_a"
    if (!g_hitch) return;
    const char *s = strrchr(path, '/'); const char *b = strrchr(path, '\\');
    if (b > s) s = b;
    s = s ? s + 1 : path;
    size_t n = strlen(s);
    if (n > 4 && !strcasecmp(s + n - 4, ".hdk") && n - 4 < sizeof s_stage && strncasecmp(s, "user", 4)) { memcpy(s_stage, s, n - 4); s_stage[n - 4] = 0; }
}
void hk_hitch_file(const char *path, uint32_t bytes, double ms, int from_ram) {
    const char *s = strrchr(path, '/');
    s = s ? s + 1 : path;
    for (int i = 0; i < s_nf; i++)
        if (!strncmp(s_f[i].name, s, sizeof s_f[i].name - 1)) { s_f[i].bytes += bytes; s_f[i].n++; s_f[i].ms += ms; s_f[i].nram += from_ram != 0; return; }
    if (s_nf == NFILES) { s_nf_over++; return; }
    FEnt *e = &s_f[s_nf++];
    snprintf(e->name, sizeof e->name, "%s", s);
    e->bytes = bytes; e->n = 1; e->ms = ms; e->nram = from_ram != 0;
}
static int cmp_ms(const void *a, const void *b) { double x = ((const FEnt *)a)->ms, y = ((const FEnt *)b)->ms; return (x < y) - (x > y); }

uint64_t g_hk_plat_upd_t, g_hk_plat_pres_t, g_hk_plat_pre_t, g_hk_plat_clr_t, g_hk_rec_t;   // + time before show_frame (pad, pause check, FPS text), RenderClear+RenderCopy, longest recorder-thread write
   // set by the platform present (port/vita/main.c): UpdateTexture / RenderPresent ticks
static double ms_of(uint64_t t);
void hk_hitch_note(const char *what, uint64_t t0) {
    if (!t0) return;
    double ms = ms_of(SDL_GetPerformanceCounter() - t0);
    if (ms >= 2.0) rt_log("[hitch] fr %u %s took %.1f ms\n", g_frame, what, ms);
}
static double ms_of(uint64_t t) { return t * 1000.0 / (double)SDL_GetPerformanceFrequency(); }
static double s_wall, s_conv, s_plat, s_pause;
void hk_hitch_times(double wall, double conv, double plat, double pause) { s_wall = wall; s_conv = conv; s_plat = plat; s_pause = pause; }
void hk_hitch_frame_end(void) {
    double wall = s_wall, conv = s_conv, plat = s_plat, pause = s_pause;
    static uint64_t snap_t[HC_N], snap_n[HC_N], snap_b[HC_N], snap_sl, snap_wt, snap_rn, snap_rb, snap_ram_b, snap_ram_n;
    static int init;
    if (!init) {
        init = 1;
        const char *e = getenv("HK_HITCH_MS");
        if (e && atof(e) > 0) s_thr = atof(e);
        e = getenv("HK_HITCH_FILES");
        if (e && atoi(e) > 0) s_nlist = atoi(e) < NFILES ? atoi(e) : NFILES;
    }
    if (wall - pause > s_thr) {
        double game = wall - pause - conv - plat;
        char b[1100]; int o = 0;
        static const char *const nm[HC_N] = {"surf", "blt", "lock", "dc", "pal", "snd", "heap", "write", "disk"};
        o += snprintf(b + o, sizeof b - o, "[hitch] fr %u stage %s w=%d: wall %.1f game %.1f conv %.1f plat %.1f pause %.1f | open %llu %.1f ms, read %llu %.2f MB %.1f ms (ram %llu rd %.2f MB), bmp-dec %.1f ms, sleep %.1f wait %.1f",
                      g_frame, s_stage, g_hk_ws_w, wall, game, conv, plat, pause, (unsigned long long)g_lt_open_n, ms_of(g_lt_open_t),
                      (unsigned long long)g_lt_read_n, g_lt_read_b / 1048576.0, ms_of(g_lt_read_t), (unsigned long long)(g_hc_ram_n - snap_ram_n),
                      (g_hc_ram_b - snap_ram_b) / 1048576.0, ms_of(g_lt_dec_t), ms_of(g_t_sleep - snap_sl), ms_of(g_t_wait - snap_wt));
        for (int i = 0; i < HC_N; i++) {
            uint64_t n = g_hc_n[i] - snap_n[i];
            if (!n) continue;
            o += snprintf(b + o, sizeof b - o, ", %s %llu %.1f ms", nm[i], (unsigned long long)n, ms_of(g_hc_t[i] - snap_t[i]));
            uint64_t by = g_hc_b[i] - snap_b[i];
            if (by) o += snprintf(b + o, sizeof b - o, " %.0f KB (max %.0f)", by / 1024.0, g_hc_max[i] / 1024.0);
        }
        extern uint64_t g_log_flush_t, g_log_flush_b, g_hk_plat_upd_t, g_hk_plat_pres_t;
        // plat split (UpdateTexture / RenderPresent) and the log thread's longest flush while this frame ran
        o += snprintf(b + o, sizeof b - o, ", plat-upd %.1f pres %.1f, logflush %.1f ms %u KB", ms_of(g_hk_plat_upd_t), ms_of(g_hk_plat_pres_t),
                      ms_of(g_log_flush_t), (unsigned)(g_log_flush_b >> 10));
        {   // where the rest of plat went: before show_frame (pad, FPS text), RenderClear+RenderCopy, anything unaccounted; the recorder thread's longest write
            extern uint64_t g_hk_plat_pre_t, g_hk_plat_clr_t, g_hk_rec_t;
            double acc = ms_of(g_hk_plat_pre_t) + ms_of(g_hk_plat_upd_t) + ms_of(g_hk_plat_clr_t) + ms_of(g_hk_plat_pres_t);
            o += snprintf(b + o, sizeof b - o, ", plat-pre %.1f clr %.1f unacc %.1f, rec-write %.1f ms", ms_of(g_hk_plat_pre_t), ms_of(g_hk_plat_clr_t), plat - acc, ms_of(g_hk_rec_t));
        }
        { extern double g_hk_dlg_build_ms; if (g_hk_dlg_build_ms > 0.5) o += snprintf(b + o, sizeof b - o, ", dlg-build %.1f ms", g_hk_dlg_build_ms); }
        rt_log("%s\n", b);
        qsort(s_f, (size_t)s_nf, sizeof s_f[0], cmp_ms);
        for (int i = 0; i < s_nf && i < s_nlist; i++)
            rt_log("[hitch]   file %s: %u reads (%u from RAM) %u KB %.1f ms\n", s_f[i].name, s_f[i].n, s_f[i].nram, s_f[i].bytes >> 10, s_f[i].ms);
        if (s_nf > s_nlist || s_nf_over) rt_log("[hitch]   (%d files read this frame%s)\n", s_nf + s_nf_over, "");
    }
    // counters restart every frame
    { extern double g_hk_dlg_build_ms; g_hk_dlg_build_ms = 0; }
    { extern uint64_t g_log_flush_t, g_log_flush_b, g_hk_rec_t; g_log_flush_t = 0; g_log_flush_b = 0; g_hk_rec_t = 0; }
    for (int i = 0; i < HC_N; i++) { snap_t[i] = g_hc_t[i]; snap_n[i] = g_hc_n[i]; snap_b[i] = g_hc_b[i]; g_hc_max[i] = 0; }
    snap_sl = g_t_sleep; snap_wt = g_t_wait; snap_ram_b = g_hc_ram_b; snap_ram_n = g_hc_ram_n; (void)snap_rn; (void)snap_rb;
    s_nf = s_nf_over = 0;
}

#endif   // HK_DEVTOOLS
