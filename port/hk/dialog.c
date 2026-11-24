// Win32 dialogs for the recompiled Handkerchief, drawn by the port over the game screen and driven by the keyboard
// or the pad. The game's own dialog procedures run unchanged: the port reads the DIALOG template from HDKWIN.EXE,
// sends WM_INITDIALOG / WM_COMMAND / WM_DESTROY, and answers the control functions the game imports
// (Get/SetDlgItemText/Int, CheckDlgButton, SendDlgItemMessageA with button, combo box and list box messages).
//
// Dialogs used by the game (tools/dump_dialogs.py): 1498 "Exit Handkerchief ?" (Esc, modeless), 1491 Game
// Configuration and 1492 Key Configuration (F2, modal), 1490 secret options, 1493/1497/1499 messages and input.
//
// Controls: arrows move the focus between controls (spatially, for a pad); space or X (pad Cross) press the focused
// control (a combo box opens its list); Enter or C (pad Triangle) press the default (OK) button;
// Esc or Z (pad Circle, Select) send IDCANCEL. The game's menu bar (LoadMenuA/SetMenu) is drawn and driven the same
// way; F10 (pad: Select on the pause screen) asks the game to show it.
#include "win32.h"
#include "plat.h"
#include "hk/font_data.h"
#include "hk/theme.h"
#include "hk/gameart.h"
#include "hk/i18n.h"
#include "safe_write.h"
#include <SDL.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_DLG 8
#define MAX_CTL 72
#define MAX_ITEMS 160
enum { C_BUTTON, C_EDIT, C_STATIC, C_LISTBOX, C_COMBO, C_OTHER };

typedef struct {
    uint16_t id;
    int cls;
    uint32_t style;
    int x, y, w, h;          // pixels, relative to the dialog client area
    char text[160];
    int checked, enabled, visible;
    int nitems, cursel, top;
    int rh;                  // list row height
    int limit;               // EM_LIMITTEXT (0: the port's 159-character buffer)
    char (*items)[48];
    uint32_t *itemdata;
} Ctl;

typedef struct {
    int used, modal, ended, visible, res;
    uint32_t hwnd, proc, result;
    char title[96];
    int x, y, w, h;          // pixels on the 640x480 screen, including the title bar
    Ctl ctl[MAX_CTL];
    int n, focus;
    int big;                 // the template was scaled up (>= 125%): 18 px font, taller rows
    int drop, drop_orig;     // the focused combo box shows its list; selection to restore on cancel
} Dlg;

static Dlg g_dlg[MAX_DLG];
#define DLG_HWND(k) (0x00030000u | ((uint32_t)(k) << 8))
#define TITLE_H 26
#define DU_X(v) ((v) * 3 / 2)   // dialog units -> pixels: MS P Gothic 9pt has ~6x12 base units
#define DU_Y(v) ((v) * 3 / 2)

static Dlg *dlg_of(uint32_t hwnd) {
    if ((hwnd & 0xffff00ffu) != 0x00030000u) return NULL;
    unsigned k = (hwnd >> 8) & 0xffu;
    return k < MAX_DLG && g_dlg[k].used ? &g_dlg[k] : NULL;
}
static Ctl *ctl_of_hwnd(uint32_t hwnd) {
    if ((hwnd & 0xffff0000u) != 0x00030000u) return NULL;
    unsigned k = (hwnd >> 8) & 0xffu, i = hwnd & 0xffu;
    if (k >= MAX_DLG || !g_dlg[k].used || i == 0 || (int)i > g_dlg[k].n) return NULL;
    return &g_dlg[k].ctl[i - 1];
}
static Ctl *ctl_by_id(Dlg *d, uint32_t id) {
    if (!d) return NULL;
    for (int i = 0; i < d->n; i++) if (d->ctl[i].id == (uint16_t)id) return &d->ctl[i];
    return NULL;
}
static uint32_t ctl_hwnd(Dlg *d, Ctl *t) { return d->hwnd | (uint32_t)(t - d->ctl + 1); }
int hk_dlg_is(uint32_t hwnd) { return dlg_of(hwnd) || ctl_of_hwnd(hwnd); }

// Topmost visible dialog (the one that gets input).
static Dlg *top_dlg(void) {
    for (int k = MAX_DLG - 1; k > 0; k--) if (g_dlg[k].used && g_dlg[k].visible) return &g_dlg[k];
    return NULL;
}

// ---------- menu bar ----------
// HDKWIN.EXE loads a MENU resource per state (1505 title, 1504 in a stage, 1502 with Network, 1503 the developers'
// editor) and, in full screen, shows it only on demand: a right click (WM_RBUTTONDOWN, window procedure 0x40ED30)
// or WM_INITMENU calls 0x40EEE9, which pauses the game and SetMenu(hwnd, menu); WM_COMMAND (0x40ED65 -> 0x40EF4A)
// hides it again with SetMenu(hwnd, 0) and leaves the command id in the window object for the main loop.
// The port draws the menu set by SetMenu as a bar at the top of the screen; the keyboard or pad picks an item, which
// is posted as WM_COMMAND; cancelling posts another right click, the game's own toggle. F10 (pad: Select on the
// port's pause screen) opens it with a right click.
static void strip_amp(char *s);
#define MENU_HANDLE(res) (0x00050000u | ((res) & 0xffffu))
#define MAX_MTOP 10
#define MAX_MITEM 24
typedef struct { char text[64], key[16]; uint16_t id; int grayed, sep; } MItem;
typedef struct { char text[64]; uint16_t id; int popup, grayed, n; MItem it[MAX_MITEM]; } MTop;
static struct {
    uint32_t res;                 // resource shown, 0 = no menu bar
    int ntop, top, item, open;    // selected title; selected item and whether its drop-down is open
    MTop t[MAX_MTOP];
} g_menu;
static void menu_text(const uint16_t **w, char *out, size_t cap, char *key, size_t kcap) {
    size_t n = 0, k = 0;
    int tab = 0;
    for (; **w; (*w)++) {
        uint16_t ch = **w;
        if (ch == '\t') { tab = 1; continue; }
        char c8 = (char)(ch < 256 ? ch : '?');
        if (tab) { if (key && k + 1 < kcap) key[k++] = c8; }
        else if (n + 1 < cap) out[n++] = c8;
    }
    (*w)++;
    out[n] = 0;
    if (key && kcap) key[k] = 0;
}
// Standard MENU template: (flags, [id], text) items; MF_POPUP (0x10) opens a sub-list, MF_END (0x80) closes one.
// Nested popups (the editor menu) are flattened into their top-level drop-down.
static int menu_parse(uint32_t res) {
    HOSTFN;
    uint32_t size, ga = w32_resource(res, 4, &size);   // RT_MENU
    if (!ga || size < 8) { rt_log("[menu] menu %u not found\n", res); return 0; }
    const uint16_t *w = (const uint16_t *)(mb + ga + 4), *end = (const uint16_t *)(mb + ga + size);
    memset(&g_menu.t, 0, sizeof g_menu.t);
    g_menu.ntop = 0;
    int lvl = 0, ends[8] = {0};   // ends[l]: the popup that opened level l+1 also ended level l
    while (w < end && lvl >= 0) {
        uint16_t fl = *w++, id = 0;
        if (!(fl & 0x10)) id = *w++;
        char text[64], key[16];
        menu_text(&w, text, sizeof text, key, sizeof key);
        strip_amp(text);
        if (lvl == 0) {
            if (g_menu.ntop < MAX_MTOP) {
                MTop *t = &g_menu.t[g_menu.ntop++];
                snprintf(t->text, sizeof t->text, "%s", text);
                t->id = id; t->popup = (fl & 0x10) != 0; t->grayed = (fl & 3) != 0;
            }
        } else if (!(fl & 0x10) && g_menu.ntop && g_menu.t[g_menu.ntop - 1].n < MAX_MITEM) {
            MTop *t = &g_menu.t[g_menu.ntop - 1];
            MItem *m = &t->it[t->n++];
            snprintf(m->text, sizeof m->text, "%s", text);
            snprintf(m->key, sizeof m->key, "%s", key);
            m->id = id; m->grayed = (fl & 3) != 0; m->sep = !id && !text[0];
            // The port is always full screen and has no DirectPlay: Toggle Display Mode (windowed mode lost the
            // palette: grey picture) and Network are shown greyed out.
            if (id == 40064 || id == 40059) m->grayed = 1;
        }
        if (fl & 0x10) { if (lvl < 7) ends[lvl] = (fl & 0x80) != 0; lvl++; }
        else if (fl & 0x80) { lvl--; while (lvl >= 0 && lvl < 8 && ends[lvl]) { ends[lvl] = 0; lvl--; } }
    }
    return g_menu.ntop > 0;
}
static int menu_shown(void) { return g_menu.res != 0; }
static int mtop_ok(int i) { return i >= 0 && i < g_menu.ntop && !g_menu.t[i].grayed && g_menu.t[i].text[0]; }
static int mitem_ok(const MTop *t, int i) { return i >= 0 && i < t->n && !t->it[i].grayed && !t->it[i].sep; }
static void menu_set(uint32_t hmenu) {
    uint32_t res = (hmenu & 0xffff0000u) == 0x00050000u ? hmenu & 0xffffu : 0;
    rt_log("[menu] SetMenu(%u)\n", res);
    if (res && res != g_menu.res && !menu_parse(res)) res = 0;
    if (res && !menu_shown()) {
        g_menu.top = 0;
        while (g_menu.top < g_menu.ntop && !mtop_ok(g_menu.top)) g_menu.top++;
        g_menu.open = 0; g_menu.item = 0;
    }
    g_menu.res = res;
}
int hk_dlg_count(void) { int n = 0; for (int i = 0; i < MAX_DLG; i++) n += g_dlg[i].used != 0; return n; }   // memory log
int hk_dlg_top_res(void) { Dlg *d = top_dlg(); return d ? d->res : 0; }   // template id of the top dialog (stress driver)
int hk_menu_shown(void);
int hk_dlg_active(void) { return top_dlg() != NULL || menu_shown(); }
int hk_menu_shown(void) { return menu_shown(); }

// ---------- template ----------
static const uint8_t *res_sz(const uint8_t *p, char *out, size_t cap, int *ord) {
    const uint16_t *w = (const uint16_t *)p;
    if (ord) *ord = -1;
    if (w[0] == 0xffff) { if (ord) *ord = w[1]; if (out && cap) out[0] = 0; return p + 4; }
    size_t n = 0;
    for (; *w; w++) {
        uint16_t ch = *w;
        if (out && n + 1 < cap) out[n++] = (char)(ch < 256 ? ch : '?');
    }
    if (out && cap) out[n < cap ? n : cap - 1] = 0;
    return (const uint8_t *)(w + 1);
}
static void strip_amp(char *s) {   // "&OK" -> "OK", "&&" -> "&"
    char *o = s;
    for (char *p = s; *p; p++) {
        if (*p == '&') { if (p[1] == '&') p++; else continue; }
        *o++ = *p;
    }
    *o = 0;
}

// DIALOG and DIALOGEX templates (the About box 1516 and the save slots 1500 are DIALOGEX).
static int parse_template(Dlg *d, uint32_t ga) {
    HOSTFN;
    const uint8_t *p = mb + ga;
    uint32_t style; memcpy(&style, p, 4);
    int ex = style == 0xffff0001u;
    uint16_t n; int16_t cx, cy;
    if (ex) { memcpy(&style, p + 12, 4); memcpy(&n, p + 16, 2); memcpy(&cx, p + 22, 2); memcpy(&cy, p + 24, 2); p += 26; }
    else { memcpy(&n, p + 8, 2); memcpy(&cx, p + 14, 2); memcpy(&cy, p + 16, 2); p += 18; }
    p = res_sz(p, NULL, 0, NULL);                      // menu
    p = res_sz(p, NULL, 0, NULL);                      // class
    p = res_sz(p, d->title, sizeof d->title, NULL);    // title
    if (style & 0x40) { p += ex ? 6 : 2; p = res_sz(p, NULL, 0, NULL); }   // DS_SETFONT: size (weight, italic, charset), face
    // Scaled up to fill the screen (at most 150%): big text and rows for a handheld screen. Control ids and messages
    // are unchanged; only the geometry differs from the template.
    int sc = 100, w0 = DU_X(cx), h0 = DU_Y(cy);
    if (w0 > 0 && h0 > 0) { sc = 150; if (620 * 100 / w0 < sc) sc = 620 * 100 / w0; if ((480 - TITLE_H - 6 - 70) * 100 / h0 < sc) sc = (480 - TITLE_H - 6 - 70) * 100 / h0; if (sc < 100) sc = 100; }
    d->big = sc >= 125;
    d->w = w0 * sc / 100 + 6; d->h = h0 * sc / 100 + TITLE_H + 6;
    if (d->w > 636) d->w = 636;
    if (d->h > 476) d->h = 476;
    d->x = (640 - d->w) / 2; d->y = (480 - d->h) / 2;
    d->n = 0;
    for (int i = 0; i < n && d->n < MAX_CTL; i++) {
        p = mb + ((uint32_t)(p - mb + 3) & ~3u);
        Ctl *t = &d->ctl[d->n];
        memset(t, 0, sizeof *t);
        int16_t x, y, w, h; uint16_t id;
        if (ex) {   // helpID, exStyle, style, x, y, cx, cy, DWORD id
            memcpy(&t->style, p + 8, 4); memcpy(&x, p + 12, 2); memcpy(&y, p + 14, 2); memcpy(&w, p + 16, 2);
            memcpy(&h, p + 18, 2); memcpy(&id, p + 20, 2);
            p += 24;
        } else {
            memcpy(&t->style, p, 4); memcpy(&x, p + 8, 2); memcpy(&y, p + 10, 2); memcpy(&w, p + 12, 2);
            memcpy(&h, p + 14, 2); memcpy(&id, p + 16, 2);
            p += 18;
        }
        int ord; char cls[32];
        p = res_sz(p, cls, sizeof cls, &ord);
        p = res_sz(p, t->text, sizeof t->text, NULL);
        uint16_t extra; memcpy(&extra, p, 2); p += 2 + extra;
        t->cls = ord == 0x80 || !strcasecmp(cls, "button") ? C_BUTTON : ord == 0x81 || !strcasecmp(cls, "edit") ? C_EDIT
               : ord == 0x82 || !strcasecmp(cls, "static") ? C_STATIC : ord == 0x83 || !strcasecmp(cls, "listbox") ? C_LISTBOX
               : ord == 0x85 || !strcasecmp(cls, "combobox") ? C_COMBO : C_OTHER;
        t->id = id;
        t->x = DU_X(x) * sc / 100; t->y = DU_Y(y) * sc / 100; t->w = DU_X(w) * sc / 100;
        t->h = t->cls == C_COMBO ? (d->big ? 28 : 20) : DU_Y(h) * sc / 100;
        t->rh = d->big ? 22 : 14;
        if (t->cls == C_BUTTON && d->big && t->h < 26 && (t->style & 15u) != 7 && !(t->style & 0x1000u)) { t->y -= (26 - t->h) / 2; t->h = 26; }
        t->enabled = !(t->style & 0x08000000u);
        t->visible = (t->style & 0x10000000u) != 0;
        t->cursel = -1;
        strip_amp(t->text);
        d->n++;
    }
    return 1;
}

static int btype(const Ctl *t) { return (int)(t->style & 15u); }
static int focusable(const Ctl *t) {
    if (!t->visible || !t->enabled) return 0;
    if (t->cls == C_BUTTON) return btype(t) != 7;   // not a group box
    return t->cls == C_COMBO || t->cls == C_LISTBOX || t->cls == C_EDIT;
}

// ---------- drawing ----------
static int g_fi;   // font in use: 0 = 14 px, 1 = 18 px (hk_fonts)
static const HkFont *font(void) { return &hk_fonts[g_fi]; }
static int text_w(const char *s) {
    int w = 0;
    for (; *s; s++) { uint8_t ch = (uint8_t)*s; w += font()->glyphs[(ch < 0x20 ? '?' : ch) - 0x20].advance; }
    return w;
}
static void put_text(uint32_t *fb, int x, int y, const char *s, uint32_t color, int maxx) {
    const HkFont *f = font();
    for (; *s; s++) {
        uint8_t ch = (uint8_t)*s;
        const HkGlyph *g = &f->glyphs[(ch < 0x20 ? '?' : ch) - 0x20];
        if (x + g->advance > maxx) break;
        for (int gy = 0; gy < f->cell; gy++)
            for (int gx = 0; gx < g->width; gx++)
                if (((f->rows[g->offset + gy] >> gx) & 1u) && x + gx >= 0 && x + gx < 640 && y + gy >= 0 && y + gy < 480)
                    fb[(y + gy) * 640 + x + gx] = color;
        x += g->advance;
    }
}
static void fill(uint32_t *fb, int x, int y, int w, int h, uint32_t c) {
    for (int j = y < 0 ? 0 : y; j < y + h && j < 480; j++)
        for (int i = x < 0 ? 0 : x; i < x + w && i < 640; i++) fb[j * 640 + i] = c;
}
static void frame(uint32_t *fb, int x, int y, int w, int h, uint32_t tl, uint32_t br) {
    fill(fb, x, y, w, 1, tl); fill(fb, x, y, 1, h, tl);
    fill(fb, x, y + h - 1, w, 1, br); fill(fb, x + w - 1, y, 1, h, br);
}
// Theme (hk/theme.h): flat dark controls, purple frames, red dots on the focused one.
// The dialogs are drawn into a cached layer (g_layer, 640x480) that is rebuilt only when the dialog state changes, and
// composited over each game frame (hk_dlg_overlay). Layer pixel: top byte 0 = opaque colour; 0xFF = untouched (the
// game shows through); otherwise the top byte w is the weight of the game pixel (w/256) and the low 24 bits the
// premultiplied colour, so that out = game * w / 256 + colour (the translucent panel and what is drawn on it).
#define LAYER_CLEAR 0xFF000000u
static uint32_t g_layer[480 * 640];
static void blend(uint32_t *fb, int x0, int y0, int w, int h, uint32_t col, int a) {   // a/16 of col over what is there
    for (int y = y0 < 0 ? 0 : y0; y < y0 + h && y < 480; y++)
        for (int x = x0 < 0 ? 0 : x0; x < x0 + w && x < 640; x++) {
            uint32_t p = fb[y * 640 + x], r = 0;
            uint32_t top = p >> 24, wt = top == 0xFFu ? 256u : top;
            for (int sh = 0; sh < 24; sh += 8) {
                uint32_t pc = top == 0xFFu ? 0 : (p >> sh) & 255u;
                r |= ((pc * (uint32_t)(16 - a) + ((col >> sh) & 255u) * (uint32_t)a) / 16u) << sh;
            }
            uint32_t nw = wt * (uint32_t)(16 - a) / 16u;
            fb[y * 640 + x] = (top == 0 ? 0 : nw << 24) | r;   // opaque stays opaque (a mix of two opaque colours)
        }
}
static void dot(uint32_t *fb, int cx, int cy, int r, uint32_t col) {
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r + r / 2 && cx + x >= 0 && cx + x < 640 && cy + y >= 0 && cy + y < 480) fb[(cy + y) * 640 + cx + x] = col;
}
// A flat box with its corners cut by one pixel.
static void rbox(uint32_t *fb, int x, int y, int w, int h, uint32_t bg, uint32_t border) {
    fill(fb, x + 1, y, w - 2, h, bg); fill(fb, x, y + 1, w, h - 2, bg);
    fill(fb, x + 1, y, w - 2, 1, border); fill(fb, x + 1, y + h - 1, w - 2, 1, border);
    fill(fb, x, y + 1, 1, h - 2, border); fill(fb, x + w - 1, y + 1, 1, h - 2, border);
}
// The title menu's red dot (HDKSYS00 (0,0), hk/gameart.h) centred at cx, cy.
static void red_dot(uint32_t *fb, int cx, int cy) { hk_art_dot(fb, 640, cx - 4, cy - 4); }
// Text in the game's font, upper case like the title menu words (hk/gameart.h). Returns the width.
static int gtext(uint32_t *fb, int x, int y, const char *s, int style) {
    char u[160];
    hk_gf_upper(u, s, sizeof u);
    return fb ? hk_gf_text(fb, 640, x, y, u, style, 1) : hk_gf_width(u, 1);
}
static int gtext_w(const char *s) { return gtext(NULL, 0, 0, s, 0); }
// The title screen's menu box around x, y, w, h over a dark translucent inside.
static void art_box(uint32_t *fb, int x, int y, int w, int h) {
    blend(fb, x + 3, y + 3, w - 6, h - 6, HK_TH_FILL, 13);
    hk_art_frame(fb, 640, x, y, w, h);
}
// Marks the focused control: purple outline (two pixels) and a red dot at each side.
static void focus_mark(uint32_t *fb, int x, int y, int w, int h, int dots) {
    frame(fb, x - 1, y - 1, w + 2, h + 2, HK_TH_BORDER, HK_TH_BORDER);
    frame(fb, x - 2, y - 2, w + 4, h + 4, HK_TH_BORDER_IN, HK_TH_BORDER_IN);
    if (dots) { red_dot(fb, x + 7, y + h / 2); red_dot(fb, x + w - 8, y + h / 2); }
}

// Static text as Windows draws it: word-wrapped to the control width, '\n' breaks lines, aligned left (0), centre (1)
// or right (2); vertically centred when vc is set. A word wider than the control is not split (it may spill over).
// The port's font is wider than the game's MS P Gothic, so a control only one line high is never wrapped: its text
// may spill a little past the control instead of being cut.
static void text_box(uint32_t *fb, int x, int y, int w, int h, const char *s, int align, int vc, uint32_t ink) {
    char lines[8][160];
    int nl = 0, w0 = w;
    if (h < 2 * font()->cell) w = 1 << 20;
    const char *p = s;
    while (*p && nl < 8) {
        char *o = lines[nl];
        int len = 0, last_space = -1;
        const char *q = p, *brk = NULL;
        for (; *q && *q != '\n' && len < 159; q++) {
            o[len] = *q; o[len + 1] = 0;
            if (*q == ' ') last_space = len;
            if (text_w(o) > w && last_space > 0) { o[last_space] = 0; brk = p + last_space + 1; break; }
            len++;
        }
        if (!brk) { o[len] = 0; brk = *q == '\n' ? q + 1 : q; }
        nl++;
        p = brk;
    }
    int lh = font()->cell, top = vc ? y + (h - nl * lh) / 2 : y;
    for (int i = 0; i < nl; i++) {
        int lw = text_w(lines[i]), tx = align == 1 ? x + (w0 - lw) / 2 : align == 2 ? x + w0 - lw : x;
        if (tx < x - 20 && align != 1) tx = x;
        put_text(fb, tx, top + i * lh, lines[i], ink, 640);
    }
}

// EDIT controls take text: typed on a keyboard while focused (PC), or through the system keyboard (Vita IME),
// opened by pressing the control (Cross). ES_READONLY (0x800) controls only show text.
static Ctl *g_ime_ctl;   // the EDIT the system keyboard is editing
static int editing(const Ctl *t) { return t->cls == C_EDIT && t->enabled && t->visible && !(t->style & 0x800u); }
static int edit_limit(const Ctl *t) { return t->limit > 0 && t->limit < 159 ? t->limit : 159; }
static void set_edit_text(Ctl *t, const char *s) {
    snprintf(t->text, sizeof t->text, "%s", s);
    if ((int)strlen(t->text) > edit_limit(t)) t->text[edit_limit(t)] = 0;
}

static const char *item_text(const Ctl *t, int i) { return i >= 0 && i < t->nitems ? t->items[i] : ""; }

// Uses the 18 px font when the control is tall enough and the text fits, else the 14 px one.
static void pick_font(const Ctl *t, const char *s, int avail) {
    g_fi = 0;
    if (t->h >= 20 || t->rh > 14) { g_fi = 1; if (text_w(s) > avail) g_fi = 0; }
}

static void draw_ctl(uint32_t *fb, const Dlg *d, const Ctl *t, int focused) {
    int x = d->x + 3 + t->x, y = d->y + TITLE_H + 3 + t->y, w = t->w, h = t->h;
    uint32_t ink = t->enabled ? HK_TH_TEXT : HK_TH_DIM;
    g_fi = d->big;
    int ty = y + (h - font()->cell) / 2;
    switch (t->cls) {
    case C_BUTTON:
        switch (btype(t)) {
        case 7: {   // group box: frame with a gap for the heading
            int gf = t->text[0] && gtext_w(t->text) <= w - 24;   // heading in the game's font when it fits
            int tw = t->text[0] ? (gf ? gtext_w(t->text) : text_w(t->text)) + 6 : 0;
            fill(fb, x, y + 6, 6, 1, HK_TH_BORDER_IN); fill(fb, x + 6 + tw, y + 6, w - 6 - tw, 1, HK_TH_BORDER_IN);
            fill(fb, x, y + 6, 1, h - 6, HK_TH_BORDER_IN); fill(fb, x + w - 1, y + 6, 1, h - 6, HK_TH_BORDER_IN);
            fill(fb, x, y + h - 1, w, 1, HK_TH_BORDER_IN);
            if (gf) gtext(fb, x + 9, y - 1, t->text, t->enabled ? GF_GOLD : GF_DIM);
            else if (t->text[0]) put_text(fb, x + 9, y, t->text, t->enabled ? HK_TH_HEAD : HK_TH_DIM, x + w);
            break;
        }
        case 2: case 3: case 4: case 9: {   // check box / radio button
            if (t->style & 0x1000u) {      // BS_PUSHLIKE (save slots 1500): a dark row, lit while checked
                rbox(fb, x, y, w, h, t->checked ? HK_TH_BAR : HK_TH_BTN, t->checked ? HK_TH_BORDER : HK_TH_BORDER_IN);
                pick_font(t, t->text, w - 10);
                text_box(fb, x + 3, y, w - 6, h, t->text, 1, 1, focused || t->checked ? HK_TH_SEL : ink);
                if (focused) focus_mark(fb, x, y, w, h, 1);
                break;
            }
            pick_font(t, t->text, w - 20);
            ty = y + (h - font()->cell) / 2;
            int r = btype(t) == 4 || btype(t) == 9, by = y + (h - 12) / 2;
            if (focused) blend(fb, x - 3, ty - 1, text_w(t->text) + 24, font()->cell + 2, HK_TH_BORDER, 6);
            if (r) {   // round mark
                dot(fb, x + 6, by + 6, 6, HK_TH_BORDER);
                dot(fb, x + 6, by + 6, 5, HK_TH_INK);
                if (t->checked) dot(fb, x + 6, by + 6, 3, HK_TH_HEAD);
            } else {
                fill(fb, x, by, 12, 12, HK_TH_BORDER);
                fill(fb, x + 1, by + 1, 10, 10, HK_TH_INK);
                if (t->checked) fill(fb, x + 3, by + 3, 6, 6, HK_TH_HEAD);
            }
            put_text(fb, x + 17, ty, t->text, focused ? HK_TH_SEL : ink, x + w + 40);
            if (focused) red_dot(fb, x - 6, y + h / 2);
            break;
        }
        default:  // push button
            pick_font(t, t->text, w - 10);
            ty = y + (h - font()->cell) / 2;
            rbox(fb, x, y, w, h, focused ? HK_TH_BAR : HK_TH_BTN, btype(t) == 1 || focused ? HK_TH_BORDER : HK_TH_BORDER_IN);
            int gw = hk_gf_width(t->text, 1), gf = gw <= w - 8 && h >= 14;   // the game's font (mixed case) when it fits
            if (gf) hk_gf_text(fb, 640, x + (w - gw) / 2, y + h / 2 - 7, t->text, !t->enabled ? GF_DIM : focused ? GF_SEL : btype(t) == 1 ? GF_GOLD : GF_NORMAL, 1);
            else put_text(fb, x + (w - text_w(t->text)) / 2, ty, t->text, focused ? HK_TH_SEL : btype(t) == 1 ? HK_TH_HEAD : ink, x + w);
            if (focused) focus_mark(fb, x, y, w, h, (gf ? gw : text_w(t->text)) + 28 <= w);
        }
        break;
    case C_STATIC:
        if ((t->style & 0x1fu) == 0x12u) { frame(fb, x, y, w, h, HK_TH_BORDER_IN, HK_TH_BORDER_IN); break; }   // SS_ETCHEDFRAME
        if ((t->style & 0x1fu) >= 3 && (t->style & 0x1fu) <= 9) break;                         // icon, rectangles, frames
        if ((t->style & 0x1fu) == 0xEu) break;                                                 // SS_BITMAP
        text_box(fb, x, y, w, h, t->text, (int)(t->style & 3u), (t->style & 0x200u) != 0, ink);   // SS_CENTERIMAGE: vertically centred
        break;
    case C_EDIT: {
        pick_font(t, t->text, w - 8);
        ty = y + (h - font()->cell) / 2;
        rbox(fb, x, y, w, h, HK_TH_INK, focused ? HK_TH_BORDER : HK_TH_BORDER_IN);
        int a = (int)(t->style & 3u), tw = text_w(t->text);   // ES_CENTER 1, ES_RIGHT 2
        int tx = a == 1 ? x + (w - tw) / 2 : a == 2 ? x + w - 4 - tw : x + 4;
        if (tx < x + 4) tx = x + 4;
        put_text(fb, tx, ty, t->text, ink, x + w - 3);
        if (focused && editing(t)) fill(fb, tx + tw + 1 < x + w - 3 ? tx + tw + 1 : x + w - 4, ty + 1, 2, font()->cell - 2, HK_TH_SEL);   // caret
        if (focused) focus_mark(fb, x, y, w, h, 0);
        break;
    }
    case C_COMBO: {   // "< value >"
        const char *v = item_text(t, t->cursel);
        char buf[64];
        snprintf(buf, sizeof buf, "< %s >", v);
        pick_font(t, buf, w - 8);
        ty = y + (h - font()->cell) / 2;
        rbox(fb, x, y, w, h, focused ? HK_TH_BAR : HK_TH_BTN, focused ? HK_TH_BORDER : HK_TH_BORDER_IN);
        if (text_w(buf) > w - 6) snprintf(buf, sizeof buf, "%s", v);
        put_text(fb, x + (w - text_w(buf)) / 2, ty, buf, focused ? HK_TH_SEL : ink, x + w - 2);
        if (focused) focus_mark(fb, x, y, w, h, 0);
        break;
    }
    case C_LISTBOX: {
        rbox(fb, x, y, w, h, HK_TH_INK, focused ? HK_TH_BORDER : HK_TH_BORDER_IN);
        int rh = t->rh, rows = (h - 4) / rh;
        g_fi = rh > 14;
        for (int i = 0; i < rows && t->top + i < t->nitems; i++) {
            int it = t->top + i;
            if (it == t->cursel) { fill(fb, x + 2, y + 2 + i * rh, w - 4, rh, HK_TH_BAR); red_dot(fb, x + 8, y + 2 + i * rh + rh / 2); }
            put_text(fb, x + 16, y + 2 + i * rh + (rh - font()->cell) / 2, t->items[it], it == t->cursel ? HK_TH_SEL : ink, x + w - 2);
        }
        if (focused) focus_mark(fb, x, y, w, h, 0);
        break;
    }
    default: break;
    }
    g_fi = 0;
}

// The open list of a combo box, below it (above if it would leave the screen), scrolled to the selection.
static void draw_drop(uint32_t *fb, const Dlg *d, const Ctl *t) {
    int rows = t->nitems < 10 ? t->nitems : 10, rh = t->rh, w = t->w, h = rows * rh + 4;
    int x = d->x + 3 + t->x, y = d->y + TITLE_H + 3 + t->y + t->h;
    if (y + h > 478) y = d->y + TITLE_H + 3 + t->y - h;
    int first = t->cursel - rows / 2;
    if (first > t->nitems - rows) first = t->nitems - rows;
    if (first < 0) first = 0;
    rbox(fb, x, y, w, h, HK_TH_INK, HK_TH_BORDER);
    g_fi = rh > 14;
    for (int i = 0; i < rows; i++) {
        int it = first + i, sel = it == t->cursel;
        if (sel) { fill(fb, x + 2, y + 2 + i * rh, w - 4, rh, HK_TH_BAR); red_dot(fb, x + 8, y + 2 + i * rh + rh / 2); }
        put_text(fb, x + 16, y + 2 + i * rh + (rh - font()->cell) / 2, t->items[it], sel ? HK_TH_SEL : HK_TH_TEXT, x + w - 2);
    }
    g_fi = 0;
}

// Called by the DirectDraw present with the converted 640x480 frame.
// Hints: on the Vita (and on PC with HK_PAD_HINTS=1) the pad buttons are drawn as symbols; markers: 1 cross,
// 2 circle, 3 triangle, 4 square, 5 L, 6 R, 7 D-pad. On PC the keyboard keys are spelled out.
static int pad_hints(void) {
#ifdef __vita__
    return 1;
#else
    static int v = -1;
    if (v < 0) v = getenv("HK_PAD_HINTS") != NULL;
    return v;
#endif
}
#define HINT(id) hk_tr(pad_hints() ? id##_PAD : id##_PC)
#define HINT_BAR HINT(S_D_BAR)
#define HINT_DROP HINT(S_D_DROP)
#define HINT_DLG HINT(S_D_DLG)
#define HINT_LIST HINT(S_D_LIST)
#define HINT_EDIT HINT(S_D_EDIT)
#define HINT_GAME HINT(S_D_GAME)
// Hint line: the markers become button symbols, the words are in the game's font (as the pause menu).
static int hint_run(uint32_t *fb, const char *hint, int x, int y) {
    char run[160];
    int n = 0, x0 = x;
    for (const char *p = hint;; p++) {
        uint8_t c = (uint8_t)*p;
        if ((c && c < 8) || !c || n >= (int)sizeof run - 1) {
            run[n] = 0;
            if (n) { if (fb) hk_gf_text(fb, 640, x, y, run, GF_NORMAL, 1); x += hk_gf_width(run, 1); }
            n = 0;
        }
        if (!c) break;
        if (c >= 8) { run[n++] = (char)c; continue; }
        if (c <= 4) { if (fb) hk_th_glyph(fb, 640, x + 1, y - 1, c); x += 19; }
        else if (c <= 6) {
            if (fb) { rbox(fb, x + 1, y, 19, 14, HK_TH_BTN, HK_TH_BORDER_IN); hk_gf_text(fb, 640, x + 7, y - 1, c == 5 ? "L" : "R", GF_SEL, 1); }
            x += 23;
        } else { if (fb) { fill(fb, x + 3, y + 5, 13, 3, HK_TH_TEXT); fill(fb, x + 8, y, 3, 13, HK_TH_TEXT); } x += 20; }
    }
    return x - x0;
}
static void hint_pill(uint32_t *fb, const char *hint, int hy) {
    int tw = hint_run(NULL, hint, 0, 0), hw = tw + 40;
    if (hy > 446) hy = 446;
    art_box(fb, (640 - hw) / 2, hy - 3, hw, 32);
    hint_run(fb, hint, (640 - tw) / 2, hy + 6);
}
static void menu_draw(uint32_t *fb) {
    const int bh = 28;
    blend(fb, 0, 0, 640, bh, HK_TH_FILL, 14);
    fill(fb, 0, bh - 3, 640, 1, HK_TH_BORDER_IN); fill(fb, 0, bh - 2, 640, 1, HK_TH_BORDER); fill(fb, 0, bh - 1, 640, 1, 0x2c1e6du);
    int x = 6, top_x = 6, has_dlg = top_dlg() != NULL;
    for (int i = 0; i < g_menu.ntop; i++) {
        MTop *t = &g_menu.t[i];
        int w = gtext_w(t->text) + 40, sel = i == g_menu.top && !has_dlg;
        if (sel) { red_dot(fb, x + 10, 12); red_dot(fb, x + w - 10, 12); top_x = x; }
        gtext(fb, x + 20, 5, t->text, sel ? GF_SEL : t->grayed ? GF_DIM : GF_NORMAL);
        x += w;
    }
    if (has_dlg) return;
    if (!g_menu.open || g_menu.top >= g_menu.ntop || !g_menu.t[g_menu.top].popup) { hint_pill(fb, HINT_BAR, 446); return; }
    MTop *t = &g_menu.t[g_menu.top];
    int w = 0;
    for (int i = 0; i < t->n; i++) { int iw = gtext_w(t->it[i].text) + (t->it[i].key[0] ? gtext_w(t->it[i].key) + 30 : 0); if (iw > w) w = iw; }
    w += 64;
    int h = 28, px = top_x + w > 636 ? 636 - w : top_x, py = bh;
    for (int i = 0; i < t->n; i++) h += t->it[i].sep ? 10 : 22;
    art_box(fb, px, py, w, h);
    int y = py + 14;
    for (int i = 0; i < t->n; i++) {
        MItem *m = &t->it[i];
        if (m->sep) { fill(fb, px + 14, y + 4, w - 28, 2, HK_TH_BORDER_IN); y += 10; continue; }
        int sel = i == g_menu.item, st = sel ? GF_SEL : m->grayed ? GF_DIM : GF_NORMAL;
        if (sel) red_dot(fb, px + 14, y + 8);
        gtext(fb, px + 26, y, m->text, st);
        if (m->key[0]) gtext(fb, px + w - 18 - gtext_w(m->key), y, m->key, st);
        y += 22;
    }
    hint_pill(fb, HINT_DROP, 446);
}

static int game_driven(const Dlg *d) { return d->res == 1500; }   // GAME DATA: see dlg_input
static void layer_draw(uint32_t *fb) {
    if (menu_shown()) menu_draw(fb);
    for (int k = 1; k < MAX_DLG; k++) {
        Dlg *d = &g_dlg[k];
        if (!d->used || !d->visible) continue;
        // The panel: the title screen's menu box, drawn 5 pixels outside the template's area (its border is about
        // 4 pixels thick and the controls start 3 pixels in), title in the game's font.
        art_box(fb, d->x - 5, d->y - 5, d->w + 10, d->h + 10);
        gtext(fb, d->x + (d->w - gtext_w(d->title)) / 2, d->y + 5, d->title, GF_SEL);
        fill(fb, d->x + 12, d->y + TITLE_H + 1, d->w - 24, 2, HK_TH_BORDER_IN);
        for (int i = 0; i < d->n; i++) if (d->ctl[i].visible) draw_ctl(fb, d, &d->ctl[i], i == d->focus && d == top_dlg());
        if (d->drop && d->focus >= 0) draw_drop(fb, d, &d->ctl[d->focus]);
        if (d == top_dlg()) {
            const char *hint = d->drop ? HINT_LIST : game_driven(d) ? HINT_GAME : HINT_DLG;
            if (d->focus >= 0 && editing(&d->ctl[d->focus])) hint = g_ime_ctl ? hk_tr(S_D_KBD) : HINT_EDIT;
            int hy = d->y + d->h + 10 < 446 ? d->y + d->h + 10 : 446;
            for (int j = 1; j < MAX_DLG; j++)   // over another dialog: at the bottom of the screen instead
                if (&g_dlg[j] != d && g_dlg[j].used && g_dlg[j].visible && hy + 30 > g_dlg[j].y - 5 && hy < g_dlg[j].y + g_dlg[j].h + 5) hy = 446;
            hint_pill(fb, hint, hy);
        }
    }
}

// FNV-style hash of everything the drawing reads (dialogs, controls, list items, menu bar, input mode).
static uint64_t hash_mem(uint64_t h, const void *p, size_t n) {
    const uint8_t *b = p;
    for (; n >= 8; n -= 8, b += 8) { uint64_t v; memcpy(&v, b, 8); h = (h ^ v) * 0x100000001b3ull; h ^= h >> 29; }
    for (; n; n--, b++) h = (h ^ *b) * 0x100000001b3ull;
    return h;
}
static uint64_t state_hash(void) {
    uint64_t h = 0xcbf29ce484222325ull;
    h = hash_mem(h, &g_menu, offsetof(typeof(g_menu), t));
    if (menu_shown()) h = hash_mem(h, g_menu.t, (size_t)g_menu.ntop * sizeof(MTop));
    for (int k = 1; k < MAX_DLG; k++) {
        const Dlg *d = &g_dlg[k];
        if (!d->used || !d->visible) continue;
        h = hash_mem(h, &k, sizeof k);
        h = hash_mem(h, d, offsetof(Dlg, ctl));
        h = hash_mem(h, &d->n, sizeof d->n + sizeof d->focus + sizeof d->big + sizeof d->drop);
        for (int i = 0; i < d->n; i++) {
            const Ctl *t = &d->ctl[i];
            h = hash_mem(h, t, offsetof(Ctl, items));
            if (t->items && t->nitems > 0) h = hash_mem(h, t->items, (size_t)t->nitems * sizeof t->items[0]);
        }
    }
    h = hash_mem(h, &g_ime_ctl, sizeof g_ime_ctl);
    return h;
}

// Composites the cached dialog layer over a game frame: fb points at the first of the 640 columns, stride in pixels.
// HK_DLG_TIME=1 logs the average cost of a composite and of a rebuild.
void hk_dlg_overlay(uint32_t *fb, int stride) {
    static uint64_t last; static int valid, y0, y1;
    static int timing = -1; static double t_comp, t_build; static unsigned n_comp, n_build;
    if (timing < 0) timing = getenv("HK_DLG_TIME") != NULL;
    struct timespec ts0, ts1; if (timing) clock_gettime(CLOCK_MONOTONIC, &ts0);
    uint64_t h = state_hash();
    int built = 0;
    if (!valid || h != last) {
        for (size_t i = 0; i < 480 * 640; i++) g_layer[i] = LAYER_CLEAR;
        layer_draw(g_layer);
        y0 = 480; y1 = 0;   // rows that hold anything
        for (int y = 0; y < 480; y++) {
            const uint32_t *r = g_layer + y * 640;
            int any = 0;
            for (int x = 0; x < 640 && !any; x++) any = r[x] != LAYER_CLEAR;
            if (any) { if (y < y0) y0 = y; y1 = y + 1; }
        }
        last = h; valid = 1; built = 1;
    }
    for (int y = y0; y < y1; y++) {
        const uint32_t *l = g_layer + y * 640;
        uint32_t *o = fb + (size_t)y * stride;
        for (int x = 0; x < 640; x++) {
            uint32_t v = l[x], top = v >> 24;
            if (top == 0xFFu) continue;
            if (top == 0) { o[x] = v; continue; }
            uint32_t p = o[x];
            o[x] = ((((p & 0xFF00FFu) * top) >> 8) & 0xFF00FFu) + (((((p >> 8) & 0xFFu) * top) >> 8) << 8) + (v & 0xFFFFFFu);
        }
    }
    if (timing) {
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        double ms = (ts1.tv_sec - ts0.tv_sec) * 1e3 + (ts1.tv_nsec - ts0.tv_nsec) / 1e6;
        if (built) { t_build += ms; n_build++; } else { t_comp += ms; n_comp++; }
        if (n_comp + n_build >= 100) {
            rt_log("[dlgtime] %u composites %.3f ms avg, %u rebuilds %.3f ms avg\n", n_comp, n_comp ? t_comp / n_comp : 0, n_build, n_build ? t_build / n_build : 0);
            n_comp = n_build = 0; t_comp = t_build = 0;
        }
    }
}

// Presents the last game frame with the dialogs on top (the game stops flipping while a modal dialog is up).
// Called from PeekMessageA/GetMessageA, so it must leave the game time to run: the interval is counted from the
// end of the last present of any kind, and nothing is presented while the game flips by itself. The GAME DATA loop
// (1500, 0x4328CD) keeps running game frames through 0x40E9F9, whose message loop (0x40EA18-0x40EA6D) only runs a
// frame once PeekMessageA finds the queue empty. When the refresh was timed from its start and a present took
// longer than the 40 ms window timer (Vita at 848x480: ~45 ms), every PeekMessageA presented again and found a
// new WM_TIMER, so the queue was never empty: no frame, no key poll (0x4117D1), the slots did not react to the pad.
// Reproduced on PC with HK_PRESENT_MS=50 HK_WIDE=848 and sound on (the timer exists only with DirectSound).
static void refresh(void) {
    extern uint32_t g_dd_present_end_ms;   // port/hk/ddraw.c
    // Only when the game itself has not presented for 60 ms (a modal loop or a paused game): while the game runs
    // (25 fps, a present every 40 ms) its own presents already show the overlay, and extra presents halved the
    // frame rate on the Vita (14 fps with the menu bar open).
    if (SDL_GetTicks() - g_dd_present_end_ms < 60) return;
    void hk_dd_present(void);
    hk_dd_present();
}
void hk_dlg_refresh(void) { if (hk_dlg_active()) refresh(); }

// ---------- creation ----------
static uint32_t dlg_send(CPU *c, Dlg *d, uint32_t msg, uint32_t wp, uint32_t lp) {
    return d->proc ? rt_guest_call(c, d->proc, 4, d->hwnd, msg, wp, lp) : 0;
}
// A message posted to a dialog (PostMessageA) and taken by the game's loop: DispatchMessageA hands it to the procedure.
// Dropped when the dialog is gone, as Windows drops messages for a destroyed window.
uint32_t hk_dlg_dispatch(CPU *c, uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp) {
    Dlg *d = dlg_of(hwnd);
    return d ? dlg_send(c, d, msg, wp, lp) : 0;
}
static void set_focus_default(Dlg *d) {
    d->focus = -1;
    for (int i = 0; i < d->n; i++) if (focusable(&d->ctl[i]) && d->ctl[i].cls == C_BUTTON && btype(&d->ctl[i]) == 1) { d->focus = i; return; }
    for (int i = 0; i < d->n; i++) if (focusable(&d->ctl[i])) { d->focus = i; return; }
}
static Dlg *dlg_create(CPU *c, uint32_t res, uint32_t proc, uint32_t param, int modal) {
    HOSTFN;
    uint32_t size, ga = w32_resource(res, 5, &size);   // RT_DIALOG
    if (!ga) { rt_log("[dlg] dialog %u not found\n", res); return NULL; }
    int k = 1;
    while (k < MAX_DLG && g_dlg[k].used) k++;
    if (k == MAX_DLG) { rt_log("[dlg] too many dialogs\n"); return NULL; }
    Dlg *d = &g_dlg[k];
    for (int i = 0; i < MAX_CTL; i++) { free(d->ctl[i].items); free(d->ctl[i].itemdata); }
    memset(d, 0, sizeof *d);
    if (!parse_template(d, ga)) return NULL;
    d->used = 1; d->modal = modal; d->res = (int)res; d->hwnd = DLG_HWND(k); d->proc = proc;
    for (int i = 0; i < d->n; i++)
        if (d->ctl[i].cls == C_COMBO || d->ctl[i].cls == C_LISTBOX) {
            d->ctl[i].items = calloc(MAX_ITEMS, sizeof *d->ctl[i].items);
            d->ctl[i].itemdata = calloc(MAX_ITEMS, sizeof *d->ctl[i].itemdata);
        }
    set_focus_default(d);
    rt_log("[dlg] create %u \"%s\" (%s) hwnd %08x proc %08x\n", res, d->title, modal ? "modal" : "modeless", d->hwnd, proc);
    d->visible = modal;   // a modeless dialog without WS_VISIBLE waits for ShowWindow
    uint32_t r = dlg_send(c, d, 0x110u, d->n ? ctl_hwnd(d, &d->ctl[d->focus >= 0 ? d->focus : 0]) : 0, param);   // WM_INITDIALOG
    (void)r;
    if (d->focus < 0 || !focusable(&d->ctl[d->focus])) set_focus_default(d);
    return d;
}
static void dlg_free(Dlg *d) {
    plat_text_enable(0);
    for (int i = 0; i < MAX_CTL; i++) { free(d->ctl[i].items); free(d->ctl[i].itemdata); d->ctl[i].items = NULL; d->ctl[i].itemdata = NULL; }
    d->used = 0; d->visible = 0;
    void hk_dd_present(void);
    hk_dd_present();   // remove it from the screen
}

// ---------- input ----------
static void command(CPU *c, Dlg *d, Ctl *t, uint32_t code) {
    dlg_send(c, d, 0x111u, (uint32_t)t->id | (code << 16), ctl_hwnd(d, t));   // WM_COMMAND
}
static void click(CPU *c, Dlg *d, Ctl *t) {
    if (!t->enabled) return;
    if (t->cls == C_BUTTON) {
        int b = btype(t);
        if (b == 3) t->checked = !t->checked;           // BS_AUTOCHECKBOX
        else if (b == 9) {                              // BS_AUTORADIOBUTTON: uncheck the rest of its group
            int i = (int)(t - d->ctl), s = i, e = i;
            while (s > 0 && !(d->ctl[s].style & 0x20000u) && d->ctl[s - 1].cls == C_BUTTON && btype(&d->ctl[s - 1]) == 9) s--;
            while (e + 1 < d->n && !(d->ctl[e + 1].style & 0x20000u) && d->ctl[e + 1].cls == C_BUTTON && btype(&d->ctl[e + 1]) == 9) e++;
            for (int j = s; j <= e; j++) d->ctl[j].checked = j == i;
        }
        command(c, d, t, 0);   // BN_CLICKED
    } else if (t->cls == C_COMBO || t->cls == C_LISTBOX) {
        if (t->nitems) { t->cursel = (t->cursel + 1) % t->nitems; command(c, d, t, 1); }   // CBN_SELCHANGE / LBN_SELCHANGE
    }
}
static void change(CPU *c, Dlg *d, Ctl *t, int dir) {   // left/right on a combo or list box
    if (!t->nitems) return;
    int n = t->cursel < 0 ? (dir > 0 ? 0 : t->nitems - 1) : (t->cursel + dir + t->nitems) % t->nitems;
    t->cursel = n;
    if (t->cls == C_LISTBOX) { int rows = (t->h - 4) / t->rh; if (rows < 1) rows = 1; if (n < t->top) t->top = n; if (n >= t->top + rows) t->top = n - rows + 1; }
    command(c, d, t, 1);
}
// Spatial focus move for a pad: the next control must lie past the focused one's edge in that direction; controls
// that overlap it across the direction (same row or column) win, then the nearest.
static void move_focus(Dlg *d, int dx, int dy) {
    if (d->focus < 0) { set_focus_default(d); return; }
    Ctl *f = &d->ctl[d->focus];
    int best = -1, bestv = 1 << 30;
    for (int i = 0; i < d->n; i++) {
        Ctl *t = &d->ctl[i];
        if (i == d->focus || !focusable(t)) continue;
        int tx = t->x + t->w / 2, ty = t->y + t->h / 2, along, gap, overlap;
        if (dx) {
            along = dx < 0 ? f->x - tx : tx - (f->x + f->w);
            if (along < -f->w / 2 || (dx < 0 ? tx >= f->x + f->w / 4 : tx <= f->x + f->w * 3 / 4)) continue;
            overlap = t->y < f->y + f->h && f->y < t->y + t->h;
            gap = overlap ? 0 : abs(ty - (f->y + f->h / 2));
        } else {
            along = dy < 0 ? f->y - ty : ty - (f->y + f->h);
            if (along < 0) continue;
            overlap = t->x < f->x + f->w && f->x < t->x + t->w;
            gap = overlap ? abs(t->x - f->x) / 4 : abs(tx - (f->x + f->w / 2));
        }
        int v = (overlap ? 0 : 1 << 20) + (along > 0 ? along : 0) + 2 * gap;
        if (v < bestv) { bestv = v; best = i; }
    }
    if (best >= 0) { d->focus = best; rt_log("[dlg] focus id %u \"%s\"\n", d->ctl[best].id, d->ctl[best].text); }
}
// K_OK presses the focused control (space, X, pad Cross); K_DEFAULT is Enter (pad Triangle): the dialog's default
// button (OK), wherever the focus is.
enum { K_UP = 1, K_DOWN = 2, K_LEFT = 4, K_RIGHT = 8, K_OK = 16, K_CANCEL = 32, K_DEFAULT = 64, K_RENAME = 128, K_DELETE = 256 };
// Dialogs the game drives itself: GAME DATA (1500, save/load slots) runs its own loop at 0x4328CD that polls the game's
// input object and moves the slot radio buttons (0x401726) or posts WM_COMMAND 0x9CA4 (confirm) / 0x9CA5 (cancel).
// Port-side navigation on top of it moved the focus to Cancel while the game moved the slot, so Enter cancelled.
// Its Rename (40072, 0x432428) and Delete (40024) buttons are mouse-only on Windows: R / D (pad Square / L) click them.
#define ID_RENAME 40072
#define ID_DELETE 40024
static void dlg_input(CPU *c, int keys) {
    Dlg *d = top_dlg();
    if (!d) return;
    if (game_driven(d)) {
        Ctl *b = (keys & K_RENAME) ? ctl_by_id(d, ID_RENAME) : (keys & K_DELETE) ? ctl_by_id(d, ID_DELETE) : NULL;
        if (b && b->visible && b->enabled) { rt_log("[dlg] %u: button %u \"%s\"\n", d->res, b->id, b->text); click(c, d, b); }
        return;
    }
    Ctl *f = d->focus >= 0 ? &d->ctl[d->focus] : NULL;
    if (f && d->drop) {   // an open combo box list: arrows choose, OK keeps the choice, cancel restores the old one
        if (keys & K_CANCEL) { f->cursel = d->drop_orig; d->drop = 0; return; }
        if (keys & (K_OK | K_DEFAULT)) { d->drop = 0; if (f->cursel != d->drop_orig) command(c, d, f, 1); return; }   // CBN_SELCHANGE
        int step = (keys & (K_UP | K_LEFT)) ? -1 : (keys & (K_DOWN | K_RIGHT)) ? 1 : 0;
        if (step && f->nitems) {
            int n = f->cursel + step;
            f->cursel = n < 0 ? 0 : n >= f->nitems ? f->nitems - 1 : n;
        }
        return;
    }
    if (keys & K_CANCEL) {
        Ctl *cancel = ctl_by_id(d, 2);
        dlg_send(c, d, 0x111u, 2u, cancel ? ctl_hwnd(d, cancel) : 0);   // IDCANCEL
        return;
    }
    if (keys & K_DEFAULT) {   // the default push button (BS_DEFPUSHBUTTON), else OK (id 1), else the focused control
        Ctl *ok = NULL;
        for (int i = 0; i < d->n && !ok; i++)
            if (d->ctl[i].cls == C_BUTTON && btype(&d->ctl[i]) == 1 && d->ctl[i].visible && d->ctl[i].enabled) ok = &d->ctl[i];
        if (!ok) ok = ctl_by_id(d, 1);
        if (ok && ok->enabled && ok->visible) { click(c, d, ok); return; }
        keys |= K_OK;
    }
    if (keys & K_OK) {
        if (f && editing(f) && !plat_has_keyboard()) {
            const char *title = d->title;   // the group box around the control names it ("SAVE DATA" in 1499)
            for (int i = 0; i < d->n; i++) {
                Ctl *g = &d->ctl[i];
                if (g->cls == C_BUTTON && btype(g) == 7 && g->text[0] && f->x >= g->x && f->y >= g->y &&
                    f->x + f->w <= g->x + g->w && f->y + f->h <= g->y + g->h) title = g->text;
            }
            if (plat_text_open(title, f->text, edit_limit(f))) g_ime_ctl = f;
            return;
        }
        if (f && f->cls == C_COMBO) { if (f->enabled && f->nitems) { d->drop = 1; d->drop_orig = f->cursel; } return; }
        if (f) click(c, d, f);
        return;
    }
    int lr = (keys & K_LEFT) ? -1 : (keys & K_RIGHT) ? 1 : 0;
    if (lr && f && f->cls == C_LISTBOX) { change(c, d, f, lr); return; }
    if (f && f->cls == C_LISTBOX && (keys & (K_UP | K_DOWN))) {
        int nf = d->focus;
        move_focus(d, 0, (keys & K_UP) ? -1 : 1);
        if (nf == d->focus) change(c, d, f, (keys & K_UP) ? -1 : 1);
        return;
    }
    if (keys & K_UP) move_focus(d, 0, -1);
    else if (keys & K_DOWN) move_focus(d, 0, 1);
    else if (lr) move_focus(d, lr, 0);
}
#define HWND_MAIN_DLG 0x00010010u   // the game's window (port/win32/user32.c HWND_MAIN)
static void menu_input(int keys) {
    if (keys & K_DEFAULT) keys |= K_OK;
    MTop *t = g_menu.top < g_menu.ntop ? &g_menu.t[g_menu.top] : NULL;
    if (!t) return;
    int lr = (keys & K_LEFT) ? -1 : (keys & K_RIGHT) ? 1 : 0;
    if (keys & K_CANCEL) {
        if (g_menu.open) g_menu.open = 0;
        else w32_post_message(HWND_MAIN_DLG, 0x111u, 0u, 0);   // WM_COMMAND with no command: the game hides the menu bar on any WM_COMMAND (a second right click did not)
        return;
    }
    if (lr) {
        int i = g_menu.top;
        for (int k = 0; k < g_menu.ntop; k++) { i = (i + lr + g_menu.ntop) % g_menu.ntop; if (mtop_ok(i)) break; }
        g_menu.top = i;
        g_menu.item = 0;
        while (g_menu.open && g_menu.item < g_menu.t[i].n && !mitem_ok(&g_menu.t[i], g_menu.item)) g_menu.item++;
        return;
    }
    if (!g_menu.open) {
        if (keys & (K_OK | K_DOWN)) {
            if (!t->popup) { if (keys & K_OK) { rt_log("[menu] command %u\n", t->id); w32_post_message(HWND_MAIN_DLG, 0x111u, t->id, 0); } return; }
            g_menu.open = 1; g_menu.item = 0;
            while (g_menu.item < t->n && !mitem_ok(t, g_menu.item)) g_menu.item++;
        }
        return;
    }
    if (keys & (K_UP | K_DOWN)) {
        int dir = (keys & K_UP) ? -1 : 1, i = g_menu.item;
        for (int k = 0; k < t->n; k++) { i = (i + dir + t->n) % t->n; if (mitem_ok(t, i)) break; }
        g_menu.item = i;
        return;
    }
    if ((keys & K_OK) && mitem_ok(t, g_menu.item)) {
        uint16_t id = t->it[g_menu.item].id;
        g_menu.open = 0;
        rt_log("[menu] command %u (%s)\n", id, t->it[g_menu.item].text);
        w32_post_message(HWND_MAIN_DLG, 0x111u, id, 0);   // WM_COMMAND from a menu: the game hides the menu itself
    }
}

// Key transitions while a dialog is up: called by USER32's message functions instead of posting WM_KEYDOWN.
// Several keys pressed together (pad Cross sends X and space) count as one action.
void hk_dlg_keys(CPU *c, const uint8_t *pressed_vks, int n) {
    Dlg *d = top_dlg();
    if (g_ime_ctl) {   // the system keyboard is up: wait for it, the pad belongs to it
        char buf[160];
        int st = plat_text_status(buf, sizeof buf);
        if (st) {
            Ctl *t = g_ime_ctl;
            g_ime_ctl = NULL;
            if (st == 1 && d && t >= d->ctl && t < d->ctl + d->n) {
                set_edit_text(t, buf);
                command(c, d, t, 0x300u);   // EN_CHANGE
            }
            rt_log("[dlg] system keyboard closed (%s)\n", st == 1 ? "text accepted" : "cancelled");
        }
        refresh();
        return;
    }
    Ctl *f = d && d->focus >= 0 ? &d->ctl[d->focus] : NULL;
    int typing = f && editing(f) && plat_has_keyboard();
    plat_text_enable(typing);
    if (typing) {   // PC: typed characters go into the focused EDIT; letters and space no longer act as dialog keys
        char buf[64];
        int m = plat_text_typed(buf, sizeof buf), changed = 0;
        for (int i = 0; i < m; i++) {
            size_t len = strlen(f->text);
            if (buf[i] == '\b') { if (len) { f->text[len - 1] = 0; changed = 1; } }
            else if ((int)len < edit_limit(f)) { f->text[len] = buf[i]; f->text[len + 1] = 0; changed = 1; }
        }
        if (changed) command(c, d, f, 0x300u);   // EN_CHANGE
    }
    int keys = 0;
    for (int i = 0; i < n; i++) {
        if (typing && (pressed_vks[i] == 0x20 || (pressed_vks[i] >= 'A' && pressed_vks[i] <= 'Z'))) continue;
        switch (pressed_vks[i]) {
        case 0x26: keys |= K_UP; break;
        case 0x28: keys |= K_DOWN; break;
        case 0x25: keys |= K_LEFT; break;
        case 0x27: keys |= K_RIGHT; break;
        case 0x20: case 'X': keys |= K_OK; break;
        case 0x0D: case 'C': keys |= K_DEFAULT; break;
        case 0x1B: case 'Z': keys |= K_CANCEL; break;
        case 'R': keys |= K_RENAME; break;
        case 'D': keys |= K_DELETE; break;
        default: break;
        }
    }
    if (keys && top_dlg()) dlg_input(c, keys);
    else if (keys && menu_shown()) menu_input(keys);
    refresh();
}
// F10 (pad: Select on the pause screen) asks the game for its menu with a right click; it also closes it.
void hk_menu_key(void) {
    if (top_dlg()) return;
    w32_post_message(HWND_MAIN_DLG, 0x204u, 0x2u, 0);   // WM_RBUTTONDOWN, MK_RBUTTON
}

// ---------- USER32 ----------
#define U(name) void imp_USER32_##name(CPU *c)
void w32_modal_step(CPU *c);   // port/win32/user32.c: one pass of the modal message loop

// CreateDialogParamA(hInstance, template, parent, proc, param)
U(CreateDialogParamA) {
    HOSTFN;
    Dlg *d = dlg_create(c, ARG(2), ARG(4), ARG(5), 0);
    if (d) {
        uint32_t size, ga = w32_resource(ARG(2), 5, &size);
        uint32_t style; memcpy(&style, mb + ga, 4);
        if (style == 0xffff0001u) memcpy(&style, mb + ga + 12, 4);   // DLGTEMPLATEEX
        if (style & 0x10000000u) d->visible = 1;
        refresh();
    }
    STDRET(5, d ? d->hwnd : 0);
}
// ---------- settings file ----------
// The game keeps its settings in a CConfig object built with defaults at every start (constructor 0x414CF6, called
// from 0x4513B2; layout from the dialog procedures 0x414F6C and 0x415242 and CConfig::Serialize 0x414D79):
// +0x04 View Sky, +0x08 View Background, +0x0C Full Effect, +0x10 Full Motion, +0x14 screen size (0 normal,
// 1 middle, 2 small), +0x18 Opening Animation, +0x1C Music On, +0x20 Sound Effects On, +0x24..+0x93 the key table
// (copied from 0x47B548 by Reset), +0x94 Move Down to Crouch, +0x98 Move Up to Jump. HDKWIN.EXE reads and writes no
// file or registry value for it (runs with both dialogs and Exit showed no CreateFileA/RegSetValueExA), so the
// settings were lost at exit. The port saves +0x04..+0x9B to hkconfig.txt in the game folder when Game or Key
// Configuration closes with OK, and loads it into each new CConfig (hook at 0x414D0B, after the constructor's
// defaults; recomp/image_patches.py).
#define CFG_FIRST 0x04u
#define CFG_END 0x9Cu
#define CFG_OBJ 0x4AC844u   // the CConfig the open configuration dialog edits (set by 0x4151CA / 0x415433)
static void cfg_path(char *out, size_t cap) { snprintf(out, cap, "%s/hkconfig.txt", g_game_dir); }
static void cfg_save(uint32_t obj) {
    HOSTFN;
    if (!obj) return;
    char path[600], tmp[640];
    cfg_path(path, sizeof path);
    if (sw_tmp_path(path, tmp, sizeof tmp)) return;
    FILE *f = fopen(tmp, "w");
    if (!f) { rt_log("[config] cannot write %s\n", tmp); return; }
    fprintf(f, "# Handkerchief port: the game's settings (CConfig fields, hex offset=value), written after OK\n");
    for (uint32_t o = CFG_FIRST; o < CFG_END; o += 4) fprintf(f, "%02x=%u\n", o, R32(obj + o));
    int ok = fclose(f) == 0;
    if (!ok || sw_commit(path)) { rt_log("[config] cannot save %s\n", path); return; }
    rt_log("[config] saved %s (music %u, sound %u, screen %u)\n", path, R32(obj + 0x1C), R32(obj + 0x20), R32(obj + 0x14));
}
// Hook at 0x414D0B (esi = the new CConfig). Unknown offsets and implausible values are ignored, field by field.
void hk_config_loaded(CPU *c) {
    HOSTFN;
    uint32_t obj = c->esi;
    char path[600], line[64];
    cfg_path(path, sizeof path);
    sw_recover(path);
    FILE *f = fopen(path, "r");
    if (!f) return;
    int n = 0;
    while (fgets(line, sizeof line, f)) {
        unsigned o, v;
        if (line[0] == '#' || sscanf(line, "%x=%u", &o, &v) != 2) continue;
        if (o < CFG_FIRST || o >= CFG_END || (o & 3u) || v > 0xFFFFu) continue;
        if (o == 0x14 && v > 2) continue;
        W32(obj + o, v);
        n++;
    }
    fclose(f);
    rt_log("[config] loaded %d values from %s (music %u, sound %u)\n", n, path, R32(obj + 0x1C), R32(obj + 0x20));
}

// DialogBoxParamA(hInstance, template, parent, proc, param): runs its own message loop until EndDialog.
U(DialogBoxParamA) {
    HOSTFN;
    Dlg *d = dlg_create(c, ARG(2), ARG(4), ARG(5), 1);
    if (!d) STDRET(5, 0xffffffffu);
    while (d->used && !d->ended) {
        w32_modal_step(c);
        if (!d->ended) { refresh(); SDL_Delay(10); }
    }
    uint32_t r = d->result;
    int res = d->res;
    if (d->used) { dlg_send(c, d, 0x2u, 0, 0); dlg_free(d); }   // WM_DESTROY
    if (r == 1 && (res == 1491 || res == 1492)) cfg_save(R32(CFG_OBJ));   // Game / Key Configuration: OK
    rt_log("[dlg] dialog %u closed -> %d\n", ARG(2), (int)r);
    STDRET(5, r);
}
U(EndDialog) {
    HOSTFN;
    Dlg *d = dlg_of(ARG(1));
    if (d) {
        d->result = ARG(2);
        if (d->modal) d->ended = 1;
        else d->visible = 0;   // a modeless dialog stays until DestroyWindow
    }
    STDRET(2, d ? 1 : 0);
}
int hk_dlg_destroy(CPU *c, uint32_t hwnd) {
    Dlg *d = dlg_of(hwnd);
    if (!d) return 0;
    if (d->modal) { d->ended = 1; return 1; }
    d->visible = 0;
    dlg_send(c, d, 0x2u, 0, 0);   // WM_DESTROY
    dlg_free(d);
    return 1;
}
int hk_dlg_show(uint32_t hwnd, int cmd) {
    Dlg *d = dlg_of(hwnd);
    Ctl *t = d ? NULL : ctl_of_hwnd(hwnd);
    if (!d && !t) return -1;
    int was = d ? d->visible : t->visible;
    if (d) d->visible = cmd != 0; else t->visible = cmd != 0;
    refresh();
    return was;
}
int hk_dlg_enable(uint32_t hwnd, int on) {
    Ctl *t = ctl_of_hwnd(hwnd);
    if (!t) return -1;
    int was = !t->enabled;
    t->enabled = on != 0;
    return was;
}
int hk_dlg_enabled(uint32_t hwnd) { Ctl *t = ctl_of_hwnd(hwnd); return t ? t->enabled : -1; }
int hk_dlg_set_focus(uint32_t hwnd) {
    for (int k = 1; k < MAX_DLG; k++) {
        Dlg *d = &g_dlg[k];
        if (!d->used) continue;
        for (int i = 0; i < d->n; i++) if (ctl_hwnd(d, &d->ctl[i]) == hwnd) { d->focus = i; return 1; }
    }
    return 0;
}

U(GetDlgItem) { HOSTFN; Dlg *d = dlg_of(ARG(1)); Ctl *t = ctl_by_id(d, ARG(2)); STDRET(2, t ? ctl_hwnd(d, t) : 0); }
U(SetDlgItemTextA) {
    HOSTFN;
    Ctl *t = ctl_by_id(dlg_of(ARG(1)), ARG(2));
    if (t) { snprintf(t->text, sizeof t->text, "%s", ARG(3) ? GSTR(ARG(3)) : ""); if (t->cls != C_EDIT) strip_amp(t->text); }
    STDRET(3, t ? 1 : 0);
}
U(GetDlgItemTextA) {
    HOSTFN;
    Ctl *t = ctl_by_id(dlg_of(ARG(1)), ARG(2));
    uint32_t buf = ARG(3), cap = ARG(4), n = 0;
    if (buf && cap) {
        const char *s = t ? (t->cls == C_COMBO && t->cursel >= 0 ? t->items[t->cursel] : t->text) : "";
        n = (uint32_t)strlen(s);
        if (n >= cap) n = cap - 1;
        memcpy(mb + buf, s, n);
        W8(buf + n, 0);
    }
    STDRET(4, n);
}
U(SetDlgItemInt) {
    HOSTFN;
    Ctl *t = ctl_by_id(dlg_of(ARG(1)), ARG(2));
    if (t) { if (ARG(4)) snprintf(t->text, sizeof t->text, "%d", (int32_t)ARG(3)); else snprintf(t->text, sizeof t->text, "%u", ARG(3)); }
    STDRET(4, t ? 1 : 0);
}
U(GetDlgItemInt) {   // (dlg, id, BOOL *translated, signed)
    HOSTFN;
    Ctl *t = ctl_by_id(dlg_of(ARG(1)), ARG(2));
    char *e = NULL;
    long v = t ? strtol(t->text, &e, 10) : 0;
    int ok = t && e && e != t->text;
    if (ARG(3)) W32(ARG(3), ok ? 1u : 0u);
    STDRET(4, ok ? (uint32_t)v : 0u);
}
U(CheckDlgButton) { HOSTFN; Ctl *t = ctl_by_id(dlg_of(ARG(1)), ARG(2)); if (t) t->checked = (int)ARG(3); STDRET(3, t ? 1 : 0); }
U(IsDlgButtonChecked) { HOSTFN; Ctl *t = ctl_by_id(dlg_of(ARG(1)), ARG(2)); STDRET(2, t ? (uint32_t)t->checked : 0u); }
U(CheckRadioButton) {   // (dlg, first, last, check)
    HOSTFN;
    Dlg *d = dlg_of(ARG(1));
    for (int i = 0; d && i < d->n; i++)
        if (d->ctl[i].id >= ARG(2) && d->ctl[i].id <= ARG(3)) d->ctl[i].checked = d->ctl[i].id == ARG(4);
    STDRET(4, d ? 1 : 0);
}

static int add_item(Ctl *t, const char *s, int at) {
    if (!t->items || t->nitems >= MAX_ITEMS) return -1;
    if (at < 0 || at > t->nitems) at = t->nitems;
    memmove(&t->items[at + 1], &t->items[at], (size_t)(t->nitems - at) * sizeof t->items[0]);
    memmove(&t->itemdata[at + 1], &t->itemdata[at], (size_t)(t->nitems - at) * sizeof t->itemdata[0]);
    snprintf(t->items[at], sizeof t->items[0], "%s", s);
    t->itemdata[at] = 0;
    t->nitems++;
    return at;
}
static int find_item(Ctl *t, const char *s, int start, int exact) {
    for (int k = 0; k < t->nitems; k++) {
        int i = (start + 1 + k) % (t->nitems ? t->nitems : 1);
        if (exact ? !strcasecmp(t->items[i], s) : !strncasecmp(t->items[i], s, strlen(s))) return i;
    }
    return -1;
}
// Messages to a control: buttons, combo boxes, list boxes and text. Unknown ones are logged once each.
static uint32_t ctl_message(CPU *c, Dlg *d, Ctl *t, uint32_t msg, uint32_t wp, uint32_t lp) {
    HOSTFN;
    int cb = t->cls == C_COMBO;
    switch (msg) {
    case 0x0C: snprintf(t->text, sizeof t->text, "%s", lp ? GSTR(lp) : ""); return 1;   // WM_SETTEXT
    case 0x0D: {                                                                      // WM_GETTEXT
        uint32_t n = (uint32_t)strlen(t->text);
        if (!wp) return 0;
        if (n >= wp) n = wp - 1;
        memcpy(mb + lp, t->text, n); W8(lp + n, 0);
        return n;
    }
    case 0x0E: return (uint32_t)strlen(t->text);                                     // WM_GETTEXTLENGTH
    case 0xF0: return (uint32_t)t->checked;                                          // BM_GETCHECK
    case 0xF1: t->checked = (int)wp; return 0;                                       // BM_SETCHECK
    case 0xC5: t->limit = (int)wp; return 0;                                         // EM_LIMITTEXT
    case 0xF5: click(c, d, t); return 0;                                             // BM_CLICK
    }
    if (cb || t->cls == C_LISTBOX) {
        uint32_t add = cb ? 0x143 : 0x180, ins = cb ? 0x14A : 0x181, reset = cb ? 0x14B : 0x184, setsel = cb ? 0x14E : 0x186,
                 getsel = cb ? 0x147 : 0x188, count = cb ? 0x146 : 0x18B, gettext = cb ? 0x148 : 0x189,
                 textlen = cb ? 0x149 : 0x18A, getdata = cb ? 0x150 : 0x199, setdata = cb ? 0x151 : 0x19A,
                 find = cb ? 0x14C : 0x18F, findx = cb ? 0x158 : 0x1A2, del = cb ? 0x144 : 0x182,
                 selstr = cb ? 0x14D : 0x18C;
        if (msg == add) return (uint32_t)add_item(t, lp ? GSTR(lp) : "", -1);
        if (msg == ins) return (uint32_t)add_item(t, lp ? GSTR(lp) : "", (int)wp);
        if (msg == reset) { t->nitems = 0; t->cursel = -1; t->top = 0; return 0; }
        if (msg == setsel) { t->cursel = (int)wp < t->nitems ? (int)wp : -1; if (t->cursel >= 0 && t->cursel < t->top) t->top = t->cursel; return (uint32_t)t->cursel; }
        if (msg == getsel) return (uint32_t)t->cursel;
        if (msg == count) return (uint32_t)t->nitems;
        if (msg == gettext) { if ((int)wp >= t->nitems) return 0xffffffffu; strcpy((char *)mb + lp, t->items[wp]); return (uint32_t)strlen(t->items[wp]); }
        if (msg == textlen) return (int)wp < t->nitems ? (uint32_t)strlen(t->items[wp]) : 0xffffffffu;
        if (msg == getdata) return (int)wp < t->nitems ? t->itemdata[wp] : 0xffffffffu;
        if (msg == setdata) { if ((int)wp < t->nitems) t->itemdata[wp] = lp; return 0; }
        if (msg == find) return (uint32_t)find_item(t, lp ? GSTR(lp) : "", (int32_t)wp, 0);
        if (msg == findx) return (uint32_t)find_item(t, lp ? GSTR(lp) : "", (int32_t)wp, 1);
        if (msg == selstr) {   // CB_SELECTSTRING / LB_SELECTSTRING: first item starting with the text after index wp
            int i = find_item(t, lp ? GSTR(lp) : "", (int32_t)wp, 0);
            if (i >= 0) t->cursel = i;
            return (uint32_t)i;
        }
        if (msg == del) {
            if ((int)wp >= t->nitems) return 0xffffffffu;
            memmove(&t->items[wp], &t->items[wp + 1], (size_t)(t->nitems - (int)wp - 1) * sizeof t->items[0]);
            memmove(&t->itemdata[wp], &t->itemdata[wp + 1], (size_t)(t->nitems - (int)wp - 1) * sizeof t->itemdata[0]);
            t->nitems--;
            if (t->cursel >= t->nitems) t->cursel = t->nitems - 1;
            return (uint32_t)t->nitems;
        }
        if (cb && (msg == 0x141 || msg == 0x14F || msg == 0x155 || msg == 0x15B)) return 0;   // limit, show dropdown, extended UI
    }
    static uint32_t said[32]; static int nsaid;
    int known = 0;
    for (int i = 0; i < nsaid; i++) known |= said[i] == msg;
    if (!known && nsaid < 32) { said[nsaid++] = msg; rt_log("[dlg] control message %04x to id %u (class %d) not handled\n", msg, t->id, t->cls); }
    return 0;
}
U(SendDlgItemMessageA) {   // (dlg, id, msg, wparam, lparam)
    HOSTFN;
    Dlg *d = dlg_of(ARG(1));
    Ctl *t = ctl_by_id(d, ARG(2));
    STDRET(5, t ? ctl_message(c, d, t, ARG(3), ARG(4), ARG(5)) : 0);
}

// Menus: LoadMenuA hands out a handle naming the resource; SetMenu shows it as the port's menu bar (see above).
U(LoadMenuA) { HOSTFN; rt_log("[menu] LoadMenuA(%u)\n", ARG(2)); STDRET(2, ARG(2) < 0x10000u ? MENU_HANDLE(ARG(2)) : 0u); }
U(SetMenu) { HOSTFN; menu_set(ARG(2)); refresh(); STDRET(2, 1); }
U(DestroyMenu) { HOSTFN; if (ARG(1) && (ARG(1) & 0xffffu) == g_menu.res) g_menu.res = 0; STDRET(1, 1); }

// ---------- "Load game" from the pause screen ----------
// The title menu's File > Continue (WM_COMMAND 40053) opens dialog 1500 in load mode, but only exists in the title
// menu (1505); the in-stage menu (1504) has Stop This Game (40061) instead. So inside a stage the request first
// posts 40061: the game asks "Stop this game ?" (dialog 1497, answered by the player with the usual dialog keys:
// OK stops, Cancel keeps playing, and then the pending load is dropped). Once the title is showing again (no stage
// drawn, no dialog or menu bar open, a short settle time), 40053 is posted once. The request is ignored while
// another one is pending, and gives up after 40 s.
// HK_LOAD_AT=<frame> (test aid, PC): requests it at that presented frame.
extern int g_hk_in_stage;   // port/hk/wide.c
enum { LG_IDLE, LG_STOPPING, LG_TITLE };
static int g_lg_state, g_lg_seen_dlg;
static unsigned g_lg_t0, g_lg_mark;
void hk_load_game_request(void) {
    extern unsigned g_frame;
    if (g_lg_state != LG_IDLE) return;
    g_lg_t0 = g_frame; g_lg_seen_dlg = 0;
    if (g_hk_in_stage) { w32_post_message(HWND_MAIN_DLG, 0x111u, 40061u, 0); g_lg_state = LG_STOPPING; rt_log("[loadgame] in a stage: Stop This Game first\n"); }
    else { w32_post_message(HWND_MAIN_DLG, 0x111u, 40053u, 0); rt_log("[loadgame] title: Continue\n"); }
}
void hk_load_game_tick(void) {   // once per presented frame
    extern unsigned g_frame;
    static long at = -2;
    if (at == -2) { const char *e = getenv("HK_LOAD_AT"); at = e ? atol(e) : -1; }
    if (at >= 0 && g_frame == (unsigned)at) hk_load_game_request();
    if (g_lg_state == LG_IDLE) return;
    if (g_frame - g_lg_t0 > 1000) { g_lg_state = LG_IDLE; rt_log("[loadgame] gave up\n"); return; }
    if (g_lg_state == LG_STOPPING) {
        if (hk_dlg_active()) { g_lg_seen_dlg = 1; g_lg_mark = g_frame; }
        else if (!g_hk_in_stage) { g_lg_state = LG_TITLE; g_lg_mark = g_frame; }
        else if (g_lg_seen_dlg && g_frame - g_lg_mark > 50) { g_lg_state = LG_IDLE; rt_log("[loadgame] Stop declined\n"); }
    } else {
        if (g_hk_in_stage || hk_dlg_active()) g_lg_mark = g_frame;
        else if (g_frame - g_lg_mark >= 30) { g_lg_state = LG_IDLE; w32_post_message(HWND_MAIN_DLG, 0x111u, 40053u, 0); rt_log("[loadgame] title ready: Continue\n"); }
    }
}
