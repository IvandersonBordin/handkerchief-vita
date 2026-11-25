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

// ---------- title menu words ----------
// The title menu words live in HDKSYS00.BMP (8-bit indices, bottom-up rows, 640x448) as 12 px tall cells at y 256 (normal) and
// y 268 (selected) with the rects NEW GAME x 184 w 92, CONTINUE x 276 w 88, NETWORK x 364 w 84, EXIT x 448 w 40 (identical in the
// PT-BR executable). They are rebuilt from the ORIGINAL letters: at run time every letter N E W G A M C O T I U R X of the four
// original words is cut out of the port's own copy of the original art (g_idx, read from the game directory) as a palette-index
// bitmap (outline, shadow and shading included, both states), and words are composed from these with the original letter gaps
// (empty ink columns between neighbours: rb(left)+lb(right) below, they reproduce all 15 pairs measured in the originals; a word
// space is 7 columns). No art is stored in the port. The letters the originals lack (P S J V) are built from the cut ones (P from
// R, S from C, J from U; V from a stroke recipe with the same 4 px strokes and cap height) and shaded by the originals' rules:
// index 0 key; 31 drop shadow on the empty pixel right below ink; 81 outline on every other empty pixel touching ink (8
// neighbours); normal row: ink is 181 on the top row of a stroke and 161 below it; selected row: ink is 208 on the bottom row of
// a stroke and 213 above it. Row 0 of the cell is outline only, ink spans rows 1..10 and the shadow row 11. (The originals are
// hand drawn and deviate from these rules in a few pixels, hence the cut letters are used as they are.) Too long words first
// lose word-space width, then letter gap, then interior columns of the letters (only where no stroke gets thinner than 4 px
// and no gap closes; such a letter is re-shaded by the rules); the word is centred in its cell.
#define TW_MAX 96
#define GL_BOX 18
typedef struct {
    uint16_t r[12];   // bit i of r[y]: ink at column i, row y of the cell
    int w;            // ink width
    uint8_t px[2][12][GL_BOX];   // [state][row][1 + column]: the letter with its 1 px margin, palette indices
    uint8_t alt[2][12][GL_BOX];  // the hand drawn originals differ from the first copy in a pixel or two in the 2nd N and the final E
    int has_alt;
} Gl;
static Gl g_gl[26];
static int g_gl_state;   // 0 not tried, 1 all letters ready, -1 failed
static uint16_t gl_row(const char *s) { uint16_t v = 0; for (int i = 0; s[i]; i++) if (s[i] == '#') v |= (uint16_t)(1u << i); return v; }
static int ink_val(uint8_t v) { return v != 0 && v != 81 && v != 31; }   // 31 is the drop shadow, not ink
static int ink_at(int x, int y) { return ink_val(g_idx[(256 + y) * SYS_W + x]); }
// Shades the ink mask of one letter by the rules above into g->px[sel] (margin included).
static void gl_shade(Gl *g, int sel) {
    static const uint8_t pn[4] = {81, 161, 181, 31}, ps[4] = {81, 213, 208, 31};
    const uint8_t *p = sel ? ps : pn;
    memset(g->px[sel], 0, sizeof g->px[sel]);
    for (int y = 0; y < 12; y++)
        for (int x = -1; x <= g->w; x++) {
#define INK(yy, xx) ((yy) >= 0 && (yy) < 12 && (xx) >= 0 && (xx) < g->w && (g->r[yy] >> (xx) & 1))
            uint8_t v = 0;
            if (INK(y, x)) v = sel ? (!INK(y + 1, x) ? p[2] : p[1]) : (!INK(y - 1, x) ? p[2] : p[1]);
            else if (INK(y - 1, x)) v = p[3];
            else
                for (int dj = -1; dj <= 1 && !v; dj++)
                    for (int di = -1; di <= 1 && !v; di++)
                        if (INK(y + dj, x + di)) v = p[0];
#undef INK
            g->px[sel][y][x + 1] = v;
        }
}
// Cuts the letters of one original word into g_gl; names has one char per letter in the word ('-' skips one, a letter already cut too).
static int cut_word(int x, int w, const char *names) {
    int a[16], b[16], n = 0;
    for (int i = 0; i < w;) {
        int occ = 0;
        for (int j = 0; j < 12; j++) occ |= ink_at(x + i, j);
        if (!occ) { i++; continue; }
        if (n >= 16) return 0;
        a[n] = i;
        while (i < w) { int o = 0; for (int j = 0; j < 12; j++) o |= ink_at(x + i, j); if (!o) break; i++; }
        b[n++] = i;
    }
    if (n != (int)strlen(names)) return 0;
    int seen[26] = {0};
    for (int k = 0; k < n; k++) {
        char c = names[k];
        if (c < 'A' || c > 'Z' || b[k] - a[k] > GL_BOX - 3) continue;
        Gl *g = &g_gl[c - 'A'];
        int occ = seen[c - 'A']++;
        uint8_t(*dst)[12][GL_BOX] = g->px;
        if (g->w) {   // seen before (another word) or a repeat in this word: keep the later copy as the variant
            if (occ == 0 || g->has_alt) continue;
            dst = g->alt;
            g->has_alt = 1;
        } else {
            g->w = b[k] - a[k];
            for (int j = 0; j < 12; j++)
                for (int i = a[k]; i < b[k]; i++) if (ink_at(x + i, j)) g->r[j] |= (uint16_t)(1u << (i - a[k]));
        }
        for (int sel = 0; sel < 2; sel++)
            for (int j = 0; j < 12; j++)
                for (int i = a[k] - 1; i <= b[k]; i++) {   // own pixels only: ink, or shadow / outline touching the own ink
                    if (i < 0 || i >= w) continue;
                    uint8_t v = g_idx[(256 + sel * 12 + j) * SYS_W + x + i];
                    int own = 0;
                    for (int dj = -1; dj <= 1 && !own; dj++)
                        for (int di = -1; di <= 1 && !own; di++) {
                            int jj = j + dj, ii = i + di;
                            own = jj >= 0 && jj < 12 && ii >= a[k] && ii < b[k] && ink_at(x + ii, jj);
                        }
                    if (v && own) dst[sel][j][i - a[k] + 1] = v;
                }
    }
    return 1;
}
static int glyphs(void) {
    if (g_gl_state) return g_gl_state > 0;
    g_gl_state = -1;
    if (!hk_art_ok()) return 0;
    memset(g_gl, 0, sizeof g_gl);
    if (!cut_word(184, 92, "NEWGAME") || !cut_word(276, 88, "CONTINUE") || !cut_word(364, 84, "NETWOR-") || !cut_word(448, 40, "EXIT")) return 0;
    for (const char *p = "NEWGAMCOTIURX"; *p; p++) if (!g_gl[*p - 'A'].w) return 0;
    // P: the bowl of R (rows 1..6) on a plain stem.
    Gl *R = &g_gl['R' - 'A'], *P = &g_gl['P' - 'A'];
    for (int j = 0; j < 12; j++) P->r[j] = j <= 6 ? R->r[j] : (uint16_t)(R->r[j] & 0xF);
    P->w = 9;
    // S: top (rows 1..4) and bottom (rows 8..10) of C, joined by a sweep on rows 5..7.
    Gl *C = &g_gl['C' - 'A'], *S = &g_gl['S' - 'A'];
    for (int j = 0; j < 12; j++) S->r[j] = j <= 4 || j >= 8 ? C->r[j] : gl_row(j == 5 ? "#######.." : j == 6 ? ".#######." : "..#######");
    S->w = C->w;
    // J: U without its upper left stem (a hook of rows 6..10 stays).
    Gl *U = &g_gl['U' - 'A'], *J = &g_gl['J' - 'A'];
    for (int j = 0; j < 12; j++) J->r[j] = j >= 1 && j < 6 ? (uint16_t)(U->r[j] & ~0xFu) : U->r[j];
    J->w = U->w;
    // V: two 4 px diagonal strokes that meet in a 6 px tip on the baseline (the tip of the original A).
    Gl *V = &g_gl['V' - 'A'];
    for (int j = 1; j <= 10; j++) {
        int x0 = ((j - 1) * 3 + 4) / 9;   // 1/3 px per row, rounded
        for (int i = 0; i < 4; i++) V->r[j] |= (uint16_t)((1u << (x0 + i)) | (1u << (11 - x0 - i)));
    }
    V->w = 12;
    for (const char *p = "PSJV"; *p; p++) for (int sel = 0; sel < 2; sel++) gl_shade(&g_gl[*p - 'A'], sel);
    g_gl_state = 1;
    return 1;
}
//                       A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
static const int8_t g_lb[26] = {0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0};
static const int8_t g_rb[26] = {1, 0, 2, 0, 2, 0, 1, 0, 2, 1, 0, 0, 1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0};
static int run_len(uint16_t r, int w, int c) { int a = c, b = c; while (a > 0 && (r >> (a - 1) & 1)) a--; while (b < w - 1 && (r >> (b + 1) & 1)) b++; return b - a + 1; }
// A column can go when no ink run of a stroke drops below 4 px and no interior gap closes.
static int col_removable(const Gl *g, int c) {
    for (int y = 0; y < 12; y++) {
        uint16_t r = g->r[y];
        if (r >> c & 1) { if (run_len(r, g->w, c) < 5) return 0; continue; }
        if (!(r & ((1u << c) - 1)) || !(r >> (c + 1))) continue;   // exterior: nothing to close
        int a = c, b = c;
        while (a > 0 && !(r >> (a - 1) & 1)) a--;
        while (b < g->w - 1 && !(r >> (b + 1) & 1)) b++;
        if (b - a + 1 < 2) return 0;
    }
    return 1;
}
static void col_remove(Gl *g, int c) {
    for (int y = 0; y < 12; y++) g->r[y] = (uint16_t)((g->r[y] & ((1u << c) - 1)) | (g->r[y] >> (c + 1) << c));
    g->w--;
    for (int sel = 0; sel < 2; sel++) gl_shade(g, sel);
}
static int tw_layout(const Gl *g, const char *l, const uint8_t *sp, int n, int space, int adj, int *pos) {
    int x = 0;
    for (int i = 0; i < n; i++) {
        if (i) {
            int gap = sp[i] ? space : g_rb[l[i - 1] - 'A'] + g_lb[l[i] - 'A'] + adj;
            x += gap < 1 ? 1 : gap;
        }
        pos[i] = x;
        x += g[i].w;
    }
    return x;
}
static int px_rank(uint8_t v) { return v == 0 ? 0 : v == 81 ? 1 : v == 31 ? 2 : 3; }   // outline < shadow < ink where letters overlap
// Renders a word (letters and spaces, upper case) into out[12 * cw] palette indices (top-down); returns 0 if not possible.
int hk_art_title_word_render(const char *s, int cw, int sel, uint8_t *out) {
    Gl g[16] = {{{0}}};
    char l[16] = {0};
    uint8_t sp[16];
    int n = 0, pend = 0, pos[16], seen[26] = {0};
    if (cw > TW_MAX || !glyphs()) return 0;
    for (; *s; s++) {
        if (*s == ' ') { pend = 1; continue; }
        if (*s < 'A' || *s > 'Z' || n >= 16 || !g_gl[*s - 'A'].w) return 0;
        l[n] = *s; g[n] = g_gl[*s - 'A']; sp[n++] = (uint8_t)pend; pend = 0;
        if (g[n - 1].has_alt && (++seen[*s - 'A'] > 1 || !s[1])) memcpy(g[n - 1].px, g[n - 1].alt, sizeof g[n - 1].px);
    }
    int space = 7, adj = 0, total = tw_layout(g, l, sp, n, space, adj, pos);
    while (total > cw - 2 && space > 5) total = tw_layout(g, l, sp, n, --space, adj, pos);
    while (total > cw - 2 && adj > -3) total = tw_layout(g, l, sp, n, space, --adj, pos);
    while (total > cw - 2) {   // condense: the widest letter that still has a removable column, the one nearest to its middle
        int bi = -1, bc = 0;
        for (int i = 0; i < n; i++) {
            int c = -1;
            for (int k = 1; k < g[i].w - 1; k++)
                if (col_removable(&g[i], k) && (c < 0 || abs(2 * k - g[i].w + 1) < abs(2 * c - g[i].w + 1))) c = k;
            if (c >= 0 && (bi < 0 || g[i].w > g[bi].w)) { bi = i; bc = c; }
        }
        if (bi < 0) return 0;
        col_remove(&g[bi], bc);
        total = tw_layout(g, l, sp, n, space, adj, pos);
    }
    memset(out, 0, (size_t)12 * (size_t)cw);
    int left = (cw - total) / 2;
    for (int i = 0; i < n; i++)
        for (int y = 0; y < 12; y++)
            for (int x = 0; x < g[i].w + 2; x++) {
                int ox = left + pos[i] + x - 1;
                uint8_t v = g[i].px[sel][y][x];
                if (ox >= 0 && ox < cw && px_rank(v) > px_rank(out[y * cw + ox])) out[y * cw + ox] = v;
            }
    return 1;
}
// Rewrites the title words of a loaded HDKSYS00 bitmap. lang 0: only NETWORK becomes OPTIONS (the Vita has no network mode);
// lang 1: all four words in Portuguese.
void hk_art_title_words(uint8_t *bits, int stride, int h, int lang) {
    static const struct { int x, w; const char *en, *pt; } W[4] = {
        {184, 92, NULL, "NOVO JOGO"}, {276, 88, NULL, "CONTINUAR"}, {364, 84, "OPTIONS", "AJUSTES"}, {448, 40, NULL, "SAIR"}};
    if (!hk_art_ok() || h != SYS_H || stride < SYS_W) return;
    for (int wi = 0; wi < 4; wi++) {
        const char *s = lang ? W[wi].pt : W[wi].en;
        if (!s) continue;
        for (int sel = 0; sel < 2; sel++) {
            uint8_t row[TW_MAX * 12];
            if (!hk_art_title_word_render(s, W[wi].w, sel, row)) continue;
            for (int y = 0; y < 12; y++)
                memcpy(bits + (size_t)(h - 1 - (256 + sel * 12 + y)) * (size_t)stride + W[wi].x, row + y * W[wi].w, (size_t)W[wi].w);
        }
    }
}

// ---------- general text in the title-word style ----------
// hk_tw_text_h draws any string (A-Z, 0-9, a few punctuation marks, the Portuguese/Latin-1 accented capitals) with the
// glyphs of the title menu words. The 13 letters cut from the original art (and P S J V derived above) are used as they
// are; the other letters, the digits and the punctuation are hand-built masks with the same 4 px strokes, cap height
// (rows 1..10 of the 12 px cell) and gaps, and the whole string is shaded at once by the title's rules (see above:
// outline 81, drop shadow 31 below ink, normal ink 181 on the top row of a stroke and 161 below, selected ink 208 on the
// bottom row and 213 above). Accents are 2 px marks on rows -2..-1 (cedilla rows 11..12). Strings with a character that
// cannot be built return -1 and the caller falls back to the plain font. Text is upper case (lower case is folded).
#define TG_ROWS 17   // canvas rows -3..13, index = row + 3
typedef struct { int w; int8_t lb, rb; uint16_t r[TG_ROWS]; } Tg;
static Tg g_tg[256];
static int g_tg_state;
static void tg_put(int c, int lb, int rb, const char *const *rows, int n, int row0) {   // rows: n strings starting at cell row row0
    Tg *g = &g_tg[c];
    memset(g, 0, sizeof *g);
    g->w = (int)strlen(rows[0]); g->lb = (int8_t)lb; g->rb = (int8_t)rb;
    for (int j = 0; j < n; j++) g->r[row0 + j + 3] = gl_row(rows[j]);
}
#define TG(c, lb, rb, row0, ...) do { static const char *const R_[] = {__VA_ARGS__}; tg_put(c, lb, rb, R_, (int)(sizeof R_ / sizeof *R_), row0); } while (0)
static int tg_init(void) {
    if (g_tg_state) return g_tg_state > 0;
    g_tg_state = -1;
    if (!glyphs()) return 0;
    memset(g_tg, 0, sizeof g_tg);
    for (int c = 0; c < 26; c++) {
        const Gl *g = &g_gl[c];
        if (!g->w) continue;
        Tg *t = &g_tg['A' + c];
        t->w = g->w; t->lb = g_lb[c]; t->rb = g_rb[c];
        for (int y = 0; y < 12; y++) t->r[y + 3] = g->r[y];
    }
    TG('B', 0, 1, 1, "########.", "#########", "####.####", "####.####", "########.", "########.", "####.####", "####.####", "#########", "########.");
    TG('D', 0, 2, 1, "#######..", "########.", "####.####", "####..###", "####..###", "####..###", "####..###", "####.####", "########.", "#######..");
    TG('F', 0, 1, 1, "########", "########", "####....", "####....", "######..", "######..", "####....", "####....", "####....", "####....");
    TG('H', 0, 1, 1, "####..####", "####..####", "####..####", "####..####", "##########", "##########", "####..####", "####..####", "####..####", "####..####");
    TG('K', 0, 1, 1, "####..####", "####.####.", "#########.", "########..", "#######...", "#######...", "########..", "####.####.", "####..####", "####..####");
    TG('L', 0, 2, 1, "####....", "####....", "####....", "####....", "####....", "####....", "####....", "####....", "########", "########");
    TG('Q', 0, 2, 1, "..######..", ".########.", "##########", "####..####", "####..####", "####..####", "####.#####", "##########", ".#########", "..######.#");
    TG('Y', 0, 1, 1, "####..####", "####..####", ".########.", ".########.", "..######..", "...####...", "...####...", "...####...", "...####...", "...####...");
    TG('Z', 0, 1, 1, "##########", "##########", "......####", ".....####.", "....####..", "...####...", "..####....", ".####.....", "##########", "##########");
    TG('0', 0, 1, 1, "..#####..", ".#######.", "####.####", "####.####", "####.####", "####.####", "####.####", "####.####", ".#######.", "..#####..");
    TG('1', 0, 1, 1, "..####.", ".#####.", "#######", "...####", "...####", "...####", "...####", "...####", "...####", "...####");
    TG('2', 0, 1, 1, "..#####..", ".#######.", "####.####", ".....####", "....####.", "...####..", "..####...", ".####....", "#########", "#########");
    TG('3', 0, 1, 1, ".#######.", "#########", "####.####", ".....####", "..######.", "..######.", ".....####", "####.####", "#########", ".#######.");
    TG('4', 0, 1, 1, ".....####.", "....#####.", "...######.", "..####.##.", ".####..##.", "##########", "##########", ".....####.", ".....####.", ".....####.");
    TG('5', 0, 1, 1, "#########", "#########", "####.....", "########.", "#########", "####.####", ".....####", "####.####", "#########", ".#######.");
    TG('6', 0, 1, 1, "..#####..", ".#######.", "####.####", "####.....", "########.", "#########", "####.####", "####.####", ".#######.", "..#####..");
    TG('7', 0, 1, 1, "#########", "#########", ".....####", "....####.", "...####..", "...####..", "..####...", "..####...", "..####...", "..####...");
    TG('8', 0, 1, 1, "..#####..", ".#######.", "####.####", "####.####", ".#######.", ".#######.", "####.####", "####.####", ".#######.", "..#####..");
    TG('9', 0, 1, 1, "..#####..", ".#######.", "####.####", "####.####", ".########", "..#######", ".....####", "####.####", ".#######.", "..#####..");
    TG('.', 1, 1, 8, "####", "####", "####");
    TG(',', 1, 1, 7, "####", "####", "###.", "##..");
    TG(':', 1, 1, 2, "####", "####", "####", "....", "....", "....", "####", "####", "####");
    TG(';', 1, 1, 2, "####", "####", "####", "....", "....", "....", "####", "###.", "##..");
    TG('!', 1, 1, 1, "####", "####", "####", "####", "####", "####", "....", "####", "####", "####");
    TG('?', 0, 1, 1, ".#######.", "#########", "####.####", ".....####", "....####.", "...####..", ".........", "...####..", "...####..", "...####..");
    TG('-', 1, 1, 5, "#######", "#######");
    TG('+', 1, 1, 2, "..####..", "..####..", "..####..", "########", "########", "..####..", "..####..", "..####..");
    TG('=', 1, 1, 3, "########", "########", "........", "........", "########", "########");
    TG('(', 1, 0, 1, "..###", ".###.", "###..", "###..", "###..", "###..", "###..", "###..", ".###.", "..###");
    TG(')', 0, 1, 1, "###..", ".###.", "..###", "..###", "..###", "..###", "..###", "..###", ".###.", "###..");
    TG('\'', 1, 1, 1, "####", "####", "####", "####");
    TG('"', 1, 1, 1, "####.####", "####.####", "####.####", "####.####");
    for (int j = 1; j <= 10; j++) {   // '/': a 4 px stroke from the top right to the bottom left
        Tg *g = &g_tg['/'];
        int x0 = (6 * (10 - j) + 4) / 9;
        g->w = 10; g->lb = 0; g->rb = 1;
        g->r[j + 3] = (uint16_t)(0xFu << x0);
    }
    {   // '$': S with a tick above and below
        Tg *g = &g_tg['$'];
        *g = g_tg['S']; g->r[0 + 3] |= 0x38; g->r[11 + 3] |= 0x38;
    }
    g_tg_state = 1;
    return 1;
}
// Accent marks (two rows) as hk_gf's: {top row, bottom row}.
static const char *const g_tg_mark[7][2] = {{"", ""}, {"##..", ".##."}, {"..##", ".##."}, {".##.", "#..#"}, {".##.#", "#.##."}, {"##.##", "##.##"}, {".#.", "##."}};
#define TW_CAN 1200
static uint8_t g_tw_m[TG_ROWS][TW_CAN + 4];
// Lays the string out into g_tw_m (column 0 = first ink column); returns the width or -1.
static int g_tw_hi;   // columns [0, g_tw_hi) of g_tw_m may hold ink: only these are cleared on the next build
static char g_tw_last[256];   // the string g_tw_m holds (a width query is usually followed by the draw of the same text)
static int g_tw_last_w = -2;
static int tw_build_raw(const char *str);
static int tw_build(const char *str) {
    if (g_tw_last_w != -2 && !strcmp(str, g_tw_last)) return g_tw_last_w;
    const int w = tw_build_raw(str);
    if (strlen(str) < sizeof g_tw_last) { strcpy(g_tw_last, str); g_tw_last_w = w; } else g_tw_last_w = -2;
    return w;
}
static int tw_build_raw(const char *str) {
    if (!tg_init()) return -1;
    if (g_tw_hi > 0) {
        const int n = g_tw_hi < TW_CAN + 4 ? g_tw_hi : TW_CAN + 4;
        for (int j = 0; j < TG_ROWS; j++) memset(g_tw_m[j], 0, (size_t)n);
    }
    g_tw_hi = 0;
    int x = 0, prev = -1, pend = 0;
    for (const uint8_t *p = (const uint8_t *)str; *p; p++) {
        uint8_t c = *p;
        if (c >= 'a' && c <= 'z') c -= 32;
        else if (c >= 0xE0 && c != 0xF7 && c != 0xFF) c -= 32;
        if (c == ' ') { if (pend) x += 3; else if (prev < 0) x += 7; pend = 1; continue; }   // extra spaces widen the gap
        int acc = AC_NONE;
        if (c >= 0xC0) {
            if (c == 0xD7 || c == 0xDF) return -1;
            c = decompose(c, &acc);
            if (c >= 'a' && c <= 'z') c -= 32;
        }
        const Tg *g = &g_tg[c];
        if (!g->w) return -1;
        if (prev >= 0) { int gap = pend ? 7 : g_tg[prev].rb + g->lb; x += gap < 1 ? 1 : gap; }
        pend = 0;
        if (x + g->w + 8 >= TW_CAN) return -1;
        g_tw_hi = x + g->w + 4;
        for (int j = 0; j < TG_ROWS; j++)
            for (int i = 0; i < g->w; i++) if (g->r[j] >> i & 1) g_tw_m[j][x + i + 1] = 1;
        if (acc != AC_NONE) {
            const char *const *mk = g_tg_mark[acc];
            int pw = (int)strlen(mk[0]), mx = x + (g->w - pw) / 2;
            int top = acc == AC_CED ? 11 : -2;
            for (int r = 0; r < 2; r++)
                for (int i = 0; i < pw; i++) if (mk[r][i] == '#' && mx + i >= 0) g_tw_m[top + r + 3][mx + i + 1] = 1;
        }
        prev = c;
        x += g->w;
    }
    if (pend && prev >= 0) x += 7;   // trailing space
    return x;
}
int hk_tw_width_h(const char *s, int h) {
    int w = tw_build(s);
    return w < 0 ? -1 : hs(w, h < 2 ? 2 : h);
}
// Draws s with its first ink column at x and its cap rows starting one cell row (1 / 2 of h px) below y, like the plain
// font's caps. style: GF_SEL / GF_NORMAL use the title's own shading and palette; the others the three-tone Style.
int hk_tw_text_h(uint32_t *buf, int bw, int x, int y, const char *s, int style, int h) {
    int w = tw_build(s);
    if (w < 0) return -1;
    if (h < 2) h = 2;
    const Style *st = &g_style[style];
    uint32_t ink_t, ink_f, ink_b, outl, shad;
    if (style == GF_SEL) { ink_t = ink_f = g_pal[213]; ink_b = g_pal[208]; outl = g_pal[81]; shad = g_pal[31]; }
    else if (style == GF_NORMAL) { ink_t = g_pal[181]; ink_f = ink_b = g_pal[161]; outl = g_pal[81]; shad = g_pal[31]; }
    else { ink_t = st->top; ink_f = st->fill; ink_b = st->low; outl = st->outline; shad = st->shadow; }
#define M_(j, i) ((j) >= 0 && (j) < TG_ROWS && (i) >= 0 && (i) < TW_CAN + 4 && g_tw_m[j][i])
    for (int j = 0; j < TG_ROWS; j++)
        for (int i = 0; i <= w + 1; i++) {   // i = ink column + 1 (column 0 holds the left outline)
            uint32_t c;
            if (M_(j, i)) c = style == GF_SEL ? (M_(j + 1, i) ? ink_t : ink_b) : style == GF_NORMAL ? (M_(j - 1, i) ? ink_f : ink_t)
                                                : !M_(j - 1, i) ? ink_t : !M_(j + 1, i) ? ink_b : ink_f;
            else if (M_(j - 1, i)) c = shad;
            else if (M_(j - 1, i - 1) || M_(j - 1, i + 1) || M_(j, i - 1) || M_(j, i + 1) || M_(j + 1, i - 1) || M_(j + 1, i) || M_(j + 1, i + 1)) c = outl;
            else continue;
            const int row = j - 2;   // cell row: cap row 1 (index 4) lands on row 2, as the plain font's
            for (int py = hs(row, h); py < hs(row + 1, h); py++)
                for (int px = hs(i - 1, h); px < hs(i, h); px++) put(buf, bw, x + px, y + py, c);
        }
#undef M_
    return hs(w, h);
}
