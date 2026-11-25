// The game's Win32 dialogs and menu bar in the pause menu's style (port/vita/pause_menu.h): crisp 960x544 overlay, the
// title screen's 9-slice box, the title-word glyphs at 2x for titles, labels and values, red dots on the selected row, the
// pad symbols in the hint line. Pure drawing from a HkDlgView (hk/dlgview.h, filled by port/hk/dialog.c); the dialog
// procedures of the game still own every state. Include after pause_menu.h. Previewed on PC by running the game
// (HK_SHOT) and by port/vita/pause_preview.c.
#ifndef HK_DLG_MENU_H
#define HK_DLG_MENU_H
#include "hk/dlgview.h"

typedef struct { int x0, y0, x1, y1; } DvBox;
#define DV_SPILL 12   // px a row's ink (accent marks, outline, shadow, dots) can reach beyond its own band
static int g_dv_band0, g_dv_band1;   // slots: when band1 > band0, draw only the rows overlapping y in [band0, band1) over a plain panel fill (a cursor move)
static DvBox g_dv_box;   // the panel of the last layout (the overlay is rebuilt only inside it)

// Lines of a static text (one screen line per text line, '\r' dropped).
static int dv_lines(const char *text, char lines[][128], int max) {
    int nl = 0;
    for (const char *p = text; *p && nl < max; ) {
        int n = 0;
        while (*p && *p != '\n') { if (*p != '\r' && n < 127) lines[nl][n++] = *p; p++; }
        if (*p == '\n') p++;
        lines[nl][n] = 0;
        nl++;
    }
    return nl ? nl : 1;
}
// Short texts use the title-word glyphs, long ones the plain font so they stay readable and narrow.
static int dv_big_ok(char lines[][128], int nl) {
    for (int i = 0; i < nl; i++) if (pm_bigw(lines[i], pm_h) > 560 || hk_tw_width_h(lines[i], pm_h) < 0) return 0;
    return 1;
}
static int dv_tw(const char *s, int big) { return big ? pm_bigw(s, pm_h) : hk_gf_width_h(s, pm_h); }
// Text clipped to maxw with ".." (plain font): used for names that can be long.
static void dv_clip(const char *s, int maxw, int h, char *out, size_t cap) {
    snprintf(out, cap, "%s", s);
    while (out[0] && hk_gf_width_h(out, h) > maxw) {
        size_t n = strlen(out);
        if (n <= 3) { out[n - 1] = 0; continue; }
        out[n - 3] = '.'; out[n - 2] = '.'; out[n - 1] = 0;
        if (hk_gf_width_h(out, h) <= maxw) break;
        out[n - 3] = 0;
    }
}
// A row's text: title glyphs when they can be built and fit in maxw, else the plain font. Returns the width drawn.
static int dv_row_text(uint32_t *buf, int bw, int x, int y, const char *s, int style, int maxw) {
    char u[200];
    hk_gf_upper(u, s, sizeof u);
    if (hk_art_ok() && hk_tw_width_h(u, pm_h) >= 0 && hk_tw_width_h(u, pm_h) <= maxw) return buf ? pm_big(buf, bw, x, y, s, style, pm_h) : hk_tw_width_h(u, pm_h);
    char c[128];
    dv_clip(u, maxw, pm_h, c, sizeof c);
    return buf ? hk_gf_text_h(buf, bw, x, y, c, style, pm_h) : hk_gf_width_h(c, pm_h);
}
static int dv_row_w(const char *s, int maxw) { return dv_row_text(NULL, 0, 0, 0, s, 0, maxw); }

// Small square mark of a radio button / selection (filled when set).
static void dv_mark(uint32_t *buf, int bw, int x, int y, int on) {
    const int m = pm_h * 3 + 4, q = pm_h / 2;
    pm_rect(buf, bw, x, y, x + m, y + m, 0x191a19u);
    pm_rect(buf, bw, x + q, y + q, x + m - q, y + m - q, 0x433a89u);
    pm_rect(buf, bw, x + 2 * q, y + 2 * q, x + m - 2 * q, y + m - 2 * q, 0x2c1e6du);
    if (on) pm_rect(buf, bw, x + 3 * q, y + 3 * q, x + m - 3 * q, y + m - 3 * q, 0xfbfddau);
}

// ---- form: heading, rows, the OK / Cancel / Reset buttons, hint ----
static int dv_form_at(uint32_t *buf, int bw, const HkDlgView *v) {
    const int PITCH = PM_CELL + (pm_h >= 4 ? 0 : 6), LH = PM_CELL + (pm_h >= 4 ? 0 : 4), H = HK_CANVAS_H(bw);
    pm_hs = pm_h >= 4 ? 4 : 2;
    if (pm_hs == 4 && pm_rich_w(v->hint, 2) + 56 > bw - 160) pm_hs = 2;
    const int hc = 8 * pm_hs;
    const int m = pm_h * 3 + 4;
    int onlytext = 1, lw = 0, cw[2] = {0, 0}, need = pm_bigw(v->title, pm_h + 2) + 96, rowsh = 0, ew = 0, ncols = 0;
    const char *onoff[2] = {hk_tr(S_ON), hk_tr(S_OFF)};
    int onw = pm_rich_w(onoff[0], 1) > pm_rich_w(onoff[1], 1) ? pm_rich_w(onoff[0], 1) : pm_rich_w(onoff[1], 1);
    for (int i = 0; i < v->nrows; i++) {
        const DvRow *r = &v->row[i];
        if (r->kind != RK_TEXT) onlytext = 0;
        int w;
        switch (r->kind) {
        case RK_TEXT: {
            char ln[4][128];
            int nl = dv_lines(r->label, ln, 4), big = dv_big_ok(ln, nl);
            for (int k = 0; k < nl; k++) { w = dv_tw(ln[k], big) + 96; if (w > need) need = w; }
            rowsh += nl * LH + 6;
            break;
        }
        case RK_SECTION: w = pm_uw(r->label) + 120; if (w > need) need = w; rowsh += PITCH + 4; break;
        case RK_COLHEAD: rowsh += PITCH - 4; ncols = ncols > 2 ? ncols : 2; for (int c = 0; c < 2; c++) { w = pm_uw(r->val[c]); if (w > cw[c]) cw[c] = w; } break;
        case RK_BUTTON: w = pm_uw(r->label) + 96; if (w > need) need = w; rowsh += PITCH; break;
        default:
            w = dv_row_w(r->label, 380); if (w > lw) lw = w;
            if (r->kind == RK_VALUE) { ncols = ncols > r->ncell ? ncols : r->ncell; for (int c = 0; c < r->ncell; c++) { w = pm_rich_w(r->val[c], 1); if (w > cw[c]) cw[c] = w; } }
            else if (r->kind == RK_EDIT) { w = hk_gf_width_h(r->val[0], pm_h) + 24; if (w > ew) ew = w; }
            rowsh += PITCH;
        }
    }
    if (ew < 70 * pm_h) ew = 70 * pm_h;
    int vcols = cw[0] + (cw[1] ? 30 + cw[1] : 0);
    if (vcols < onw) vcols = onw;
    int has_edit = 0, has_lab = 0;
    for (int i = 0; i < v->nrows; i++) { has_edit |= v->row[i].kind == RK_EDIT; has_lab |= v->row[i].kind >= RK_CHECK && v->row[i].kind != RK_BUTTON; }
    if (has_lab) { int w = lw + 20 * pm_h + (has_edit && ew > vcols ? ew : vcols) + 96; if (w > need) need = w; }
    int btw = 0;
    for (int i = 0; i < v->nbtn; i++) btw += pm_uw(v->btn[i].label) + (i ? 12 * pm_h : 0);
    if (btw + 96 > need) need = btw + 96;
    if (pm_rich_w(v->hint, 2) + 56 > need) need = pm_rich_w(v->hint, 2) + 56;
    int boxw = need < (onlytext ? 320 : 360) ? (onlytext ? 320 : 360) : need;
    const int btnh = v->nbtn ? PITCH + 6 : 0;
    const int boxh = 14 + PM_HEAD_H + 8 + rowsh + btnh + 6 + 2 + 8 + hc + 12;
    if (boxw > bw - 16) boxw = bw - 16;
    const int x0 = (bw - boxw) / 2, y0 = (H - boxh) / 2, x1 = x0 + boxw, y1 = y0 + boxh;
    g_dv_box = (DvBox){x0, y0, x1, y1};
    if (!buf) return boxw <= bw - 16 && boxh <= H - 4 && need <= bw - 16;
    pm_panel(buf, bw, x0, y0, x1, y1);
    int y = y0 + 14;
    pm_heading(buf, bw, bw / 2, y, v->title);
    y += PM_HEAD_H + 8;
    const int lx = x0 + 40, rx = x1 - 40;
    const int c1r = rx, c0r = cw[1] ? rx - cw[1] - 30 : rx;   // right edges of the value columns
    for (int i = 0; i < v->nrows; i++) {
        const DvRow *r = &v->row[i];
        const int on = r->focus >= 0, dis = !r->enabled;
        const int style = dis ? GF_DIM : on ? GF_SEL : GF_NORMAL;
        switch (r->kind) {
        case RK_TEXT: {
            char ln[4][128];
            int nl = dv_lines(r->label, ln, 4), big = dv_big_ok(ln, nl);
            for (int k = 0; k < nl; k++) {
                int tw = dv_tw(ln[k], big);
                if (big) pm_big(buf, bw, (bw - tw) / 2, y, ln[k], onlytext ? GF_SEL : GF_NORMAL, pm_h);
                else hk_gf_text_h(buf, bw, (bw - tw) / 2, y, ln[k], onlytext ? GF_SEL : GF_NORMAL, pm_h);
                y += LH;
            }
            y += 6;
            break;
        }
        case RK_SECTION: {
            int hy = y + 2;
            pm_utext(buf, bw, x0 + 24, hy, r->label, GF_GOLD);
            pm_rect(buf, bw, x0 + 30 + pm_uw(r->label), hy + 7 * pm_h / 2, x1 - 24, hy + 7 * pm_h / 2 + 2, 0x433a89u);
            y += PITCH + 4;
            break;
        }
        case RK_COLHEAD:
            for (int c = 0; c < 2; c++) { int ce = c ? c1r : c0r; pm_big(buf, bw, ce - cw[c] / 2 - pm_bigw(r->val[c], pm_h) / 2 - (cw[c] ? 0 : 0) + 0, y, r->val[c], GF_GOLD, pm_h); }
            y += PITCH - 4;
            break;
        case RK_BUTTON: {
            int tw = pm_uw(r->label), tx = (bw - tw) / 2;
            pm_utext(buf, bw, tx, y, r->label, style);
            if (on) pm_dots(buf, bw, tx, tx + tw, y);
            y += PITCH;
            break;
        }
        default: {
            const char *lab = r->label;
            int labw = dv_row_w(lab, 380);
            if (lab[0]) dv_row_text(buf, bw, lx, y, lab, style, 380);
            if (r->kind == RK_CHECK) {
                const char *s = onoff[r->checked ? 0 : 1];
                pm_rich(buf, bw, rx - pm_rich_w(s, 1), y, s, on ? GF_GOLD : style, 1);
                if (on) pm_dots(buf, bw, lx, rx, y);
            } else if (r->kind == RK_RADIO) {
                dv_mark(buf, bw, rx - m, y + (PM_CELL - m) / 2, r->checked);
                if (on) pm_dots(buf, bw, lx, rx, y);
            } else if (r->kind == RK_EDIT) {
                const int ex0 = lab[0] ? rx - (ew > rx - lx - labw - 20 * pm_h ? rx - lx - labw - 20 * pm_h : ew) : lx;
                pm_rect(buf, bw, ex0, y - 2, rx, y + PM_CELL, 0x433a89u);
                pm_rect(buf, bw, ex0 + 2, y, rx - 2, y + PM_CELL - 2, 0x191a19u);
                char c[128];
                dv_clip(r->val[0], rx - ex0 - 28, pm_h, c, sizeof c);
                int tw = hk_gf_text_h(buf, bw, ex0 + 10, y, c, on ? GF_SEL : GF_NORMAL, pm_h);
                if (on) pm_rect(buf, bw, ex0 + 12 + tw, y + 4, ex0 + 12 + tw + pm_h, y + PM_CELL - 6, 0xfbfddau);
                if (on) pm_dots(buf, bw, ex0, rx, y);
            } else {
                for (int c = 0; c < r->ncell; c++) {
                    const int ce = r->ncell == 1 || c == 1 ? c1r : c0r, w = pm_rich_w(r->val[c], 1);
                    const int fc = r->focus == c;
                    pm_rich(buf, bw, ce - w, y, r->val[c], fc ? GF_GOLD : dis ? GF_DIM : GF_NORMAL, 1);
                    if (fc) pm_dots(buf, bw, ce - w, ce, y);
                }
                if (on && r->ncell == 1) pm_dots(buf, bw, lx, rx, y);
            }
            y += PITCH;
        }
        }
    }
    if (v->nbtn) {
        y += 6;
        int tot = 0;
        for (int i = 0; i < v->nbtn; i++) tot += pm_uw(v->btn[i].label) + (i ? 12 * pm_h : 0);
        int x = (bw - tot) / 2;
        for (int i = 0; i < v->nbtn; i++) {
            const DvBtn *b = &v->btn[i];
            int tw = pm_uw(b->label);
            pm_utext(buf, bw, x, y - 4, b->label, !b->enabled ? GF_DIM : b->focus ? GF_SEL : GF_NORMAL);
            if (b->focus) pm_dots(buf, bw, x, x + tw, y - 4);
            x += tw + 12 * pm_h;
        }
    }
    pm_rect(buf, bw, x0 + 20, y1 - 12 - hc - 10, x1 - 20, y1 - 12 - hc - 8, 0x433a89u);
    pm_rich(buf, bw, (bw - pm_rich_w(v->hint, 2)) / 2, y1 - 12 - hc, v->hint, GF_NORMAL, 2);
    if (v->drop) {   // the open list of a combo box: its own box over the form, the choice in the middle
        const int n = v->dropn;
        int w = 0;
        for (int i = 0; i < n; i++) if (pm_rich_w(v->dropitem[i], 1) > w) w = pm_rich_w(v->dropitem[i], 1);
        w += 120;
        const int dh = 30 + n * PITCH, dx0 = (bw - w) / 2, dy0 = (H - dh) / 2;
        pm_rect(buf, bw, dx0, dy0, dx0 + w, dy0 + dh, PM_FILL);
        pm_panel(buf, bw, dx0, dy0, dx0 + w, dy0 + dh);
        for (int i = 0; i < n; i++) {
            int tw = pm_rich_w(v->dropitem[i], 1), tx = (bw - tw) / 2, ty = dy0 + 15 + i * PITCH;
            pm_rich(buf, bw, tx, ty, v->dropitem[i], i == v->dropsel ? GF_SEL : GF_NORMAL, 1);
            if (i == v->dropsel) pm_dots(buf, bw, tx, tx + tw, ty);
        }
    }
    return 1;
}

// ---- GAME DATA (dialog 1500): the slot table, as large as RESULTS ----
static int dv_slots_at(uint32_t *buf, int bw, const HkDlgView *v) {
    const int PITCH = PM_CELL + (pm_h >= 4 ? 0 : 6), H = HK_CANVAS_H(bw), sm = pm_h >= 4 ? 3 : 2;
    pm_hs = pm_h >= 4 ? 4 : 2;
    if (pm_hs == 4 && pm_rich_w(v->hint, 2) + 56 > bw - 160) pm_hs = 2;
    const int hc = 8 * pm_hs, hdr = 8 * sm + 8;
    int numw, areaw, datew, namew;
    {   // the column widths depend only on the slots' texts: kept for the next call on the same view (layout, draw and band passes ask repeatedly)
        static uint64_t ck; static int ch = -1, cl = -1, cu = -1, cn, ca, cd, cm;
        const uint64_t k = hk_dlgview_hash(v);
        if (k != ck || ch != pm_h || cl != g_hk_lang || cu != g_pm_ui) {
            cn = pm_bigw("00", pm_h); ca = pm_bigw(hk_tr(S_D_AREA), sm); cd = pm_bigw(hk_tr(S_D_DATE), sm); cm = 0;
            for (int i = 0; i < v->nslots; i++) {
                const DvSlot *s = &v->slot[i];
                int w = s->area[0] ? pm_bigw(s->area, pm_h) : 0; if (w > ca) ca = w;
                w = hk_gf_width_h(s->date, pm_h); if (w > cd) cd = w;
                w = dv_row_w(s->name[0] ? s->name : hk_tr(S_D_EMPTY), 1000); if (w > cm) cm = w;
            }
            ck = k; ch = pm_h; cl = g_hk_lang; cu = g_pm_ui;
        }
        numw = cn; areaw = ca; datew = cd; namew = cm;
    }
    const int note = v->note[0] != 0, gap = 28;
    int boxw = 200 * pm_h;   // RESULTS' width
    int need = 56 + numw + gap + (namew < 8 * pm_h * 5 ? 8 * pm_h * 5 : namew) + gap + areaw + gap + datew + 56;
    if (need > boxw) boxw = need;
    if (pm_rich_w(v->hint, 2) + 56 > boxw) boxw = pm_rich_w(v->hint, 2) + 56;
    if (boxw > bw - 16) boxw = bw - 16;
    const int boxh = 18 + PM_HEAD_H + 8 + hdr + v->nslots * PITCH + (note ? hc + 10 : 0) + 8 + 2 + 10 + hc + 14;
    const int x0 = (bw - boxw) / 2, y0 = (H - boxh) / 2, x1 = x0 + boxw, y1 = y0 + boxh;
    g_dv_box = (DvBox){x0, y0, x1, y1};
    if (!buf) return need <= bw - 16 && boxh <= H - 8;
    const int band = g_dv_band1 > g_dv_band0;
    if (band) pm_blend(buf, bw, x0 + 3, g_dv_band0, x1 - 3, g_dv_band1, PM_FILL, 12);
    else pm_panel(buf, bw, x0, y0, x1, y1);
    int y = y0 + 18;
    if (!band) pm_heading(buf, bw, bw / 2, y, v->title);
    y += PM_HEAD_H + 8;
    const int lx = x0 + 56, rx = x1 - 56;
    const int dx = rx - datew, ax = dx - gap - areaw, nx = lx + numw + gap, nmax = ax - gap - nx;
    if (!band || g_dv_band0 < y + 8 * sm + 4 + DV_SPILL) {   // the header's ink can reach the first band
        pm_big(buf, bw, lx, y, "#", GF_GOLD, sm);
        pm_big(buf, bw, nx, y, hk_tr(S_D_NAME), GF_GOLD, sm);
        pm_big(buf, bw, ax, y, hk_tr(S_D_AREA), GF_GOLD, sm);
        pm_big(buf, bw, dx, y, hk_tr(S_D_DATE), GF_GOLD, sm);
    }
    y += hdr - 4;
    if (!band || g_dv_band0 < y - 4) pm_rect(buf, bw, x0 + 20, y - 6, x1 - 20, y - 4, 0x433a89u);
    for (int i = 0; i < v->nslots; i++) {
        const DvSlot *s = &v->slot[i];
        const int on = s->sel, ys = y + i * PITCH;
        // A row's bar is [ys-1, ys+CELL-1); its text and dots lie in [ys-2h-2, ys+6h+2) (accent marks above, shadow below).
        const int bar_in = !band || (ys - 1 < g_dv_band1 && ys + PM_CELL - 1 > g_dv_band0);
        const int ink_in = !band || (ys - 2 * pm_h - 2 < g_dv_band1 && ys + 6 * pm_h + 2 > g_dv_band0);
        if (!bar_in && !ink_in) continue;
        const int style = !s->enabled ? GF_DIM : on ? GF_SEL : GF_NORMAL;
        if (on && bar_in) pm_blend(buf, bw, x0 + 22, ys - 1, x1 - 22, ys + PM_CELL - 1, 0x433a89u, 7);
        if (!ink_in) continue;
        char num[8];
        snprintf(num, sizeof num, "%02d", i + 1);
        pm_utext(buf, bw, lx, ys, num, on ? GF_SEL : GF_GOLD);
        if (s->empty || !s->name[0]) dv_row_text(buf, bw, nx, ys, hk_tr(S_D_EMPTY), GF_DIM, nmax + gap + areaw + gap + datew);
        else {
            dv_row_text(buf, bw, nx, ys, s->name, style, nmax);
            if (s->area[0]) {
                char a[16];
                hk_gf_upper(a, s->area, sizeof a);
                for (char *p = a; *p; p++) if (*p == '_') *p = '-';
                pm_big(buf, bw, ax, ys, a, on ? GF_GOLD : style, pm_h);
            }
            hk_gf_text_h(buf, bw, dx, ys, s->date, on ? GF_SEL : GF_NORMAL, pm_h);
        }
        if (on) pm_dots(buf, bw, lx, rx, ys);
    }
    y += v->nslots * PITCH;
    if (band && g_dv_band1 + DV_SPILL <= y) return 1;   // the last rows' bands also see the note, rule and hint
    if (note) pm_rich(buf, bw, (bw - pm_rich_w(v->note, 2)) / 2, y + 6, v->note, GF_GOLD, 2);
    pm_rect(buf, bw, x0 + 20, y1 - 14 - hc - 12, x1 - 20, y1 - 14 - hc - 10, 0x433a89u);
    pm_rich(buf, bw, (bw - pm_rich_w(v->hint, 2)) / 2, y1 - 14 - hc, v->hint, GF_NORMAL, 2);
    return 1;
}

// ---- the menu bar: the titles in a row, the open menu under them ----
static int dv_menu_at(uint32_t *buf, int bw, const HkDlgView *v) {
    const int PITCH = PM_CELL + (pm_h >= 4 ? 2 : 6), H = HK_CANVAS_H(bw), tgap = 20 * pm_h;
    pm_hs = pm_h >= 4 ? 4 : 2;
    if (pm_hs == 4 && pm_rich_w(v->hint, 2) + 56 > bw - 160) pm_hs = 2;
    const int hc = 8 * pm_hs;
    int tabw = 0, iw = 0, kw = 0, rows = 0;
    for (int i = 0; i < v->ntop; i++) tabw += pm_uw(v->top[i]) + (i ? tgap : 0);
    for (int i = 0; i < v->nitems; i++) {
        if (v->item[i].sep) { rows += 12; continue; }
        int w = pm_uw(v->item[i].text), k = v->item[i].key[0] ? pm_rich_w(v->item[i].key, 1) : 0;
        if (w > iw) iw = w;
        if (k > kw) kw = k;
        rows += PITCH;
    }
    int need = tabw + 2 * 6 * pm_h + 96, w2 = iw + (kw ? kw + 24 * pm_h : 0) + 96;
    if (w2 > need) need = w2;
    if (pm_rich_w(v->hint, 2) + 56 > need) need = pm_rich_w(v->hint, 2) + 56;
    const int boxw = need > bw - 16 ? bw - 16 : need;
    const int boxh = 18 + PITCH + (v->nitems ? 16 + rows : 0) + 8 + 2 + 10 + hc + 14;
    const int x0 = (bw - boxw) / 2, y0 = (H - boxh) / 2, x1 = x0 + boxw, y1 = y0 + boxh;
    g_dv_box = (DvBox){x0, y0, x1, y1};
    if (!buf) return need <= bw - 16 && boxh <= H - 8;
    pm_panel(buf, bw, x0, y0, x1, y1);
    int y = y0 + 18, x = (bw - tabw) / 2;
    for (int i = 0; i < v->ntop; i++) {
        int tw = pm_uw(v->top[i]), sel = i == v->topsel;
        pm_utext(buf, bw, x, y, v->top[i], v->topgray[i] ? GF_DIM : sel ? GF_SEL : GF_NORMAL);
        if (sel) pm_dots(buf, bw, x, x + tw, y);
        x += tw + tgap;
    }
    y += PITCH;
    if (v->nitems) {
        pm_rect(buf, bw, x0 + 20, y + 4, x1 - 20, y + 6, 0x433a89u);
        y += 16;
        const int lx = x0 + 56, rx = x1 - 56;
        for (int i = 0; i < v->nitems; i++) {
            if (v->item[i].sep) { pm_rect(buf, bw, x0 + 40, y + 5, x1 - 40, y + 7, 0x433a89u); y += 12; continue; }
            int on = i == v->itemsel, st = v->item[i].gray ? GF_DIM : on ? GF_SEL : GF_NORMAL;
            pm_utext(buf, bw, lx, y, v->item[i].text, st);
            if (v->item[i].key[0]) pm_rich(buf, bw, rx - pm_rich_w(v->item[i].key, 1), y, v->item[i].key, v->item[i].gray ? GF_DIM : GF_NORMAL, 1);
            if (on) pm_dots(buf, bw, lx, rx, y);
            y += PITCH;
        }
    }
    pm_rect(buf, bw, x0 + 20, y1 - 14 - hc - 12, x1 - 20, y1 - 14 - hc - 10, 0x433a89u);
    pm_rich(buf, bw, (bw - pm_rich_w(v->hint, 2)) / 2, y1 - 14 - hc, v->hint, GF_NORMAL, 2);
    return 1;
}

static int dv_at(uint32_t *buf, int bw, const HkDlgView *v) {
    return v->kind == DV_SLOTS ? dv_slots_at(buf, bw, v) : v->kind == DV_MENU ? dv_menu_at(buf, bw, v) : dv_form_at(buf, bw, v);
}
static void dv_draw(uint32_t *buf, int bw, const HkDlgView *v) {
    for (pm_h = g_pm_ui >= 4 ? 4 : 2; pm_h > 2 && !dv_at(NULL, bw, v); pm_h -= 2) {}
    dv_at(buf, bw, v);
}
static void pm_draw_dlg(uint32_t *buf, const void *arg) { dv_draw(buf, PM_OW, arg); }
// Rebuilds the overlay for a view (the caller does this only when the view changed). A cursor move in the slot table
// (the view differs from the last one only in the selected slot) redraws just the changed rows' bands; g_pm_dirty is
// the bounding box of everything that changed either way.
static const uint32_t *prev_ovl;   // the buffer the last view was built into (a different one: full build)
static void pm_overlay_dlg(uint32_t *ovl, const HkDlgView *v) {
    static HkDlgView prev;
    static DvBox prev_box;
    static int prev_h, prev_lang, prev_ui;
    for (pm_h = g_pm_ui >= 4 ? 4 : 2; pm_h > 2 && !dv_at(NULL, PM_OW, v); pm_h -= 2) {}
    dv_at(NULL, PM_OW, v);
    const DvBox b = g_dv_box;
    int band_ok = prev_ovl == ovl && v->kind == DV_SLOTS && prev.kind == DV_SLOTS && !v->drop && !prev.drop && pm_h == prev_h &&
                  g_hk_lang == prev_lang && g_pm_ui == prev_ui && !memcmp(&b, &prev_box, sizeof b) && v->nslots == prev.nslots;
    if (band_ok) {
        HkDlgView t = *v;
        for (int i = 0; i < t.nslots && i < 10; i++) t.slot[i].sel = prev.slot[i].sel;
        band_ok = hk_dlgview_hash(&t) == hk_dlgview_hash(&prev);
    }
    if (band_ok) {
        const int PITCH = PM_CELL + (pm_h >= 4 ? 0 : 6), hdr = 8 * (pm_h >= 4 ? 3 : 2) + 8;
        const int ytab = b.y0 + 18 + PM_HEAD_H + 8 + hdr - 4;
        PmRect acc = {PM_OW, PM_OH, 0, 0};
        for (int i = 0; i < v->nslots && i < 10; i++) {
            if (!v->slot[i].sel == !prev.slot[i].sel) continue;
            const int y0 = ytab + i * PITCH - 2 * pm_h - 2, y1 = ytab + (i + 1) * PITCH;   // the row's bar and ink (accent marks reach h above it)
            g_dv_band0 = y0; g_dv_band1 = y1;
            g_pm_partial = 1;
            pm_overlay_build(ovl, pm_draw_dlg, v, b.x0 + 3, y0, b.x1 - b.x0 - 6, y1 - y0);
            g_pm_partial = 0;
            g_dv_band0 = g_dv_band1 = 0;
            if (g_pm_dirty.x1 > g_pm_dirty.x0) {
                if (g_pm_dirty.x0 < acc.x0) acc.x0 = g_pm_dirty.x0;
                if (g_pm_dirty.y0 < acc.y0) acc.y0 = g_pm_dirty.y0;
                if (g_pm_dirty.x1 > acc.x1) acc.x1 = g_pm_dirty.x1;
                if (g_pm_dirty.y1 > acc.y1) acc.y1 = g_pm_dirty.y1;
            }
        }
        g_pm_dirty = acc.x1 > acc.x0 ? acc : (PmRect){0, 0, 0, 0};
        g_pm_dirty_ovl = ovl;
    } else if (v->drop) pm_overlay_build(ovl, pm_draw_dlg, v, 0, 0, PM_OW, PM_OH);
    else pm_overlay_build(ovl, pm_draw_dlg, v, b.x0 - 4, b.y0 - 4, b.x1 - b.x0 + 8, b.y1 - b.y0 + 8);
    prev = *v; prev_ovl = ovl; prev_box = b; prev_h = pm_h; prev_lang = g_hk_lang; prev_ui = g_pm_ui;
}
#endif
