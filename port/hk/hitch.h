// Frame hitch log (cfg `hitch=1` on the Vita, HK_HITCH=1 on the PC): for any presented frame whose wall time exceeds
// 60 ms (HK_HITCH_MS / cfg hitch_ms overrides), one "[hitch]" line gives the breakdown by category and the stage file
// in use. Off by default; when off every hook below is one load and one branch.
// Categories count calls, time (performance-counter ticks) and bytes. Time inside a category is wall time of the
// call on any thread (the game's decode threads are rare; the game itself is single threaded), so categories can
// overlap `game` when something runs in parallel (preload thread, present thread).
#pragma once
#include <stdint.h>
#include <SDL.h>
#include "hkdev.h"
#if !HK_DEVTOOLS
// Player build: every hook is an empty inline / constant, hitch.c is not compiled.
#define HC_WRAP(cat, name) static void name(CPU *c)
#define HC_K(cat, mod, name) void imp_##mod##_##name(CPU *c)
#define g_hitch 0
static double g_hk_dlg_build_ms __attribute__((unused));
static inline uint64_t hc_t0(void) { return 0; }
static inline void hc_end(int cat, uint64_t t0, uint64_t bytes) { (void)cat; (void)t0; (void)bytes; }
static inline void hk_hitch_note(const char *what, uint64_t t0) { (void)what; (void)t0; }
enum { HC_SURF, HC_BLT, HC_LOCK, HC_DC, HC_PAL, HC_SND, HC_HEAP, HC_WRITE, HC_DISK, HC_N };
static inline void hk_hitch_file(const char *path, uint32_t bytes, double ms, int from_ram) { (void)path; (void)bytes; (void)ms; (void)from_ram; }
static inline void hk_hitch_open(const char *path) { (void)path; }
static inline void hk_hitch_times(double a, double b, double c, double d) { (void)a; (void)b; (void)c; (void)d; }
static inline void hk_hitch_frame_end(void) {}
#else
extern double g_hk_dlg_build_ms;
enum { HC_SURF, HC_BLT, HC_LOCK, HC_DC, HC_PAL, HC_SND, HC_HEAP, HC_WRITE, HC_DISK, HC_N };
extern int g_hitch;
extern uint64_t g_hc_t[HC_N], g_hc_n[HC_N], g_hc_b[HC_N], g_hc_max[HC_N];
static inline uint64_t hc_t0(void) { return g_hitch ? SDL_GetPerformanceCounter() : 0; }
static inline void hc_end(int cat, uint64_t t0, uint64_t bytes) {
    if (!t0) return;
    uint64_t d = SDL_GetPerformanceCounter() - t0;
    __atomic_fetch_add(&g_hc_t[cat], d, __ATOMIC_RELAXED);
    __atomic_fetch_add(&g_hc_n[cat], 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&g_hc_b[cat], bytes, __ATOMIC_RELAXED);
    if (bytes > g_hc_max[cat]) g_hc_max[cat] = bytes;
}
void hk_hitch_note(const char *what, uint64_t t0);   // logs one slow (>= 2 ms) file operation with the frame number
// Wraps a guest-callable native `name(CPU *c)`: HC_WRAP(HC_SURF, dd_create_surface) { body }
#define HC_WRAP(cat, name) static void name##_hc(CPU *c); \
    static void name(CPU *c) { uint64_t t_ = hc_t0(); name##_hc(c); hc_end(cat, t_, 0); } \
    static void name##_hc(CPU *c)
// Same for an imp_<mod>_<name> import: HC_K(HC_WRITE, KERNEL32, WriteFile) { body }
#define HC_K(cat, mod, name) static void imp_##mod##_##name##_hc(CPU *c); \
    void imp_##mod##_##name(CPU *c) { uint64_t t_ = hc_t0(); imp_##mod##_##name##_hc(c); hc_end(cat, t_, 0); hk_hitch_note(#name, t_); } \
    static void imp_##mod##_##name##_hc(CPU *c)
// Per-frame list of the files the game opened on its own thread (file name, bytes read, source).
void hk_hitch_file(const char *path, uint32_t bytes, double ms, int from_ram);
void hk_hitch_open(const char *path);   // CreateFileA of a game file: remembers the stage label (System/<x>.HDK)
void hk_hitch_times(double wall_ms, double conv_ms, double plat_ms, double pause_ms);   // present_primary: this frame's split
void hk_hitch_frame_end(void);   // hk_load_trace_frame: logs the frame if it was slow, restarts the counters
#endif
