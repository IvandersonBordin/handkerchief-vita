// Memory log ("[mem]" lines) and the in-process stress driver (HK_STRESS) of the recompiled game.
// HK_MEMLOG=1 (hkport.cfg memlog=1 on the Vita): one [mem] line per cycle (stress) or every 600 presented frames.
// HK_MEMLOG=2 also logs the guest heap's live-block histogram ("[gheap]"). Both are test aids: nothing is on by
// default, and nothing here changes what the game does.
// HK_STRESS=<kind>:<n> drives the game's own title/stage/dialog paths n times with injected keys and menu commands:
//   area       Stop This Game (40061) + NEW GAME again, same stage              (stage unload + load)
//   territory  the same, alternating the stage between HK_STRESS_LABELS (default a0,c0)
//   save       Continue slot 1, then F2 save into slot 2, Stop, Continue and load slot 2 again
//   menus      in a stage: Stop-confirm, save dialog and menu bar opened and cancelled; title Continue dialog cancelled
#include "rt.h"
#include "win32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __vita__
#include <malloc.h>
#else
#include <dirent.h>
#include <unistd.h>
#ifdef __GLIBC__
#include <malloc.h>
#endif
#endif

#define HWND_MAIN_DLG 0x00010010u   // the game window (port/hk/dialog.c)
typedef struct { int buffers, notifies, playing, dups, ghosts, top_slot; uint64_t sample_bytes; unsigned dead_guest_objs; } HkDsStats;
void hk_dd_stats(int *surfaces, uint64_t *bytes, int *others, int *peak_slot);
void hk_ds_stats(HkDsStats *o);
void hk_gdi_stats(int *bitmaps, uint64_t *bitmap_bytes, int *fonts, int *dcs);
int hk_dlg_count(void);
int hk_dlg_top_res(void);
int hk_menu_shown(void);
int hk_dlg_active(void);
int hk_cs_count(void);
void hk_load_game_request(void);   // port/hk/dialog.c
int hk_wav_miss_count(void);
int h_stats(int counts[], int n);
extern unsigned g_hk_closed_unfreed[16];
extern int g_hk_in_stage;
extern unsigned g_frame;
int g_hk_memlog;   // 0 off, 1 [mem] lines, 2 + heap histogram (HK_MEMLOG, or memlog= in hkport.cfg)

static unsigned long long rss_kb(void) {   // resident set; 0 where /proc is missing (the Vita)
    unsigned long long pages = 0, res = 0;
    FILE *f = fopen("/proc/self/statm", "r");
    if (!f) return 0;
    if (fscanf(f, "%llu %llu", &pages, &res) != 2) res = 0;
    fclose(f);
    return res * 4;
}
static int fd_count(void) {
#ifdef __vita__
    return -1;
#else
    int n = 0;
    DIR *d = opendir("/proc/self/fd");
    if (!d) return -1;
    while (readdir(d)) n++;
    closedir(d);
    return n - 3;   // ".", ".." and the directory handle itself
#endif
}
static int thread_count(void) {
    int n = -1;
    FILE *f = fopen("/proc/self/status", "r");
    char line[128];
    while (f && fgets(line, sizeof line, f)) if (sscanf(line, "Threads: %d", &n) == 1) break;
    if (f) fclose(f);
    return n;
}

void hk_mem_log(const char *tag, int cycle) {
    if (!g_hk_memlog) return;
    GHeapStats g;
    gheap_stats(&g);
    int surf, others, slot, bmp, fonts, dcs, h[16];
    uint64_t surf_b, bmp_b;
    hk_dd_stats(&surf, &surf_b, &others, &slot);
    hk_gdi_stats(&bmp, &bmp_b, &fonts, &dcs);
    HkDsStats ds;
    hk_ds_stats(&ds);
    int htot = h_stats(h, 16);
    unsigned long long mall = 0;
#ifdef __vita__
    { struct mallinfo mi = mallinfo(); mall = (unsigned long long)mi.uordblks >> 10; }
#elif defined(__GLIBC__) && (__GLIBC__ > 2 || __GLIBC_MINOR__ >= 33)
    { struct mallinfo2 mi = mallinfo2(); mall = (unsigned long long)mi.uordblks >> 10; }
#endif
    rt_log("[mem] %s cyc=%d fr=%u | gfree=%lluK glargest=%lluK gblocks=%u gused=%lluK gpeak=%lluK | dd surf=%d %lluK other=%d slot=%d | "
           "ds buf=%d nt=%d play=%d dup=%d ghost=%d smp=%lluK dead=%u slot=%d miss=%d | h tot=%d file=%d ev=%d thr=%d sem=%d find=%d map=%d mtx=%d key=%d tmr=%d mmio=%d "
           "unfreed ev=%u sem=%u thr=%u cs=%d | gdi bmp=%d %lluK font=%d dc=%d | dlg=%d | rss=%lluK malloc=%lluK fd=%d thr=%d\n",
           tag, cycle, g_frame, (unsigned long long)(g.free_total >> 10), (unsigned long long)(g.free_largest >> 10), g.free_blocks,
           (unsigned long long)(g.used_bytes >> 10), (unsigned long long)(g.peak_bytes >> 10),
           surf, (unsigned long long)(surf_b >> 10), others, slot,
           ds.buffers, ds.notifies, ds.playing, ds.dups, ds.ghosts, (unsigned long long)(ds.sample_bytes >> 10), ds.dead_guest_objs, ds.top_slot, hk_wav_miss_count(),
           htot, h[H_FILE], h[H_EVENT], h[H_THREAD], h[H_SEM], h[H_FIND], h[H_MAPPING], h[H_MUTEX], h[H_REGKEY], h[H_TIMER], h[H_MMIO],
           g_hk_closed_unfreed[H_EVENT], g_hk_closed_unfreed[H_SEM], g_hk_closed_unfreed[H_THREAD], hk_cs_count(),
           bmp, (unsigned long long)(bmp_b >> 10), fonts, dcs, hk_dlg_count(), rss_kb(), mall, fd_count(), thread_count());
    if (g_hk_memlog >= 2) gheap_report();
}

// ---------- stress driver ----------
// A script is a list of steps, run in order and repeated: "A<frame>" wait for an absolute frame, "W<n>" wait n frames,
// "K<hex vk>:<n>" hold a key n frames, "C<id>" post WM_COMMAND, "S" wait until a stage has been drawn for 200 frames (the stage music loads ~15 frames after it starts),
// "T" wait for the title (no stage, no dialog, 90 frames), "D<res>" wait for dialog <res>, "N" wait until no dialog,
// "M" mark: log [mem] and count one cycle. Steps are separated by spaces. "|" separates the prologue (run once).
#define MAX_STEPS 64
typedef struct { char op; int a, b; } Step;
static Step g_steps[MAX_STEPS];
static int g_nsteps, g_loop_from, g_pc, g_cycles, g_total, g_active, g_kind;
static unsigned g_step_t0, g_stable;
static int g_key;
static unsigned g_key_until;
static const char *g_labels[8] = {"a0", "c0"};
static int g_nlabels = 2;
static char g_label_buf[128];

static void script_parse(const char *s) {
    g_nsteps = 0; g_loop_from = 0;
    while (*s && g_nsteps < MAX_STEPS) {
        while (*s == ' ') s++;
        if (!*s) break;
        if (*s == '|') { g_loop_from = g_nsteps; s++; continue; }
        Step *st = &g_steps[g_nsteps++];
        st->op = *s++; st->a = st->b = 0;
        char *e;
        if (st->op == 'K') { st->a = (int)strtol(s, &e, 16); s = e; if (*s == ':') st->b = (int)strtol(s + 1, &e, 10), s = e; }
        else if (st->op == 'C') { st->a = (int)strtol(s, &e, 10); s = e; }
        else if (st->op != 'S' && st->op != 'L' && st->op != 'E' && st->op != 'T' && st->op != 'N' && st->op != 'M') { st->a = (int)strtol(s, &e, 10); s = e; }
    }
}

void hk_stress_init(const char *spec) {
    if (!spec || !*spec) return;
    char kind[32] = "";
    int n = 0;
    if (sscanf(spec, "%31[^:]:%d", kind, &n) < 1 || n <= 0) { rt_log("[stress] bad spec '%s' (kind:n)\n", spec); return; }
    const char *prologue_new = "A1300 K0d:6 S |";
    const char *prologue_cont = "A1300 K28:3 W20 K0d:6 D1500 W60 K0d:4 S |";
    const char *script = NULL;
    if (!strcmp(kind, "area") || !strcmp(kind, "territory"))
        script = "A1300 K0d:6 S | M W30 C40061 D1497 W20 K0d:4 N E";
    else if (!strcmp(kind, "idle"))   // the title's attract loop (logos, title, demo) left alone: a cycle is 900 frames
        script = "A1300 | M W900";
    else if (!strcmp(kind, "save"))
        script = "A1300 K28:3 W20 K0d:6 D1500 W60 K0d:4 S | M W60 K71:4 D1500 W40 K28:3 W40 K0d:4 N W60 L D1497 W20 K0d:4 N D1500 W60 K26:3 W40 K0d:4 S";   // the dialog remembers the slot used last: Up picks slot 1 again
    else if (!strcmp(kind, "menus"))
        script = "A1300 K28:3 W20 K0d:6 D1500 W60 K0d:4 S | M W30 C40061 D1497 W30 K1b:4 N W30 K71:4 D1500 W30 K1b:4 N W30 K79:3 W40 K79:3 W40";
    else { rt_log("[stress] unknown kind '%s' (area, territory, save, menus)\n", kind); return; }
    (void)prologue_new; (void)prologue_cont;
    const char *lab = getenv("HK_STRESS_LABELS");
    if (lab && !strcmp(kind, "territory")) {
        snprintf(g_label_buf, sizeof g_label_buf, "%s", lab);
        g_nlabels = 0;
        for (char *t = strtok(g_label_buf, ","); t && g_nlabels < 8; t = strtok(NULL, ",")) g_labels[g_nlabels++] = t;
    }
    g_kind = !strcmp(kind, "territory") ? 1 : 0;
    script_parse(script);
    g_total = n; g_active = 1; g_pc = 0; g_cycles = 0;
    if (!g_hk_memlog) g_hk_memlog = 1;
    rt_log("[stress] %s x%d, %d steps (loop from %d)\n", kind, n, g_nsteps, g_loop_from);
}

// Stage label for the next NEW GAME (territory kind), NULL otherwise: glue.c points the game's stage table at it.
const char *hk_stress_stage(void) { return g_active && g_kind == 1 ? g_labels[g_cycles % g_nlabels] : NULL; }

int hk_stress_key(int vk) { return g_active && g_key == vk && g_frame < g_key_until; }

static int step_done(const Step *s) {
    switch (s->op) {
    case 'A': return (int)g_frame >= s->a;
    case 'W': return g_frame - g_step_t0 >= (unsigned)s->a;
    case 'K': return g_frame >= g_key_until;
    case 'S': return g_hk_in_stage && !hk_dlg_active() ? ++g_stable >= 200 : (g_stable = 0, 0);
    case 'E':   // tap Enter every 24 frames until a stage has been drawn for 60 frames (title, logos, cards)
        if (!hk_dlg_active() && !g_hk_in_stage && (g_frame - g_step_t0) % 24 == 0) { g_key = 0x0d; g_key_until = g_frame + 4; }
        return g_hk_in_stage && !hk_dlg_active() ? ++g_stable >= 200 : (g_stable = 0, 0);
    case 'T': return !g_hk_in_stage && !hk_dlg_active() ? ++g_stable >= 90 : (g_stable = 0, 0);
    case 'D': return hk_dlg_top_res() == s->a;
    case 'N': return !hk_dlg_active();
    default: return 1;
    }
}

void hk_stress_tick(void) {
    if (g_active) {
        for (int guard = 0; guard < 4 && g_active; guard++) {
            Step *s = &g_steps[g_pc];
            if (g_frame - g_step_t0 > 6000 && s->op != 'A') {
                rt_log("[stress] TIMEOUT at step %d (%c%d) cycle %d, frame %u, in_stage=%d dlg=%d\n", g_pc, s->op, s->a, g_cycles, g_frame, g_hk_in_stage, hk_dlg_top_res());
                hk_mem_log("timeout", g_cycles);
                g_active = 0;
                break;
            }
            if (!step_done(s)) break;
            if (s->op == 'M') { if (g_cycles) hk_mem_log("cycle", g_cycles); else hk_mem_log("start", 0); }
            if (s->op == 'M' && g_cycles >= g_total) { rt_log("[stress] done %d cycles at frame %u\n", g_cycles, g_frame); rt_log_flush(); g_active = 0;
#ifndef __vita__
                if (getenv("HK_STRESS_EXIT") || getenv("HK_HEADLESS")) exit(0);
#endif
                break; }
            if (s->op == 'M') g_cycles++;
            g_pc++; g_stable = 0;
            if (g_pc >= g_nsteps) g_pc = g_loop_from;
            g_step_t0 = g_frame;
            s = &g_steps[g_pc];
            if (RT_TRACE("HK_STRESS_TRACE")) rt_log("[stress] step %d %c%d fr=%u stage=%d dlg=%d\n", g_pc, s->op, s->a, g_frame, g_hk_in_stage, hk_dlg_top_res());
            if (s->op == 'K') { g_key = s->a; g_key_until = g_frame + (unsigned)s->b; }
            else if (s->op == 'C') { w32_post_message(HWND_MAIN_DLG, 0x111u, (uint32_t)s->a, 0); }
            else if (s->op == 'L') hk_load_game_request();   // pause-menu Load game: Stop This Game, then Continue once the title is up
        }
    } else if (g_hk_memlog && !g_total && g_frame % 600 == 0 && g_frame) hk_mem_log("periodic", 0);
}
