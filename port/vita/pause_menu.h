// Pause menu (Start) drawing and layout for the Vita port, in the style of the game's title menu: a framed box with a
// purple border, centred items, the selected one marked with red dots. Pure drawing code (no SDL, no Vita API) so the
// same code renders on PC for a preview (port/vita/pause_preview.c). Needs hk/font_data.h before it.
#ifndef HK_PAUSE_MENU_H
#define HK_PAUSE_MENU_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hk/i18n.h"

enum { PM_MAIN, PM_CHEATS, PM_SETTINGS, PM_CONTROLS, PM_RESULTS /* the counters panel; only R_BACK is selectable */ };
enum { R_CONTINUE, R_SAVE, R_CHEATS, R_SETTINGS, R_GAMEMENU, R_QUIT,
       R_CHEAT_ON, R_INV, R_MONEY, R_BACK, R_SCREEN, R_NATIVE, R_FPS, R_VOL, R_MUS,
       R_CONTROLS, R_RESET, R_LOAD, R_LANG, R_RESULTS, R_ACT0 /* R_ACT0 + a - 1 rows for actions 1..PM_NACT-1 */ };

// Gameplay button map: map[button] = action. D-pad and left stick (move) and Start (pause) are not remappable.
enum { ACT_NONE, ACT_SHOOT, ACT_RAPID, ACT_JUMP, ACT_BOMB, ACT_SWITCH, ACT_SHIFT, ACT_MENU, ACT_TIPS, PM_NACT };
enum { PB_CROSS, PB_SQUARE, PB_CIRCLE, PB_TRIANGLE, PB_L, PB_R, PB_SELECT, PM_NBTN };
static const char *pm_act_name(int a) { return hk_tr(S_A_NONE + a); }   // S_A_* are in the ACT_* order
static const char *const pm_act_key[PM_NACT] = {"none", "shoot", "rapid", "jump", "bomb", "switch", "shift", "menu", "tips"};
static const char *const pm_btn_name[PM_NBTN] = {"Cross", "Square", "Circle", "Triangle", "L", "R", "Select"};
static const char *const pm_btn_key[PM_NBTN] = {"cross", "square", "circle", "triangle", "l", "r", "select"};
static void pm_map_defaults(int *map) {
    static const int d[PM_NBTN] = {ACT_JUMP, ACT_SHOOT, ACT_BOMB, ACT_SWITCH, ACT_SHIFT, ACT_RAPID, ACT_TIPS};   // Select (held) = tips & controls card; the game menu is Start > Game menu (assignable)
    for (int i = 0; i < PM_NBTN; i++) map[i] = d[i];
}
// Assigns action a to button b; the action that was on b moves to the button a came from (swap).
static void pm_map_assign(int *map, int b, int a) {
    for (int i = 0; i < PM_NBTN; i++) if (i != b && map[i] == a) { map[i] = map[b]; break; }
    map[b] = a;
}

typedef struct {
    int wide, fps, wide_native, started_native, vol, mus;   // settings (wide: 1 = stretched 16:9)
    int cheats, inv_on, money, in_stage;
    const int *map;   // button map (PM_NBTN entries)
    int capture;      // Controls: waiting for a button to assign (action id)
    int lang, lang_ok, started_lang;   // language (0 English, 1 Portugues BR) and whether the translation files were found
} PmState;

// Selectable rows of a screen, in order; returns the count.
static int pm_rows(int screen, const PmState *s, int *out) {
    int n = 0;
    if (screen == PM_MAIN) {
        out[n++] = R_CONTINUE; out[n++] = R_SAVE; out[n++] = R_LOAD; out[n++] = R_CHEATS; out[n++] = R_SETTINGS; out[n++] = R_RESULTS; out[n++] = R_GAMEMENU; out[n++] = R_QUIT;
    } else if (screen == PM_RESULTS) {
        out[n++] = R_BACK;
    } else if (screen == PM_CHEATS) {
        out[n++] = R_CHEAT_ON;
        if (s->cheats) { out[n++] = R_INV; out[n++] = R_MONEY; }
        out[n++] = R_BACK;
    } else if (screen == PM_CONTROLS) {
        for (int a = 1; a < PM_NACT; a++) out[n++] = R_ACT0 + a - 1;
        out[n++] = R_RESET; out[n++] = R_BACK;
    } else {
        out[n++] = R_SCREEN; out[n++] = R_NATIVE; out[n++] = R_FPS; out[n++] = R_VOL; out[n++] = R_MUS; out[n++] = R_CONTROLS; out[n++] = R_LANG; out[n++] = R_BACK;
    }
    return n;
}

static const char *pm_label(int id) {
    switch (id) {
    case R_CONTINUE: return hk_tr(S_CONTINUE);
    case R_SAVE: return hk_tr(S_SAVE);
    case R_LOAD: return hk_tr(S_LOAD);
    case R_LANG: return hk_tr(S_LANG);
    case R_CHEATS: return hk_tr(S_CHEATS);
    case R_SETTINGS: return hk_tr(S_SETTINGS);
    case R_RESULTS: return hk_tr(S_RESULTS);
    case R_GAMEMENU: return hk_tr(S_GAMEMENU);
    case R_QUIT: return hk_tr(S_QUIT);
    case R_CHEAT_ON: return hk_tr(S_CHEATS);
    case R_INV: return hk_tr(S_INV);
    case R_MONEY: return hk_tr(S_MONEY);
    case R_BACK: return hk_tr(S_BACK);
    case R_SCREEN: return hk_tr(S_SCREEN);
    case R_NATIVE: return hk_tr(S_NATIVE);
    case R_FPS: return hk_tr(S_FPS);
    case R_VOL: return hk_tr(S_VOL);
    case R_CONTROLS: return hk_tr(S_CONTROLS);
    case R_RESET: return hk_tr(S_RESET);
    case R_MUS: return hk_tr(S_MUS);
    default: return pm_act_name(id - R_ACT0 + 1);
    }
}
// Value text at the right of a row ("" = none; volume and music are drawn as bars instead).
static void pm_value(int id, const PmState *s, char *out, size_t cap) {
    out[0] = 0;
    switch (id) {
    case R_CHEAT_ON: snprintf(out, cap, "%s", hk_tr(s->cheats ? S_ON : S_OFF)); break;
    case R_INV: snprintf(out, cap, "%s", hk_tr(s->inv_on ? S_ON : S_OFF)); break;
    case R_MONEY: if (s->money) snprintf(out, cap, "x%d", s->money); break;
    case R_SCREEN: snprintf(out, cap, "%s", hk_tr(s->wide ? S_W169 : S_W43)); break;
    case R_NATIVE: snprintf(out, cap, "%s", hk_tr(s->wide_native ? S_ON : S_OFF)); break;
    case R_FPS: snprintf(out, cap, "%s", hk_tr(s->fps ? S_ON : S_OFF)); break;
    case R_LANG: snprintf(out, cap, "%s", s->lang ? "Portugu\352s (BR)" : "English"); break;
    default:
        if (id >= R_ACT0 && s->map) {
            int a = id - R_ACT0 + 1;
            if (s->capture == a) { snprintf(out, cap, hk_tr(S_PRESS)); break; }
            snprintf(out, cap, "-");
            for (int b = 0; b < PM_NBTN; b++) if (s->map[b] == a) { snprintf(out, cap, "%s", pm_btn_name[b]); break; }
        }
        break;
    }
}

#include "hk/theme.h"
#include "hk/gameart.h"
#define PM_FILL HK_TH_FILL

static void pm_rect(uint32_t *buf, int bw, int x0, int y0, int x1, int y1, uint32_t col) {
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (x >= 0 && x < bw && y >= 0 && y < HK_CANVAS_H(bw)) buf[y * bw + x] = col;
}
static void pm_blend(uint32_t *buf, int bw, int x0, int y0, int x1, int y1, uint32_t col, int a) {   // a/16 of col
    for (int y = y0 < 0 ? 0 : y0; y < y1 && y < HK_CANVAS_H(bw); y++)
        for (int x = x0 < 0 ? 0 : x0; x < x1 && x < bw; x++) {
            uint32_t p = buf[y * bw + x], r = 0;
            for (int sh = 0; sh < 24; sh += 8) r |= ((((p >> sh) & 255u) * (16 - a) + ((col >> sh) & 255u) * a) / 16u) << sh;
            buf[y * bw + x] = r;
        }
}

// Darkens the game frame (src: XRGB, pitch in pixels) into dst (bw x 480).
static void pm_dim(uint32_t *dst, const uint32_t *src, int pitch, int width, int height, int bw) {
    memset(dst, 0, (size_t)bw * 480 * 4);
    for (int y = 0; y < height && y < 480; y++)
        for (int x = 0; x < width && x < bw; x++) dst[y * bw + x] = (src[y * pitch + x] >> 1) & 0x7f7f7fu;
}

// The title screen's menu box (hk/gameart.h: HDKSYS00 9-slice at (0,56)) over a dark translucent inside, so the
// text stays readable over a busy stage (the title draws the box over its dark ground without a fill).
static void pm_panel(uint32_t *buf, int bw, int x0, int y0, int x1, int y1) {
    pm_blend(buf, bw, x0 + 3, y0 + 3, x1 - 3, y1 - 3, PM_FILL, 12);
    hk_art_frame(buf, bw, x0, y0, x1 - x0, y1 - y0);
}
// Text with pad symbols: markers \001 cross, \002 circle, \003 triangle, \004 square, \005 L, \006 R, \007 D-pad, and
// the words Cross / Circle / Triangle / Square (a ':' right after one is dropped) become the button symbols.
// upper: the text runs in upper case, as the title menu words. Returns the width; draws only when buf is set.
// The menus are drawn on a 960x544 canvas (the Vita's native output, see "Overlay" below), never stretched, so the
// text scale is a whole number: pm_h in halves is 2 (1x, the title's size) or 4 (2x, 22 px caps); headings are 2
// halves larger (3x at 4). Each screen uses the largest scale up to g_pm_ui that fits in 544 rows (pm_render); the
// hint line is 2x when it fits the width, else 1x (pm_hs). An odd menu_scale (3) rounds down to 1x.
static int g_pm_ui = 4;   // the owner's choice (config menu_scale: 2, 3, 4)
static int pm_h = 2;      // scale of the screen being drawn (even)
static int pm_hs = 2;     // scale of the hint line and the note (even)
static int pm_rich(uint32_t *buf, int bw, int x, int y, const char *s, int style, int upper) {
    const int h = upper ? pm_h : pm_hs, sc = h / 2;   // pad symbols are replicated sc times
    static const char *const words[5] = {"", "Cross", "Circle", "Triangle", "Square"};
    char run[160];
    int n = 0, x0 = x;
    for (const char *p = s;; ) {
        int sym = 0, len = 0;
        if (*p >= 1 && *p <= 7) { sym = *p; len = 1; }
        else for (int k = 1; k <= 4; k++) {
            int wl = (int)strlen(words[k]);
            if (!strncmp(p, words[k], (size_t)wl) && !((p[wl] | 32) >= 'a' && (p[wl] | 32) <= 'z')) { sym = k; len = wl; if (p[wl] == ':') len++; break; }
        }
        if (sym || !*p || n >= (int)sizeof run - 1) {
            run[n] = 0;
            if (n) {
                char u[160];
                if (upper) hk_gf_upper(u, run, sizeof u); else strcpy(u, run);
                if (buf) hk_gf_text_h(buf, bw, x, y, u, style, h);
                x += hk_gf_width_h(u, h);
            }
            n = 0;
            if (!*p) break;
        }
        if (!sym) { run[n++] = *p++; continue; }
        p += len;
        const int sy = y;
        if (sym <= 4) { if (buf) hk_th_glyph_s(buf, bw, HK_CANVAS_H(bw), x + sc, sy - sc, sym, sc); x += 19 * sc; }
        else if (sym <= 6) {
            if (buf) { pm_rect(buf, bw, x + sc, sy, x + 20 * sc, sy + 14 * sc, 0x433a89u); pm_rect(buf, bw, x + 2 * sc, sy + sc, x + 19 * sc, sy + 13 * sc, 0x1c1838u); hk_gf_text(buf, bw, x + 7 * sc, sy - sc, sym == 5 ? "L" : "R", GF_SEL, sc); }
            x += 23 * sc;
        }
        else { if (buf) { pm_rect(buf, bw, x + 3 * sc, sy + 5 * sc, x + 16 * sc, sy + 8 * sc, 0xdcd6c6u); pm_rect(buf, bw, x + 8 * sc, sy, x + 11 * sc, sy + 13 * sc, 0xdcd6c6u); } x += 20 * sc; }
    }
    return x - x0;
}
static int pm_rich_w(const char *s, int upper) { return pm_rich(NULL, 0, 0, 0, s, 0, upper); }
// A selected row: the title's red dots, 12 px left of the text and 4 px right of its end (0x408D5C), 2 px lower than
// the word's top (the font's caps start at cell row 2).
// Scaled with the text (dot 8 * h / 2, gaps 12 and 4 at 1x), centred on the caps (cell rows 2..12).
static void pm_dots(uint32_t *buf, int bw, int xl, int xr, int y) {
    const int h = pm_h, d = 4 * h, yy = y + (15 * h + 2) / 4 - d / 2;
    hk_art_dot_h(buf, bw, xl - 6 * h, yy, h);
    if (xr >= 0) hk_art_dot_h(buf, bw, xr + 2 * h, yy, h);
}
static int pm_utext(uint32_t *buf, int bw, int x, int y, const char *s, int style) {   // upper-case text, scale pm_h
    char u[160];
    hk_gf_upper(u, s, sizeof u);
    return hk_gf_text_h(buf, bw, x, y, u, style, pm_h);
}
static int pm_uw(const char *s) { char u[160]; hk_gf_upper(u, s, sizeof u); return hk_gf_width_h(u, pm_h); }
#define PM_CELL (8 * pm_h)            // text cell height at the current scale
#define PM_HEAD_H (8 * (pm_h + 2))    // heading cell: one step (1x) larger than the rows
static void pm_heading(uint32_t *buf, int bw, int cx, int y, const char *s) {   // pm_h + 1x, in the PAUSE sign's colours
    char u[96];
    hk_gf_upper(u, s, sizeof u);
    hk_gf_text_h(buf, bw, cx - hk_gf_width_h(u, pm_h + 2) / 2, y, u, GF_HEAD, pm_h + 2);
}

// Draws the menu box over buf (already dimmed). sel indexes the selectable rows of the screen. With buf NULL only lays
// out and returns 1 if the box fits the 480-row frame at scale pm_h (pm_render tries 2x, 1.5x, 1x).
static int pm_render_at(uint32_t *buf, int bw, int screen, int sel, const PmState *st) {
    const int PITCH = PM_CELL + (pm_h >= 4 ? 2 : 6), SECT = PM_CELL + (pm_h >= 4 ? 4 : 8);   // 2x: tight rows so Settings fits 544
    int ids[16], n = pm_rows(screen, st, ids);
    const char *title = hk_tr(screen == PM_MAIN ? S_PAUSE : screen == PM_CHEATS ? S_CHEATS_T : screen == PM_CONTROLS ? S_CONTROLS_T : S_SETTINGS_T);
    const char *note = NULL;   // settings note under the rows
    if (screen == PM_SETTINGS) {
        if (st->lang && !st->lang_ok) note = hk_tr(S_N_NOFILES);
        else if (st->lang != st->started_lang) note = hk_tr(S_N_LANG);
        else if (st->wide_native != st->started_native) note = hk_tr(S_N_NATIVE);
    }
    const char *hint = hk_tr(screen == PM_MAIN ? S_H_MAIN : screen == PM_CONTROLS ? (st->capture ? S_H_CAPTURE : S_H_ASSIGN) : S_H_CHANGE);
    pm_hs = pm_h >= 4 ? 4 : 2;   // hint 2x when it fits the width, else 1x
    if (pm_hs == 4 && pm_rich_w(hint, 0) + 56 > bw - 160) pm_hs = 2;
    const int hh = pm_hs, hc = 8 * hh;
    const int slot = 6 * pm_h, barw = 10 * slot;   // volume bar: 10 slots of 12 px at 1x
    int body = 0, boxw = 150 * pm_h, cw;
    for (int i = 0; i < n; i++) {
        char val[32];
        pm_value(ids[i], st, val, sizeof val);
        int bar = ids[i] == R_VOL || ids[i] == R_MUS;
        cw = pm_uw(pm_label(ids[i])) + (val[0] ? 20 * pm_h + pm_rich_w(val, 1) : bar ? 20 * pm_h + barw : 0) + 96;
        if (cw > boxw) boxw = cw;
        body += PITCH;
        if (ids[i] == R_SCREEN || ids[i] == R_VOL) body += SECT;
    }
    if ((cw = pm_rich_w(hint, 0) + 56) > boxw) boxw = cw;
    if (note && (cw = hk_gf_width_h(note, hh) + 56) > boxw) boxw = cw;
    const int extra = note ? hc + 6 : 0;
    const int boxh = 18 + PM_HEAD_H + 12 + body + extra + 8 + 2 + 10 + hc + 14;   // pad, heading, gap, rows, note, gap, rule, gap, hint, pad
    if (!buf) return boxw <= bw - 16 && boxh <= HK_CANVAS_H(bw) - 8;
    if (boxw > bw - 16) boxw = bw - 16;
    const int x0 = (bw - boxw) / 2, y0 = (HK_CANVAS_H(bw) - boxh) / 2, x1 = x0 + boxw, y1 = y0 + boxh;
    pm_panel(buf, bw, x0, y0, x1, y1);
    int y = y0 + 18;
    pm_heading(buf, bw, bw / 2, y, title);
    y += PM_HEAD_H + 12;
    for (int i = 0; i < n; i++) {
        int id = ids[i];
        if (id == R_SCREEN || id == R_VOL) {
            int hy = y + 2;
            const char *sec = hk_tr(id == R_SCREEN ? S_GRAPHICS : S_SOUND);
            pm_utext(buf, bw, x0 + 24, hy, sec, GF_GOLD);
            pm_rect(buf, bw, x0 + 30 + pm_uw(sec), hy + 7 * pm_h / 2, x1 - 24, hy + 7 * pm_h / 2 + 2, 0x433a89u);
            y += SECT;
        }
        int on = i == sel;
        int disabled = (id == R_SAVE || id == R_RESULTS) && !st->in_stage;   // Results: no counters outside a stage; Load in a stage first stops the game (hk_load_game_request)
        int style = disabled ? GF_DIM : on ? GF_SEL : GF_NORMAL;
        const char *lab = pm_label(id);
        char val[32];
        pm_value(id, st, val, sizeof val);
        int bar = id == R_VOL || id == R_MUS;
        if (!val[0] && !bar) {   // centred, as the title menu
            int tw = pm_uw(lab), tx = (bw - tw) / 2;
            pm_utext(buf, bw, tx, y, lab, style);
            if (on) pm_dots(buf, bw, tx, tx + tw, y);
            y += PITCH;
            continue;
        }
        int lx = x0 + 40, rx = x1 - 40;
        pm_utext(buf, bw, lx, y, lab, style);
        if (bar) {
            const int q = pm_h / 2;   // pixel unit of the bar (1 px at 1x, 2 at 2x)
            int v = id == R_VOL ? st->vol : st->mus, bx = rx - barw + 2 * q, t = y + pm_h, b = y + 7 * pm_h;
            for (int k = 0; k < 10; k++) {
                int sx = bx + k * slot, ex = sx + slot - 2 * q;
                pm_rect(buf, bw, sx, t, ex, b, 0x191a19u);
                pm_rect(buf, bw, sx + q, t + q, ex - q, b - q, k < v ? (on ? 0xfbfddau : 0x998979u) : 0x2c1e6du);
                if (k < v) pm_rect(buf, bw, sx + q, b - q - 3 * q, ex - q, b - q, on ? 0xdcd6c6u : 0x857767u);
            }
        } else {
            pm_rich(buf, bw, rx - pm_rich_w(val, 1), y, val, on ? GF_GOLD : style, 1);
        }
        if (on) pm_dots(buf, bw, lx, rx, y);
        y += PITCH;
    }
    if (note) hk_gf_text_h(buf, bw, (bw - hk_gf_width_h(note, hh)) / 2, y + 4, note, GF_GOLD, hh);
    pm_rect(buf, bw, x0 + 20, y1 - 14 - hc - 12, x1 - 20, y1 - 14 - hc - 10, 0x433a89u);
    pm_rich(buf, bw, (bw - pm_rich_w(hint, 0)) / 2, y1 - 14 - hc, hint, GF_NORMAL, 0);
    return 1;
}
static void pm_render(uint32_t *buf, int bw, int screen, int sel, const PmState *st) {
    for (pm_h = g_pm_ui >= 4 ? 4 : 2; pm_h > 2 && !pm_render_at(NULL, bw, screen, sel, st); pm_h -= 2) {}
    pm_render_at(buf, bw, screen, sel, st);
}

// RESULTS panel size at scale pm_h (2 or 4).
static int pm_res_scale(void) { return pm_h = g_pm_ui >= 4 ? 4 : 2; }
#define PM_RES_W (200 * pm_h)
#define PM_RES_H (18 + PM_HEAD_H + 18 + 8 * (PM_CELL + 8) + 20)
// RESULTS panel (held Select in a stage): the counters of the game's own RESULTS screen, live (port/hk/results.c
// reads them; offsets and evidence in docs/save-and-stages.md "Results counters"). v[13] in the game's order; the
// layout follows the RESULTS screen draw 0x45735B: TIME = v[0] / 25 as hh:mm:ss, SHOT v[3]/v[2], BOMB v[5]/v[4],
// TURNCOAT BOMB v[7]/v[6], EXPENDITURE $v[9], KILL(S) v[10], DESTRUCTION v[11], INJURIES v[12]. Drawn over the
// running game (not dimmed outside the panel).
static void pm_results_draw(uint32_t *buf, int bw, const uint32_t *v) {
    enum { NR = 8 };
    const int PITCH = PM_CELL + 8;
    char val[NR][24];
    const uint32_t t = v[0] / 25u;
    snprintf(val[0], sizeof val[0], "%.2u:%.2u:%.2u", t / 3600u, t / 60u % 60u, t % 60u);
    snprintf(val[1], sizeof val[1], "%u/%u", v[3], v[2]);
    snprintf(val[2], sizeof val[2], "%u/%u", v[5], v[4]);
    snprintf(val[3], sizeof val[3], "%u/%u", v[7], v[6]);
    snprintf(val[4], sizeof val[4], "$%u", v[9]);
    snprintf(val[5], sizeof val[5], "%u", v[10]);
    snprintf(val[6], sizeof val[6], "%u", v[11]);
    snprintf(val[7], sizeof val[7], "%u", v[12]);
    static const int lab[NR] = {S_R_TIME, S_R_SHOT, S_R_BOMB, S_R_TCBOMB, S_R_EXP, S_R_KILL, S_R_DESTR, S_R_INJ};
    const int boxw = PM_RES_W, boxh = PM_RES_H;
    const int x0 = (bw - boxw) / 2, y0 = (HK_CANVAS_H(bw) - boxh) / 2, x1 = x0 + boxw, y1 = y0 + boxh;
    pm_panel(buf, bw, x0, y0, x1, y1);
    int y = y0 + 18;
    pm_heading(buf, bw, bw / 2, y, hk_tr(S_RESULTS));
    y += PM_HEAD_H + 18;
    for (int i = 0; i < NR; i++) {
        pm_utext(buf, bw, x0 + 40, y, hk_tr(lab[i]), GF_NORMAL);
        hk_gf_text_h(buf, bw, x1 - 40 - hk_gf_width_h(val[i], pm_h), y, val[i], GF_SEL, pm_h);
        y += PITCH;
    }
}

// ---- Tips & controls card (Select held) ----
// The current button map (pm_act_name / g_map) and one of the game's own hint texts (string resources of the running
// EXE, so the PT-BR EXE gives Portuguese; nothing is stored here). The caller fills "%s" with pm_btn_sym().
// Button as drawn: a pad symbol marker (pm_rich) for the face and shoulder buttons, the word for Select.
static const char *pm_btn_sym(int b) {
    static const char *const sym[PM_NBTN] = {"\001", "\004", "\002", "\003", "\005", "\006", "Select"};
    return sym[b];
}
// Copies fmt to out replacing the first "%s" with the button that has action a ("-" when unassigned).
static void pm_tip_fill(char *out, size_t cap, const char *fmt, const int *map, int a) {
    const char *sym = "-";
    for (int b = 0; b < PM_NBTN; b++) if (map[b] == a) { sym = pm_btn_sym(b); break; }
    size_t n = 0;
    for (const char *p = fmt; *p && n + 1 < cap; ) {
        if (p[0] == '%' && p[1] == 's') { for (const char *q = sym; *q && n + 1 < cap; q++) out[n++] = *q; p += 2; }
        else out[n++] = *p++;
    }
    out[n] = 0;
}
typedef struct { const int *map; int page, npages; char tip[400]; } PmTipsArg;
#define PM_TIPS_LINES 6
// Returns 1 when the card fits at the current pm_h; draws only when buf is set.
static int pm_tips_at(uint32_t *buf, int bw, const PmTipsArg *a) {
    const int PITCH = PM_CELL + (pm_h >= 4 ? 2 : 6), cap = PM_CELL + 6, LH = PM_CELL + 4;
    pm_hs = pm_h;
    char lines[PM_TIPS_LINES][128];
    int nl = 0, tw = 0;
    for (const char *p = a->tip; *p && nl < PM_TIPS_LINES; ) {
        int n = 0;
        while (*p && *p != '\n') { if (*p != '\r' && n < (int)sizeof lines[0] - 1) lines[nl][n++] = *p; p++; }
        if (*p == '\n') p++;
        lines[nl][n] = 0;
        if (pm_rich_w(lines[nl], 0) > tw) tw = pm_rich_w(lines[nl], 0);
        nl++;
    }
    if (!nl) { snprintf(lines[0], sizeof lines[0], "%s", hk_tr(S_T_NONE)); tw = pm_rich_w(lines[0], 0); nl = 1; }
    const int na = PM_NACT - 1, rows = (na + 1) / 2;
    int colw = 0;
    char val[2][PM_NACT][12];
    for (int k = 1; k < PM_NACT; k++) {
        snprintf(val[0][k], sizeof val[0][k], "-");
        for (int b = 0; b < PM_NBTN; b++) if (a->map[b] == k) { snprintf(val[0][k], sizeof val[0][k], "%s", pm_btn_sym(b)); break; }
        int w = pm_uw(pm_act_name(k)) + 24 + pm_rich_w(val[0][k], 1);
        if (w > colw) colw = w;
    }
    char tag[40], hint[64];
    snprintf(tag, sizeof tag, hk_tr(S_T_TIP), a->page + 1, a->npages);
    snprintf(hint, sizeof hint, "%s", hk_tr(S_T_HINT));
    int boxw = 2 * colw + 40 + 96;
    if (tw + 96 > boxw) boxw = tw + 96;
    if (pm_rich_w(hint, 0) + 56 > boxw) boxw = pm_rich_w(hint, 0) + 56;
    const int boxh = 18 + PM_HEAD_H + 12 + cap + rows * PITCH + 10 + 2 + 10 + cap + nl * LH + 10 + 2 + 10 + PM_CELL + 14;
    if (!buf) return boxw <= bw - 16 && boxh <= HK_CANVAS_H(bw) - 8;
    const int x0 = (bw - boxw) / 2, y0 = (HK_CANVAS_H(bw) - boxh) / 2, x1 = x0 + boxw, y1 = y0 + boxh;
    pm_panel(buf, bw, x0, y0, x1, y1);
    int y = y0 + 18;
    pm_heading(buf, bw, bw / 2, y, hk_tr(S_TIPS_T));
    y += PM_HEAD_H + 12;
    pm_utext(buf, bw, x0 + 24, y + 2, hk_tr(S_T_CTRL), GF_GOLD);
    pm_rect(buf, bw, x0 + 30 + pm_uw(hk_tr(S_T_CTRL)), y + 2 + 7 * pm_h / 2, x1 - 24, y + 2 + 7 * pm_h / 2 + 2, 0x433a89u);
    y += cap;
    for (int k = 1; k < PM_NACT; k++) {
        const int c = (k - 1) / rows, r = (k - 1) % rows;
        const int lx = x0 + 40 + c * (colw + 40 + (boxw - 96 - 2 * colw - 40) / 2), rx = lx + colw;
        pm_utext(buf, bw, lx, y + r * PITCH, pm_act_name(k), GF_NORMAL);
        pm_rich(buf, bw, rx - pm_rich_w(val[0][k], 1), y + r * PITCH, val[0][k], GF_SEL, 1);
    }
    y += rows * PITCH + 10;
    pm_rect(buf, bw, x0 + 20, y, x1 - 20, y + 2, 0x433a89u);
    y += 12;
    pm_utext(buf, bw, x0 + 24, y + 2, tag, GF_GOLD);
    y += cap;
    for (int i = 0; i < nl; i++) { pm_rich(buf, bw, (bw - pm_rich_w(lines[i], 0)) / 2, y, lines[i], GF_NORMAL, 0); y += LH; }
    y += 10;
    pm_rect(buf, bw, x0 + 20, y, x1 - 20, y + 2, 0x433a89u);
    pm_rich(buf, bw, (bw - pm_rich_w(hint, 0)) / 2, y1 - 14 - PM_CELL, hint, GF_NORMAL, 0);
    return 1;
}
static void pm_tips_draw(uint32_t *buf, int bw, const PmTipsArg *a) {
    for (pm_h = g_pm_ui >= 4 ? 4 : 2; pm_h > 2 && !pm_tips_at(NULL, bw, a); pm_h -= 2) {}
    pm_tips_at(buf, bw, a);
}

// Message box (MessageBoxA on the Vita): title, text lines and the answers with their button symbols (l2 NULL: one
// answer), in the pause menu's box.
static void pm_msgbox_at(uint32_t *buf, int bw, const char *title, const char *text, const char *l1, const char *l2, int fit) {
    char lines[10][128];
    int nl = 0, tw = 0;
    const int lh = PM_CELL + 4;
    for (const char *p = text ? text : ""; *p && nl < 10; ) {   // one screen line per text line, '\r' dropped
        int n = 0;
        while (*p && *p != '\n') { if (*p != '\r' && n < (int)sizeof lines[0] - 1) lines[nl][n++] = *p; p++; }
        if (*p == '\n') p++;
        lines[nl][n] = 0;
        if (hk_gf_width_h(lines[nl], pm_h) > tw) tw = hk_gf_width_h(lines[nl], pm_h);
        nl++;
    }
    char ans[96];
    snprintf(ans, sizeof ans, l2 ? "\001 %s      \002 %s" : "\001 %s", l1, l2 ? l2 : "");
    if (pm_rich_w(ans, 1) > tw) tw = pm_rich_w(ans, 1);
    const int title_h = title && *title ? PM_HEAD_H + 14 : 0;
    if (title_h && hk_gf_width_h(title, pm_h + 2) > tw) tw = hk_gf_width_h(title, pm_h + 2);
    if (fit && tw + 96 > bw - 16 && pm_h > 2) { pm_h -= 2; pm_msgbox_at(buf, bw, title, text, l1, l2, fit); return; }
    int boxw = tw + 96 < 320 ? 320 : tw + 96 > bw - 16 ? bw - 16 : tw + 96, boxh = 18 + title_h + nl * lh + 14 + 2 + 12 + PM_CELL + 18;
    int x0 = (bw - boxw) / 2, y0 = (HK_CANVAS_H(bw) - boxh) / 2, x1 = x0 + boxw, y1 = y0 + boxh;
    pm_panel(buf, bw, x0, y0, x1, y1);
    int y = y0 + 18;
    if (title_h) { pm_heading(buf, bw, bw / 2, y, title); y += title_h; }
    for (int i = 0; i < nl; i++) { hk_gf_text_h(buf, bw, (bw - hk_gf_width_h(lines[i], pm_h)) / 2, y, lines[i], GF_SEL, pm_h); y += lh; }
    pm_rect(buf, bw, x0 + 20, y1 - 18 - PM_CELL - 12, x1 - 20, y1 - 18 - PM_CELL - 10, 0x433a89u);
    pm_rich(buf, bw, (bw - pm_rich_w(ans, 1)) / 2, y1 - 18 - PM_CELL, ans, GF_SEL, 1);
}
static void pm_msgbox(uint32_t *buf, int bw, const char *title, const char *text, const char *l1, const char *l2) {
    pm_h = g_pm_ui >= 4 ? 4 : 2;
    pm_msgbox_at(buf, bw, title, text, l1, l2, 1);
}

// ---- Overlay: the menus at the native output resolution ----
// The Vita presents the game frame (640x480 or 848x480) scaled and filtered to 960x544; text drawn into that frame came
// out blurry. The menus are instead drawn on a separate PM_OW x PM_OH canvas that the platform layer shows 1:1 over the
// frame (an ARGB texture with alpha blending, nearest neighbour), after darkening the frame with a translucent black
// rectangle. The draw functions above paint opaque colours and blend over what is there (the panel's fill), so the
// canvas is rendered twice, over black and over white; the difference gives each pixel's alpha and the black pass its
// premultiplied colour (the same trick the RESULTS layer used for the frame).
#define PM_OW 960
#define PM_OH 544
typedef void (*pm_draw_fn)(uint32_t *buf, const void *arg);
static uint32_t pm_tmp[2][PM_OW * PM_OH];   // black and white passes (shared, 4 MB)
// Renders fn into ovl (PM_OW x PM_OH, non-premultiplied ARGB: SDL_BLENDMODE_BLEND), only inside the rect (x0, y0, w, h)
// (the rest stays transparent).
static void pm_overlay_build(uint32_t *ovl, pm_draw_fn fn, const void *arg, int rx, int ry, int rw, int rh) {
    memset(ovl, 0, sizeof(uint32_t) * PM_OW * PM_OH);
    if (rx < 0) { rw += rx; rx = 0; }
    if (ry < 0) { rh += ry; ry = 0; }
    if (rx + rw > PM_OW) rw = PM_OW - rx;
    if (ry + rh > PM_OH) rh = PM_OH - ry;
    if (rw <= 0 || rh <= 0) return;
    for (int j = 0; j < 2; j++) {
        for (int y = ry; y < ry + rh; y++)
            for (int x = rx; x < rx + rw; x++) pm_tmp[j][y * PM_OW + x] = j ? 0xffffffu : 0;
        fn(pm_tmp[j], arg);
    }
    for (int y = ry; y < ry + rh; y++)
        for (int x = rx; x < rx + rw; x++) {
            uint32_t c = pm_tmp[0][y * PM_OW + x], wv = pm_tmp[1][y * PM_OW + x];
            uint32_t a = 255u - (((wv >> 8) & 255u) - ((c >> 8) & 255u));   // green channel: alpha = 255 - weight of the background
            if (((wv >> 8) & 255u) < ((c >> 8) & 255u)) a = 255u;
            if (!a) continue;
            uint32_t r = (c >> 16) & 255u, g = (c >> 8) & 255u, b = c & 255u;
            if (a < 255u) { r = r * 255u / a; g = g * 255u / a; b = b * 255u / a; if (r > 255u) r = 255u; if (g > 255u) g = 255u; if (b > 255u) b = 255u; }
            ovl[y * PM_OW + x] = a << 24 | r << 16 | g << 8 | b;
        }
}
// CPU reference of what the platform layer does on the GPU: the frame (w x h, pitch in pixels) nearest-scaled to the
// largest centred size that keeps its aspect (or stretched), darkened by dim/255 toward black, with the overlay blended
// over it. Used by the PC preview tool and the PC build's screenshots; out is PM_OW x PM_OH XRGB.
static void pm_composite(uint32_t *out, const uint32_t *frame, int w, int h, int pitch, int stretch, int dim, const uint32_t *ovl) {
    int dw = PM_OW, dh = PM_OH;
    if (!stretch) { if (dw * h > dh * w) dw = dh * w / h; else dh = dw * h / w; }
    const int ox = (PM_OW - dw) / 2, oy = (PM_OH - dh) / 2;
    for (int y = 0; y < PM_OH; y++)
        for (int x = 0; x < PM_OW; x++) {
            uint32_t p = 0;
            if (frame && x >= ox && x < ox + dw && y >= oy && y < oy + dh) p = frame[((y - oy) * h / dh) * pitch + (x - ox) * w / dw] & 0xffffffu;
            if (dim) p = ((((p & 0xff00ffu) * (uint32_t)(255 - dim)) / 255u) & 0xff00ffu) | ((((p >> 8) & 255u) * (uint32_t)(255 - dim) / 255u) << 8);
            uint32_t o = ovl ? ovl[y * PM_OW + x] : 0, a = o >> 24;
            if (a) {
                uint32_t r = ((p >> 16 & 255u) * (255 - a) + (o >> 16 & 255u) * a) / 255u, g = ((p >> 8 & 255u) * (255 - a) + (o >> 8 & 255u) * a) / 255u, b = ((p & 255u) * (255 - a) + (o & 255u) * a) / 255u;
                p = r << 16 | g << 8 | b;
            }
            out[y * PM_OW + x] = p;
        }
}

typedef struct { int screen, sel; PmState st; } PmScreenArg;
static void pm_draw_screen(uint32_t *buf, const void *arg) { const PmScreenArg *a = arg; pm_render(buf, PM_OW, a->screen, a->sel, &a->st); }
typedef struct { const char *title, *text, *l1, *l2; } PmMsgArg;
static void pm_draw_msgbox(uint32_t *buf, const void *arg) { const PmMsgArg *a = arg; pm_msgbox(buf, PM_OW, a->title, a->text, a->l1, a->l2); }
static void pm_draw_tips(uint32_t *buf, const void *arg) { pm_tips_draw(buf, PM_OW, arg); }
static void pm_draw_results(uint32_t *buf, const void *arg) { pm_res_scale(); pm_results_draw(buf, PM_OW, arg); }

static void pm_overlay_screen(uint32_t *ovl, int screen, int sel, const PmState *st) {
    PmScreenArg a = {screen, sel, *st};
    pm_overlay_build(ovl, pm_draw_screen, &a, 0, 0, PM_OW, PM_OH);
}
static void pm_overlay_msgbox(uint32_t *ovl, const char *title, const char *text, const char *l1, const char *l2) {
    PmMsgArg a = {title, text, l1, l2};
    pm_overlay_build(ovl, pm_draw_msgbox, &a, 0, 0, PM_OW, PM_OH);
}
// Tips card: a full-canvas build (drawn over the unchanged frame, like RESULTS); the caller rebuilds on page / map changes.
static void pm_overlay_tips(uint32_t *ovl, const PmTipsArg *a) { pm_overlay_build(ovl, pm_draw_tips, a, 0, 0, PM_OW, PM_OH); }
// RESULTS from the pause menu: uncached, into any overlay buffer.
static void pm_overlay_results_now(uint32_t *ovl, const uint32_t *v) {
    pm_res_scale();
    pm_overlay_build(ovl, pm_draw_results, v, (PM_OW - PM_RES_W) / 2, (PM_OH - PM_RES_H) / 2, PM_RES_W, PM_RES_H);
}
// RESULTS: rebuilt only when a shown value, the language or the scale changes (TIME: once a second). Returns 1 when
// ovl was rebuilt (the caller uploads it), 0 when it still holds the current panel.
static int pm_overlay_results(uint32_t *ovl, const uint32_t *v) {
    static uint32_t key[16];
    static int valid;
    pm_res_scale();
    uint32_t k[16] = {v[0] / 25u, (uint32_t)g_hk_lang, (uint32_t)pm_h};
    for (int i = 2; i < 13; i++) k[i + 2] = v[i];
    if (valid && !memcmp(k, key, sizeof k)) return 0;
    pm_overlay_build(ovl, pm_draw_results, v, (PM_OW - PM_RES_W) / 2, (PM_OH - PM_RES_H) / 2, PM_RES_W, PM_RES_H);
    memcpy(key, k, sizeof k); valid = 1;
    return 1;
}
#endif
