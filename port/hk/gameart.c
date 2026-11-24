// The game's menu art (HDKSYS00.BMP) for the port's menus; see gameart.h for the evidence and the rects.
#include "hk/gameart.h"
#include "hk/font_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char g_game_dir[512];

#define SYS_W 640
#define SYS_H 448
static uint8_t *g_idx;          // HDKSYS00 palette indices, top-down, SYS_W x SYS_H
static uint32_t g_pal[256];     // its colour table, 0xRRGGBB
static int g_state;             // 0 not tried, 1 loaded, -1 missing

static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static int load(void) {
    char path[600];
    snprintf(path, sizeof path, "%s/BITMAP/HDKSYS00.BMP", g_game_dir[0] ? g_game_dir : ".");
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint8_t h[54];
    int ok = fread(h, 1, 54, f) == 54 && h[0] == 'B' && h[1] == 'M';
    uint32_t off = ok ? rd32(h + 10) : 0, hs = ok ? rd32(h + 14) : 0;
    int32_t w = ok ? (int32_t)rd32(h + 18) : 0, ht = ok ? (int32_t)rd32(h + 22) : 0;
    ok = ok && w == SYS_W && (ht == SYS_H || ht == -SYS_H) && (h[28] | h[29] << 8) == 8 && rd32(h + 30) == 0;
    uint32_t ncol = ok ? rd32(h + 46) : 0;
    if (!ncol) ncol = 256;
    uint8_t pal[1024];
    ok = ok && ncol <= 256 && fseek(f, 14 + (long)hs, SEEK_SET) == 0 && fread(pal, 4, ncol, f) == ncol;
    uint8_t *idx = ok ? malloc(SYS_W * SYS_H) : NULL;
    ok = ok && idx && fseek(f, (long)off, SEEK_SET) == 0;
    for (int r = 0; ok && r < SYS_H; r++) ok = fread(idx + (ht > 0 ? SYS_H - 1 - r : r) * SYS_W, 1, SYS_W, f) == SYS_W;
    fclose(f);
    if (!ok) { free(idx); return 0; }
    for (uint32_t i = 0; i < 256; i++) g_pal[i] = i < ncol ? (uint32_t)pal[i * 4 + 2] << 16 | (uint32_t)pal[i * 4 + 1] << 8 | pal[i * 4] : 0;
    g_idx = idx;
    return 1;
}
int hk_art_ok(void) {
    if (!g_state) g_state = load() ? 1 : -1;
    return g_state > 0;
}

static void put(uint32_t *buf, int bw, int x, int y, uint32_t c) {
    if (x >= 0 && x < bw && y >= 0 && y < HK_CANVAS_H(bw)) buf[y * bw + x] = c;
}
void hk_art_blit(uint32_t *buf, int bw, int dx, int dy, int sx, int sy, int w, int h) {
    if (!hk_art_ok()) return;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int x = sx + i, y = sy + j;
            if (x < 0 || x >= SYS_W || y < 0 || y >= SYS_H) continue;
            uint8_t v = g_idx[y * SYS_W + x];
            if (v) put(buf, bw, dx + i, dy + j, g_pal[v]);
        }
}

// The title's menu box (0x408C15 draws it with 16-pixel corners and 32-pixel edge tiles from (0,56)).
void hk_art_frame(uint32_t *buf, int bw, int x, int y, int w, int h) {
    if (w < 32) w = 32;
    if (h < 32) h = 32;
    if (!hk_art_ok()) {   // flat fallback in the box colours
        for (int k = 0; k < 3; k++)
            for (int i = x + k; i < x + w - k; i++) { put(buf, bw, i, y + k, 0x6c5bc5u); put(buf, bw, i, y + h - 1 - k, 0x5947a5u); }
        for (int k = 0; k < 3; k++)
            for (int j = y + k; j < y + h - k; j++) { put(buf, bw, x + k, j, 0x6c5bc5u); put(buf, bw, x + w - 1 - k, j, 0x5947a5u); }
        return;
    }
    for (int i = 16; i < w - 16; i += 32) {
        int tw = w - 16 - i < 32 ? w - 16 - i : 32;
        hk_art_blit(buf, bw, x + i, y, 16, 56, tw, 16);
        hk_art_blit(buf, bw, x + i, y + h - 16, 16, 104, tw, 16);
    }
    for (int j = 16; j < h - 16; j += 32) {
        int th = h - 16 - j < 32 ? h - 16 - j : 32;
        hk_art_blit(buf, bw, x, y + j, 0, 72, 16, th);
        hk_art_blit(buf, bw, x + w - 16, y + j, 48, 72, 16, th);
    }
    hk_art_blit(buf, bw, x, y, 0, 56, 16, 16);
    hk_art_blit(buf, bw, x + w - 16, y, 48, 56, 16, 16);
    hk_art_blit(buf, bw, x, y + h - 16, 0, 104, 16, 16);
    hk_art_blit(buf, bw, x + w - 16, y + h - 16, 48, 104, 16, 16);
}
void hk_art_dot(uint32_t *buf, int bw, int x, int y) { hk_art_dot_h(buf, bw, x, y, 2); }
// Floor of i * h / 2 (h = scale in halves), for nearest-neighbour scaling by 1, 1.5, 2, 2.5, 3 ...
static int hs(int i, int h) { int v = i * h; return v >= 0 ? v / 2 : -((-v + 1) / 2); }
void hk_art_dot_h(uint32_t *buf, int bw, int x, int y, int h) {
    if (h != 2 && hk_art_ok()) {
        for (int j = 0; j < 8; j++)
            for (int i = 0; i < 8; i++) {
                uint8_t v = g_idx[j * SYS_W + i];
                if (v) for (int py = hs(j, h); py < hs(j + 1, h); py++) for (int px = hs(i, h); px < hs(i + 1, h); px++) put(buf, bw, x + px, y + py, g_pal[v]);
            }
        return;
    }
    if (hk_art_ok()) { hk_art_blit(buf, bw, x, y, 0, 0, 8, 8); return; }
    for (int j = 0; j < 8; j++)
        for (int i = 0; i < 8; i++) { int dx = 2 * i - 7, dy = 2 * j - 7; if (dx * dx + dy * dy <= 52) put(buf, bw, x + i, y + j, dx * dx + dy * dy <= 12 ? 0xf6261cu : 0xa22d1eu); }
}

// ---------- font ----------
// Glyph boxes [x0, x1) of the HDKSYS00 font, measured on the bitmap (columns with any non-zero pixel).
static const uint16_t g_row1[51][2] = {   // '!'..'S', cell rows y 1..16
    {163,167},{169,175},{176,185},{186,194},{195,208},{209,219},{221,224},{226,230},{232,236},{238,243},{245,252},
    {255,258},{261,265},{268,271},{273,277},{279,287},{290,295},{298,306},{308,315},{317,325},{326,334},{336,344},
    {346,353},{355,362},{363,371},{373,376},{378,381},{383,391},{393,401},{403,410},{412,420},{421,435},{437,447},
    {449,458},{459,469},{471,480},{482,490},{492,500},{501,511},{513,522},{524,527},{529,536},{538,548},{549,557},
    {559,570},{572,581},{582,593},{595,604},{605,616},{617,627},{628,637}};
static const uint16_t g_row2[39][2] = {   // 'T'..'z', cell rows y 17..32 ('\\' is the yen sign in the bitmap)
    {297,306},{307,316},{318,327},{328,341},{343,352},{353,362},{363,372},{373,377},{378,386},{388,392},{393,400},
    {401,409},{411,414},{416,423},{425,433},{435,442},{444,452},{453,461},{462,467},{468,476},{478,486},{487,490},
    {492,496},{497,505},{506,509},{511,523},{524,532},{534,542},{543,552},{553,561},{563,568},{569,577},{578,583},
    {584,592},{593,601},{602,613},{614,622},{623,631},{632,639}};
#define SPACE_ADV 5

enum { AC_NONE, AC_GRAVE, AC_ACUTE, AC_CIRC, AC_TILDE, AC_DIAER, AC_CED };
static uint8_t decompose(uint8_t ch, int *acc) {
    static const char base[64] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
    static const uint8_t kind[64] = {
        AC_GRAVE, AC_ACUTE, AC_CIRC, AC_TILDE, AC_DIAER, AC_NONE, AC_NONE, AC_CED, AC_GRAVE, AC_ACUTE, AC_CIRC, AC_DIAER,
        AC_GRAVE, AC_ACUTE, AC_CIRC, AC_DIAER, AC_NONE, AC_TILDE, AC_GRAVE, AC_ACUTE, AC_CIRC, AC_TILDE, AC_DIAER, AC_NONE,
        AC_NONE, AC_GRAVE, AC_ACUTE, AC_CIRC, AC_DIAER, AC_ACUTE, AC_NONE, AC_NONE};
    *acc = AC_NONE;
    if (ch < 0xC0) return ch >= 0x20 && ch < 0x7f ? ch : '?';
    int i = ch - 0xC0;
    *acc = kind[i & 31];
    return (uint8_t)base[i];
}
// Source box of a glyph: returns its width, 0 for the space; *sx, *sy: left column and top row of the cell.
static int glyph_box(uint8_t c, int *sx, int *sy) {
    if (c == '{') c = '('; else if (c == '}') c = ')'; else if (c == '|') c = '!'; else if (c == '~') c = '-';
    else if (c == '\\') c = '/';
    if (c >= '!' && c <= 'S') { *sx = g_row1[c - '!'][0]; *sy = 1; return g_row1[c - '!'][1] - *sx; }
    if (c >= 'T' && c <= 'z') { *sx = g_row2[c - 'T'][0]; *sy = 17; return g_row2[c - 'T'][1] - *sx; }
    return 0;
}

#define MW 1100
#define MH (HK_GF_CELL + 6)   // rows -3 .. cell + 2
#define MY 3
static uint8_t g_m[MH][MW];   // glyph intensity 0..3 (one string at a time)

static void mark(int x, int top, int w, int acc) {   // accent over (or cedilla under) a glyph of width w at x
    static const char *const pat[7][2] = {{"", ""}, {"##..", ".##."}, {"..##", ".##."}, {".##.", "#..#"},
                                          {".##.#", "#.##."}, {"##.##", "##.##"}, {".#.", "##."}};
    const char *const *p = pat[acc];
    int pw = (int)strlen(p[0]), x0 = x + (w - pw) / 2;
    for (int r = 0; r < 2; r++)
        for (int i = 0; i < pw; i++)
            if (p[r][i] == '#' && x0 + i >= 0 && x0 + i < MW && top + r + MY >= 0 && top + r + MY < MH) g_m[top + r + MY][x0 + i] = 3;
}
static int lay_out(const char *s, int *wout) {   // fills g_m from column 2; returns the advance (scale 1)
    memset(g_m, 0, sizeof g_m);
    int x = 2;
    for (const uint8_t *p = (const uint8_t *)s; *p; p++) {
        int acc, sx, sy;
        uint8_t c = decompose(*p, &acc);
        int w = glyph_box(c, &sx, &sy);
        if (!w) { x += SPACE_ADV; continue; }
        if (x + w + 2 >= MW) break;
        for (int j = 0; j < HK_GF_CELL; j++)
            for (int i = 0; i < w; i++) {
                uint8_t v = g_idx[(sy + j) * SYS_W + sx + i];
                g_m[j + MY][x + i] = v == 255 ? 3 : v == 7 ? 2 : v == 248 ? 1 : 0;   // other indices: neighbouring art
            }
        if (acc != AC_NONE) {
            int upper = c >= 'A' && c <= 'Z';
            if (c == 'i') for (int j = 0; j < 5; j++) for (int i = 0; i < w; i++) g_m[j + MY][x + i] = 0;   // dotless i
            mark(x, acc == AC_CED ? 13 : upper ? -1 : 2, w, acc);
        }
        x += w + 1;
    }
    *wout = x + 2;
    return x - 2 > 0 ? x - 3 : 0;
}

typedef struct { uint32_t top, fill, low, outline, shadow; } Style;
static const Style g_style[] = {
    [GF_SEL] = {0xfbfddau, 0xfbfddau, 0xdcd6c6u, 0x4f524du, 0x2d2a2fu},
    [GF_NORMAL] = {0xb3a590u, 0x998979u, 0x857767u, 0x4f524du, 0x2d2a2fu},
    [GF_DIM] = {0x625a50u, 0x5a5248u, 0x4e463eu, 0x2d2a2fu, 0x191a19u},
    [GF_HEAD] = {0x628ba6u, 0x4c4dcau, 0x433a89u, 0x191a19u, 0x2d2a2fu},
    [GF_GOLD] = {0xffe890u, 0xf0c050u, 0xc89030u, 0x4f3a1du, 0x2d2a2fu},
};
static uint32_t mix(uint32_t a, uint32_t b, int wa) {   // wa/4 of a
    uint32_t r = 0;
    for (int sh = 0; sh < 24; sh += 8) r |= ((((a >> sh) & 255u) * (uint32_t)wa + ((b >> sh) & 255u) * (uint32_t)(4 - wa)) / 4u) << sh;
    return r;
}

int hk_gf_width(const char *s, int sc) { return hk_gf_width_h(s, 2 * sc); }
int hk_gf_text(uint32_t *buf, int bw, int x, int y, const char *s, int style, int sc) { return hk_gf_text_h(buf, bw, x, y, s, style, 2 * sc); }
int hk_gf_width_h(const char *s, int h) {
    if (h < 2) h = 2;
    if (!hk_art_ok()) {
        int w = 0;
        for (const uint8_t *p = (const uint8_t *)s; *p; p++) w += hk_fonts[0].glyphs[(*p < 0x20 ? '?' : *p) - 0x20].advance;
        return hs(w, h);
    }
    int w = 0;
    for (const uint8_t *p = (const uint8_t *)s; *p; p++) {
        int acc, sx, sy, gw = glyph_box(decompose(*p, &acc), &sx, &sy);
        w += gw ? gw + 1 : SPACE_ADV;
    }
    return hs(w > 0 ? w - 1 : 0, h);
}

int hk_gf_text_h(uint32_t *buf, int bw, int x, int y, const char *s, int style, int h) {
    const Style *st = &g_style[style];
    if (h < 2) h = 2;
    if (!hk_art_ok()) {   // fallback: the port's 1-bit font
        const HkFont *f = &hk_fonts[0];
        int x0 = x;
        for (const uint8_t *p = (const uint8_t *)s; *p; p++) {
            const HkGlyph *g = &f->glyphs[(*p < 0x20 ? '?' : *p) - 0x20];
            for (int gy = 0; gy < f->cell; gy++)
                for (int gx = 0; gx < g->width; gx++)
                    if ((f->rows[g->offset + gy] >> gx) & 1u)
                        for (int py = hs(gy, h); py < hs(gy + 1, h); py++) for (int px = hs(gx, h); px < hs(gx + 1, h); px++) put(buf, bw, x + px, y + py, st->fill);
            x += hs(g->advance, h);
        }
        return x - x0;
    }
    int mw, adv = lay_out(s, &mw);
    // Outline: any of the 8 neighbours is ink; shadow: the pixel above or above-left is ink or outline.
    static uint8_t kind[MH][MW];   // 0 none, 1 shadow, 2 outline, 3 ink
    for (int j = 0; j < MH; j++)
        for (int i = 0; i < mw; i++) {
            if (g_m[j][i]) { kind[j][i] = 3; continue; }
            int o = 0;
            for (int dj = -1; dj <= 1 && !o; dj++)
                for (int di = -1; di <= 1 && !o; di++) {
                    int jj = j + dj, ii = i + di;
                    o = jj >= 0 && jj < MH && ii >= 0 && ii < MW && g_m[jj][ii];
                }
            kind[j][i] = o ? 2 : 0;
        }
    for (int j = MH - 1; j > 0; j--)
        for (int i = 1; i < mw; i++)
            if (!kind[j][i] && (kind[j - 1][i] >= 2 || kind[j - 1][i - 1] >= 2)) kind[j][i] = 1;
    for (int j = 0; j < MH; j++) {
        int row = j - MY;   // cell row
        uint32_t ink = row <= 3 && style == GF_HEAD ? st->top : row <= 2 ? st->top : row >= 10 ? st->low : st->fill;
        for (int i = 0; i < mw; i++) {
            uint32_t c;
            switch (kind[j][i]) {
            case 3: c = g_m[j][i] == 3 ? ink : mix(ink, st->outline, g_m[j][i] == 2 ? 3 : 2); break;
            case 2: c = st->outline; break;
            case 1: c = st->shadow; break;
            default: continue;
            }
            for (int py = hs(row, h); py < hs(row + 1, h); py++)
                for (int px = hs(i - 2, h); px < hs(i - 1, h); px++) put(buf, bw, x + px, y + py, c);
        }
    }
    return hs(adv, h);
}

void hk_gf_upper(char *dst, const char *src, int cap) {
    int n = 0;
    for (const uint8_t *p = (const uint8_t *)src; *p && n < cap - 1; p++) {
        uint8_t c = *p;
        if (c >= 'a' && c <= 'z') c -= 32;
        else if (c >= 0xE0 && c != 0xF7 && c != 0xFF) c -= 32;
        dst[n++] = (char)c;
    }
    dst[n] = 0;
}
