// Win32 dialogs for the recompiled Handkerchief, drawn by the port over the game screen and driven by the keyboard
// or the pad. The game's own dialog procedures run unchanged: the port reads the DIALOG template from HDKWIN.EXE,
// sends WM_INITDIALOG / WM_COMMAND / WM_DESTROY, and answers the control functions the game imports
// (Get/SetDlgItemText/Int, CheckDlgButton, SendDlgItemMessageA with button, combo box and list box messages).
//
// Dialogs used by the game (tools/dump_dialogs.py): 1498 "Exit Handkerchief ?" (Esc, modeless), 1491 Game
// Configuration and 1492 Key Configuration (F2, modal), 1490 secret options, 1493/1497/1499 messages and input.
//
// Controls: arrows move the focus between controls (left/right change a combo box, list box or radio group);
// Enter, space or X (pad Cross) press the focused control; Esc or Z (pad Circle, Select) send IDCANCEL.
#include "win32.h"
#include "plat.h"
#include "hk/font_data.h"
#include <SDL.h>
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
} Dlg;

static Dlg g_dlg[MAX_DLG];
#define DLG_HWND(k) (0x00030000u | ((uint32_t)(k) << 8))
#define TITLE_H 18
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
int hk_dlg_active(void) { return top_dlg() != NULL; }

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

static int parse_template(Dlg *d, uint32_t ga) {
    HOSTFN;
    const uint8_t *p = mb + ga;
    uint32_t style; memcpy(&style, p, 4);
    if (style == 0xffff0001u) { rt_log("[dlg] extended DIALOGEX template not supported\n"); return 0; }
    uint16_t n; int16_t cx, cy;
    memcpy(&n, p + 8, 2); memcpy(&cx, p + 14, 2); memcpy(&cy, p + 16, 2);
    p += 18;
    p = res_sz(p, NULL, 0, NULL);                      // menu
    p = res_sz(p, NULL, 0, NULL);                      // class
    p = res_sz(p, d->title, sizeof d->title, NULL);    // title
    if (style & 0x40) { p += 2; p = res_sz(p, NULL, 0, NULL); }   // DS_SETFONT: point size, face
    d->w = DU_X(cx) + 6; d->h = DU_Y(cy) + TITLE_H + 6;
    if (d->w > 636) d->w = 636;
    if (d->h > 476) d->h = 476;
    d->x = (640 - d->w) / 2; d->y = (480 - d->h) / 2;
    d->n = 0;
    for (int i = 0; i < n && d->n < MAX_CTL; i++) {
        p = mb + ((uint32_t)(p - mb + 3) & ~3u);
        Ctl *t = &d->ctl[d->n];
        memset(t, 0, sizeof *t);
        int16_t x, y, w, h; uint16_t id;
        memcpy(&t->style, p, 4); memcpy(&x, p + 8, 2); memcpy(&y, p + 10, 2); memcpy(&w, p + 12, 2);
        memcpy(&h, p + 14, 2); memcpy(&id, p + 16, 2);
        p += 18;
        int ord; char cls[32];
        p = res_sz(p, cls, sizeof cls, &ord);
        p = res_sz(p, t->text, sizeof t->text, NULL);
        uint16_t extra; memcpy(&extra, p, 2); p += 2 + extra;
        t->cls = ord == 0x80 || !strcasecmp(cls, "button") ? C_BUTTON : ord == 0x81 || !strcasecmp(cls, "edit") ? C_EDIT
               : ord == 0x82 || !strcasecmp(cls, "static") ? C_STATIC : ord == 0x83 || !strcasecmp(cls, "listbox") ? C_LISTBOX
               : ord == 0x85 || !strcasecmp(cls, "combobox") ? C_COMBO : C_OTHER;
        t->id = id;
        t->x = DU_X(x); t->y = DU_Y(y); t->w = DU_X(w);
        t->h = t->cls == C_COMBO ? 20 : DU_Y(h);
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
static const HkFont *font(void) { return &hk_fonts[0]; }
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
#define C_FACE 0xc0c0c0u
#define C_LIGHT 0xffffffu
#define C_SHADOW 0x808080u
#define C_DARK 0x000000u
#define C_FOCUS 0xffd000u

static const char *item_text(const Ctl *t, int i) { return i >= 0 && i < t->nitems ? t->items[i] : ""; }

static void draw_ctl(uint32_t *fb, const Dlg *d, const Ctl *t, int focused) {
    int x = d->x + 3 + t->x, y = d->y + TITLE_H + 3 + t->y, w = t->w, h = t->h;
    uint32_t ink = t->enabled ? C_DARK : C_SHADOW;
    int ty = y + (h - font()->cell) / 2;
    switch (t->cls) {
    case C_BUTTON:
        switch (btype(t)) {
        case 7:   // group box
            frame(fb, x, y + 6, w, h - 6, C_SHADOW, C_LIGHT);
            if (t->text[0]) { fill(fb, x + 6, y, text_w(t->text) + 4, 13, C_FACE); put_text(fb, x + 8, y, t->text, ink, x + w); }
            break;
        case 2: case 3: case 4: case 9: {   // check box / radio button
            int r = btype(t) == 4 || btype(t) == 9, by = y + (h - 11) / 2;
            fill(fb, x, by, 11, 11, C_LIGHT);
            frame(fb, x, by, 11, 11, C_SHADOW, C_LIGHT);
            if (t->checked) { if (r) fill(fb, x + 3, by + 3, 5, 5, C_DARK); else for (int i = 0; i < 7; i++) { fill(fb, x + 2 + i, by + 2 + i, 1, 1, C_DARK); fill(fb, x + 8 - i, by + 2 + i, 1, 1, C_DARK); } }
            put_text(fb, x + 15, ty, t->text, ink, x + w + 40);
            if (focused) frame(fb, x - 2, ty - 1, text_w(t->text) + 19, font()->cell + 2, C_FOCUS, C_FOCUS);
            break;
        }
        default:  // push button
            fill(fb, x, y, w, h, C_FACE);
            frame(fb, x, y, w, h, C_LIGHT, C_DARK);
            frame(fb, x + 1, y + 1, w - 2, h - 2, C_FACE, C_SHADOW);
            put_text(fb, x + (w - text_w(t->text)) / 2, ty, t->text, ink, x + w);
            if (btype(t) == 1) frame(fb, x - 1, y - 1, w + 2, h + 2, C_DARK, C_DARK);
            if (focused) { frame(fb, x - 2, y - 2, w + 4, h + 4, C_FOCUS, C_FOCUS); frame(fb, x - 3, y - 3, w + 6, h + 6, C_FOCUS, C_FOCUS); }
        }
        break;
    case C_STATIC:
        if ((t->style & 0x1fu) == 0x12u) { frame(fb, x, y, w, h, C_SHADOW, C_LIGHT); break; }   // SS_ETCHEDFRAME
        if ((t->style & 0x1fu) >= 4 && (t->style & 0x1fu) <= 9) break;                         // rectangles and frames
        {
            int a = (int)(t->style & 3u), tx = a == 1 ? x + (w - text_w(t->text)) / 2 : a == 2 ? x + w - text_w(t->text) : x;
            if (tx < x - 20) tx = x;
            put_text(fb, tx, (t->style & 0x200u) ? ty : y, t->text, ink, 640);   // SS_CENTERIMAGE: vertically centred
        }
        break;
    case C_EDIT:
        fill(fb, x, y, w, h, t->enabled ? C_LIGHT : C_FACE);
        frame(fb, x, y, w, h, C_SHADOW, C_LIGHT);
        put_text(fb, x + 3, ty, t->text, C_DARK, x + w - 2);
        if (focused) frame(fb, x - 2, y - 2, w + 4, h + 4, C_FOCUS, C_FOCUS);
        break;
    case C_COMBO:
        fill(fb, x, y, w, h, t->enabled ? C_LIGHT : C_FACE);
        frame(fb, x, y, w, h, C_SHADOW, C_LIGHT);
        put_text(fb, x + 3, ty, item_text(t, t->cursel), ink, x + w - 12);
        put_text(fb, x + w - 10, ty, focused ? ">" : "v", focused ? 0x0000c0u : C_SHADOW, 640);
        if (focused) { put_text(fb, x - 9, ty, "<", C_FOCUS, 640); frame(fb, x - 2, y - 2, w + 4, h + 4, C_FOCUS, C_FOCUS); }
        break;
    case C_LISTBOX: {
        fill(fb, x, y, w, h, C_LIGHT);
        frame(fb, x, y, w, h, C_SHADOW, C_LIGHT);
        int rows = (h - 4) / 14;
        for (int i = 0; i < rows && t->top + i < t->nitems; i++) {
            int it = t->top + i;
            if (it == t->cursel) fill(fb, x + 2, y + 2 + i * 14, w - 4, 14, 0x000080u);
            put_text(fb, x + 3, y + 2 + i * 14, t->items[it], it == t->cursel ? C_LIGHT : C_DARK, x + w - 2);
        }
        if (focused) frame(fb, x - 2, y - 2, w + 4, h + 4, C_FOCUS, C_FOCUS);
        break;
    }
    default: break;
    }
}

// Called by the DirectDraw present with the converted 640x480 frame.
void hk_dlg_overlay(uint32_t *fb) {
    for (int k = 1; k < MAX_DLG; k++) {
        Dlg *d = &g_dlg[k];
        if (!d->used || !d->visible) continue;
        fill(fb, d->x, d->y, d->w, d->h, C_FACE);
        frame(fb, d->x, d->y, d->w, d->h, C_LIGHT, C_DARK);
        frame(fb, d->x + 1, d->y + 1, d->w - 2, d->h - 2, C_FACE, C_SHADOW);
        fill(fb, d->x + 3, d->y + 3, d->w - 6, TITLE_H - 2, 0x000080u);
        put_text(fb, d->x + 6, d->y + 4, d->title, C_LIGHT, d->x + d->w - 4);
        for (int i = 0; i < d->n; i++) if (d->ctl[i].visible) draw_ctl(fb, d, &d->ctl[i], i == d->focus && d == top_dlg());
        if (d == top_dlg()) {
            const char *hint = "Cross/Enter: select   Circle/Esc: cancel   D-pad: move";
            int hy = d->y + d->h + 4 < 466 ? d->y + d->h + 4 : 466;
            fill(fb, (640 - text_w(hint)) / 2 - 4, hy - 1, text_w(hint) + 8, 15, 0x202020u);
            put_text(fb, (640 - text_w(hint)) / 2, hy, hint, 0xe0e0e0u, 640);
        }
    }
}

// Presents the last game frame with the dialogs on top (the game stops flipping while a dialog is up).
static void refresh(void) {
    static uint32_t last;
    uint32_t now = SDL_GetTicks();
    if (now - last < 33) return;
    last = now;
    void hk_dd_present(void);
    hk_dd_present();
}
void hk_dlg_refresh(void) { if (hk_dlg_active()) refresh(); }

// ---------- creation ----------
static uint32_t dlg_send(CPU *c, Dlg *d, uint32_t msg, uint32_t wp, uint32_t lp) {
    return d->proc ? rt_guest_call(c, d->proc, 4, d->hwnd, msg, wp, lp) : 0;
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
    if (t->cls == C_LISTBOX) { int rows = (t->h - 4) / 14; if (rows < 1) rows = 1; if (n < t->top) t->top = n; if (n >= t->top + rows) t->top = n - rows + 1; }
    command(c, d, t, 1);
}
static void move_focus(Dlg *d, int dx, int dy) {
    if (d->focus < 0) { set_focus_default(d); return; }
    Ctl *f = &d->ctl[d->focus];
    int fx = f->x + f->w / 2, fy = f->y + f->h / 2, best = -1, bestv = 1 << 30;
    for (int i = 0; i < d->n; i++) {
        Ctl *t = &d->ctl[i];
        if (i == d->focus || !focusable(t)) continue;
        int tx = t->x + t->w / 2, ty = t->y + t->h / 2, ax = (tx - fx) * dx + (ty - fy) * dy;   // along the direction
        int across = abs(dx ? ty - fy : tx - fx);
        if (ax <= 2) continue;
        int v = ax + 3 * across;
        if (v < bestv) { bestv = v; best = i; }
    }
    if (best >= 0) d->focus = best;
}
enum { K_UP = 1, K_DOWN = 2, K_LEFT = 4, K_RIGHT = 8, K_OK = 16, K_CANCEL = 32 };
static void dlg_input(CPU *c, int keys) {
    Dlg *d = top_dlg();
    if (!d) return;
    Ctl *f = d->focus >= 0 ? &d->ctl[d->focus] : NULL;
    if (keys & K_CANCEL) {
        Ctl *cancel = ctl_by_id(d, 2);
        dlg_send(c, d, 0x111u, 2u, cancel ? ctl_hwnd(d, cancel) : 0);   // IDCANCEL
        return;
    }
    if (keys & K_OK) { if (f) click(c, d, f); return; }
    int lr = (keys & K_LEFT) ? -1 : (keys & K_RIGHT) ? 1 : 0;
    if (lr && f && (f->cls == C_COMBO || f->cls == C_LISTBOX)) { change(c, d, f, lr); return; }
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
// Key transitions while a dialog is up: called by USER32's message functions instead of posting WM_KEYDOWN.
// Several keys pressed together (pad Cross sends X and space) count as one action.
void hk_dlg_keys(CPU *c, const uint8_t *pressed_vks, int n) {
    int keys = 0;
    for (int i = 0; i < n; i++)
        switch (pressed_vks[i]) {
        case 0x26: keys |= K_UP; break;
        case 0x28: keys |= K_DOWN; break;
        case 0x25: keys |= K_LEFT; break;
        case 0x27: keys |= K_RIGHT; break;
        case 0x0D: case 0x20: case 'X': keys |= K_OK; break;
        case 0x1B: case 'Z': keys |= K_CANCEL; break;
        default: break;
        }
    if (keys) dlg_input(c, keys);
    refresh();
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
        if (style & 0x10000000u) d->visible = 1;
        refresh();
    }
    STDRET(5, d ? d->hwnd : 0);
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
    if (d->used) { dlg_send(c, d, 0x2u, 0, 0); dlg_free(d); }   // WM_DESTROY
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
    case 0xC5: return 0;                                                             // EM_LIMITTEXT
    case 0xF5: click(c, d, t); return 0;                                             // BM_CLICK
    }
    if (cb || t->cls == C_LISTBOX) {
        uint32_t add = cb ? 0x143 : 0x180, ins = cb ? 0x14A : 0x181, reset = cb ? 0x14B : 0x184, setsel = cb ? 0x14E : 0x186,
                 getsel = cb ? 0x147 : 0x188, count = cb ? 0x146 : 0x18B, gettext = cb ? 0x148 : 0x189,
                 textlen = cb ? 0x149 : 0x18A, getdata = cb ? 0x150 : 0x199, setdata = cb ? 0x151 : 0x19A,
                 find = cb ? 0x14C : 0x18F, findx = cb ? 0x158 : 0x1A2, del = cb ? 0x144 : 0x182;
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
