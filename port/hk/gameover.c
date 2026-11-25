// "Continue from the last save?" after GAME OVER.
//
// Evidence (docs/save-and-stages.md "GAME OVER"): the stage's player/HUD object draw 0x43DB3A pushes the string
// "GAME OVER" (0x485E70) at 0x43DB82 only when the player object flag [obj+0x80] & 2 is set and [owner+0x58] & 1; the
// recompiled game gets a hook before 0x43DB82 (recomp/image_patches.py) that runs once per game-over frame. A few
// seconds later the game goes back to the startup sequence (the title).
//
// Flow: GAME OVER drawn for GO_WAIT frames and at least one slot file holds a save -> the platform layer asks the
// question (plat_present_xrgb -> hk_gameover_ask_pending / hk_gameover_answer). Yes: the existing Load game request
// (port/hk/dialog.c hk_load_game_request: Stop This Game, whose "Stop this game ?" dialog 1497 gets an injected Enter,
// then Continue = dialog 1500), then the slot last saved to / loaded from (g_hk_last_slot; fallback: newest in-save date) is
// checked directly in the dialog, verified, and confirmed with an injected Space (hk_gameover_key, ORed into
// plat_vk_down by the platform layer); a failed verification sends Esc and nothing is loaded. No: nothing changes.
#include "rt.h"
#include "win32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#define GO_WAIT 30
enum { GO_IDLE, GO_ASK, GO_LOAD, GO_VERIFY, GO_KEYS, GO_SETTLE };
static int g_go_target, g_go_abort;
static int g_go_drawn, g_go_frames, g_go_asked, g_go_state;
static unsigned g_go_t0, g_go_t1;
static int g_go_back;
extern int g_hk_in_stage;   // port/hk/wide.c
static unsigned char g_go_q[16];
static int g_go_qn;
static unsigned g_go_qat;

int hk_dlg_slots(char out[10][160], int enabled[10]);   // port/hk/dialog.c
int hk_dlg_select_slot(int n);                           // port/hk/dialog.c: check radio 1420+n-1 (1..10), 0 when it does not exist
int hk_dlg_selected_slot(void);                          // port/hk/dialog.c: checked slot 1..10 of dialog 1500, 0 if none
int hk_dlg_top_res(void);                                // port/hk/dialog.c: resource id of the top dialog, 0 if none
void hk_load_game_request(void);                         // port/hk/dialog.c
extern unsigned g_frame;

void hk_gameover_draw(CPU *c) { (void)c; g_go_drawn = 1; }

// The slot the player last saved to or loaded from (1..10, 0 = unknown). Set when dialog 1500 is confirmed
// (port/hk/dialog.c) and when the game writes UserNN.HDK (CopyFileA / MoveFileA, port/hk/imports_easy.c); persisted by
// the platform layer (Vita: hkport.cfg last_slot=N through hk_last_slot_cb; PC: HK_LAST_SLOT sets the start value).
int g_hk_last_slot;
void (*hk_last_slot_cb)(void);
void hk_last_slot_set(int n) {
    if (n < 1 || n > 10 || n == g_hk_last_slot) return;
    g_hk_last_slot = n;
    rt_dlog("[gameover] last slot = %d\n", n);
    if (hk_last_slot_cb) hk_last_slot_cb();
}
// A host path ".../UserNN.HDK" or "...UserNN.HDK.bak" (any case), NN 01..10.
void hk_last_slot_note_path(const char *hp) {
    size_t n = strlen(hp);
    if (n > 4 && !strcasecmp(hp + n - 4, ".bak")) n -= 4;
    if (n < 10 || strncasecmp(hp + n - 4, ".HDK", 4) || strncasecmp(hp + n - 10, "User", 4)) return;
    const char *b = hp + n - 6;
    if (b[0] >= '0' && b[0] <= '9' && b[1] >= '0' && b[1] <= '9') hk_last_slot_set((b[0] - '0') * 10 + b[1] - '0');
}
static int slot_nonempty(int i) {
    char g[32], h[600];
    struct stat st;
    snprintf(g, sizeof g, "USER\\USER%02d.HDK", i);
    w32_host_path(g, h, sizeof h);
    return !stat(h, &st) && st.st_size > 0;
}

static int have_save(void) {
    for (int i = 1; i <= 10; i++) {
        char g[32], h[600];
        struct stat st;
        snprintf(g, sizeof g, "USER\\USER%02d.HDK", i);
        w32_host_path(g, h, sizeof h);
        if (!stat(h, &st) && st.st_size > 0) return 1;
    }
    return 0;
}

// "Data 2 [11/21/2026 sat 9:32:33]" -> sortable number, 0 when the text has no date (empty slot).
static long long slot_date(const char *t) {
    const char *b = strchr(t, '[');
    int mo, d, y, h, mi, s;
    if (!b || sscanf(b + 1, "%d/%d/%d %*s %d:%d:%d", &mo, &d, &y, &h, &mi, &s) != 6) return 0;
    // The game formats "%d/%d/%d" as month/day/year (both exes share the format string), but accept day/month/year
    // too: a first field above 12 can only be the day.
    if (mo > 12 && d <= 12) { int t = mo; mo = d; d = t; }
    return (((long long)y * 13 + mo) * 32 + d) * 86400 + h * 3600 + mi * 60 + s;
}

static void key_push(int vk) { if (g_go_qn < (int)sizeof g_go_q) g_go_q[g_go_qn++] = (unsigned char)vk; }
int hk_gameover_key(int vk) { return g_go_qn && g_go_q[0] == vk && g_frame >= g_go_qat && g_frame < g_go_qat + 3; }

void hk_gameover_tick(void) {   // once per present (port/hk/ddraw.c)
    static int gap;
    if (g_go_drawn) { g_go_frames++; gap = 0; }
    else if (++gap > 100) { g_go_frames = 0; if (g_go_state == GO_IDLE) g_go_asked = 0; }
    g_go_drawn = 0;
    if (g_go_qn && g_frame >= g_go_qat + 6) { memmove(g_go_q, g_go_q + 1, (size_t)--g_go_qn); g_go_qat = g_frame + 1; }
    switch (g_go_state) {
    case GO_IDLE:
        if (g_go_frames >= GO_WAIT && !g_go_asked) {
            g_go_asked = 1;
            if (have_save()) { g_go_state = GO_ASK; rt_dlog("[gameover] question\n"); }
            else rt_dlog("[gameover] no save, no question\n");
        }
        break;
    case GO_LOAD: {
        char t[10][160]; int en[10];
        if (g_frame - g_go_t0 > 1500) { g_go_state = GO_IDLE; rt_dlog("[gameover] gave up\n"); break; }
        if (hk_dlg_top_res() == 1497) {   // "Stop this game ?": Enter = OK, once (the dialog stays up a few frames)
            static unsigned pushed;
            if (!g_go_qn && g_frame - pushed > 60) { pushed = g_frame; g_go_qat = g_frame + 3; key_push(0x0D); rt_dlog("[gameover] Stop: Enter\n"); }
            break;
        }
        if (hk_dlg_slots(t, en) != 1) break;
        if (!g_hk_last_slot) { const char *e = getenv("HK_LAST_SLOT"); if (e) g_hk_last_slot = atoi(e); }
        int best = -1, target = 0;
        long long bv = 0;
        for (int i = 0; i < 10; i++) {
            long long v = slot_date(t[i]);
            rt_dlog("[gameover] slot %d \"%s\" enabled %d date %lld\n", i + 1, t[i], en[i], v);
            if (v > bv) { bv = v; best = i; }
        }
        int ls = g_hk_last_slot;
        if (ls >= 1 && ls <= 10 && en[ls - 1] && t[ls - 1][0] && slot_nonempty(ls)) { target = ls; rt_dlog("[gameover] using last used slot %d\n", ls); }
        else if (best >= 0) { target = best + 1; rt_dlog("[gameover] last slot %d unusable: newest dated slot %d\n", ls, target); }
        if (!target) { g_go_state = GO_IDLE; rt_dlog("[gameover] no dated slot\n"); break; }
        g_go_target = target; g_go_abort = 0;
        hk_dlg_select_slot(target);
        g_go_t1 = g_frame; g_go_state = GO_VERIFY;
        break;
    }
    case GO_VERIFY:   // a few frames after the direct selection: the game's own loop must still show the chosen slot
        if (hk_dlg_top_res() != 1500) { if (g_frame - g_go_t1 > 30) { g_go_state = GO_IDLE; rt_dlog("[gameover] dialog gone before the slot was confirmed\n"); } break; }
        if (g_frame - g_go_t1 < 4) break;
        if (hk_dlg_selected_slot() == g_go_target) {
            rt_dlog("[gameover] slot %d verified, confirming\n", g_go_target);
            key_push(0x20);
        } else {
            rt_dlog("[gameover] slot %d NOT verified (selected %d): back to the title, nothing loaded\n", g_go_target, hk_dlg_selected_slot());
            key_push(0x1B); g_go_abort = 1;
        }
        g_go_qat = g_frame + 2; g_go_state = GO_KEYS;
        break;
    case GO_KEYS:
        if (!g_go_qn) { g_go_state = GO_SETTLE; g_go_t1 = g_frame; g_go_back = 0; }
        break;
    case GO_SETTLE:   // the slot was confirmed: the stage loads; the chain stays hidden until it runs
        if (g_go_abort) { if (g_frame - g_go_t1 > 40) g_go_state = GO_IDLE; break; }
        if (g_hk_in_stage) { if (!g_go_back) { g_go_back = 1; g_go_t1 = g_frame; } else if (g_frame - g_go_t1 > 8) g_go_state = GO_IDLE; }
        else if (g_frame - g_go_t1 > 900) g_go_state = GO_IDLE;
        break;
    }
    if ((g_go_state == GO_LOAD || g_go_state == GO_VERIFY || g_go_state == GO_KEYS) && g_frame - g_go_t0 > 1500) g_go_state = GO_IDLE;
}
// While the auto-load chain runs (Stop confirm, title, Continue dialog, slot selection) the port shows a frozen frame with a
// "Loading..." box instead of those screens (port/hk/ddraw.c).
int hk_gameover_hiding(void) { return g_go_state == GO_LOAD || g_go_state == GO_VERIFY || g_go_state == GO_KEYS || g_go_state == GO_SETTLE; }

int hk_gameover_ask_pending(void) { return g_go_state == GO_ASK; }

void hk_gameover_answer(int yes) {
    if (g_go_state != GO_ASK) return;
    if (!yes) { g_go_state = GO_IDLE; return; }
    g_go_t0 = g_frame; g_go_state = GO_LOAD;
    hk_load_game_request();
}
