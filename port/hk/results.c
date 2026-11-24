// Live RESULTS counters (the values of the game's RESULTS screen), read from guest memory for the port's own
// RESULTS panel (Select on the Vita, port/vita/pause_menu.h pm_results). Evidence: docs/save-and-stages.md "Results
// counters".
//
// The game object (vtable 0x472240, 0x768 bytes, ctor 0x44C721 called from the stage scene ctor at 0x454333 and
// from 0x44CDAA) keeps 13 dwords at +0x4AC: reset by its ctor (0x44C95D -> 0x44D38B memset 0x34), increased by its
// virtual 0x40433B -> 0x44D36C (`add [ecx+idx*4+0x4AC],amount`) and saved / loaded with the game (0x44CE3F /
// 0x44CE66, 0x34 bytes). The RESULTS screen (ctor 0x4570B6, draw 0x45735B) points at them ([this+0x8C]) and shows
// TIME = [0] / 25 as h:m:s (0x45749E), then with format 0x48A280 "%d/%d\n\n%d/%d\n\n%d/%d\n\n%c%d\n\n%d\n\n%d\n\n%d"
// (0x457609..0x45763F): SHOT [3]/[2], BOMB [5]/[4], TURNCOAT BOMB [7]/[6], EXPENDITURE '$'[9], KILL(S) [10],
// DESTRUCTION [11], INJURIES [12]. The stage scene (vtable 0x4724B8) holds the object at +0x60 (0x455286 reads the
// territory-cleared flags [obj+0x45C] through it).
#include "rt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RS_SCENE_VT 0x4724B8u
#define RS_GAME_VT 0x472240u
static uint32_t g_rs_scene;   // last stage scene drawn (hk_ws_bar_x, port/hk/wide.c)
extern int g_hk_in_stage;     // port/hk/wide.c

void hk_results_scene(uint32_t scene) { g_rs_scene = scene; }

// Copies the 13 counters; 0 when no stage is shown or the object does not look right.
int hk_results_read(uint32_t out[13]) {
    uint8_t *const mb = g_mem;
    const uint32_t s = g_rs_scene;
    if (!g_hk_in_stage || !s || s + 0x64u > g_mem_size || R32(s) != RS_SCENE_VT) return 0;
    const uint32_t o = R32(s + 0x60u);
    if (!o || o + 0x4ACu + 0x34u > g_mem_size || R32(o) != RS_GAME_VT) return 0;
    memcpy(out, mb + o + 0x4ACu, 13 * 4);
    return 1;
}

// HK_RESULTS_LOG=1 (test aid): logs the counters whenever one other than the time changes.
void hk_results_tick(void) {
    static int on = -1;
    static uint32_t last[13];
    if (on < 0) on = HK_ENV("HK_RESULTS_LOG") != NULL;
    uint32_t v[13];
    if (!on) return;
    static int was_ok = -1;
    const int ok = hk_results_read(v);
    if (ok != was_ok) { extern unsigned g_frame; rt_dlog("[results] frame %u: counters %s (scene %08x)\n", g_frame, ok ? "readable" : "not readable", g_rs_scene); was_ok = ok; }
    if (!ok) return;
    if (!memcmp(v + 1, last + 1, 12 * 4)) return;
    memcpy(last, v, sizeof last);
    extern unsigned g_frame;
    uint8_t *const mb = g_mem;
    char line[200];
    int n = snprintf(line, sizeof line, "[results] frame %u obj %08x:", g_frame, R32(g_rs_scene + 0x60u));
    for (int i = 0; i < 13 && n < (int)sizeof line; i++) n += snprintf(line + n, sizeof line - (size_t)n, " %u", v[i]);
    rt_dlog("%s\n", line);
}
