// The game's own menu art, used by the port's menus (pause menu, RESULTS panel, message box, dialogs, menu bar) so
// they look like the title screen. Loaded at run time from the game's BITMAP/HDKSYS00.BMP (never copied into the
// port). Evidence (title screen, frame 1500, HK_TRACE_PIX blit log; the title menu is drawn by the caller 0x408C15 /
// 0x408D5C from surface #6 = HDKSYS00, colour key 0):
//   - menu box: 9-slice of the 64x64 sprite at (0,56): corners 16x16 at (0,56) (48,56) (0,104) (48,104), top and
//     bottom edges 32x16 at (16,56) / (16,104), left and right edges 16x32 at (0,72) / (48,72), tiled from the
//     corner, the last tile cut (title: box 200x104 at (220,342), interior not filled);
//   - selection mark: the 8x8 red dot at (0,0), 12 px left of the selected word and 4 px right of it, 2 px lower;
//   - menu words: 12 px tall bitmaps, unselected row y 256, selected row y 268 (NEW GAME at x 184, w 92);
//   - text font: HDKSYS00 also holds a proportional ASCII font, white with two grey edge shades (palette 255, 7,
//     248): '!'..'S' at y 1..16 from x 163, 'T'..'z' at y 17..32 from x 297 (glyph boxes in gameart.c).
// Text is drawn in the style of the title menu words: 1 px dark outline (palette 81, 0x4F524D), drop shadow
// (palette 31, 0x2D2A2F), cream fill when selected (palette 213, 0xFBFDDA; lower rows 208, 0xDCD6C6) and taupe when
// not (palette 161, 0x998979). Headings use the colours of the PAUSE sign at (568,424): fill 0x4C4DCA, top 0x628BA6,
// bottom 0x433A89, outline 0x191A19. If the bitmap is missing, the boxes fall back to flat frames and the text to
// the port's 1-bit font. Pure C (no SDL), so the PC preview tool (port/vita/pause_preview.c) uses it too.
#ifndef HK_GAMEART_H
#define HK_GAMEART_H
#include <stdint.h>

enum { GF_SEL, GF_NORMAL, GF_DIM, GF_HEAD, GF_GOLD };
// Canvas height of a buffer bw pixels wide: 544 rows for the 960-wide menu overlay (port/vita/pause_menu.h, drawn at the
// native 960x544 output), 480 for the game-frame buffers (<= 848 wide).
#define HK_CANVAS_H(bw) ((bw) >= 960 ? 544 : 480)
#define HK_GF_CELL 16   // text cell height at scale 1 (caps from row 2 to row 12, descenders to row 15)

int hk_art_ok(void);   // loads the bitmap on first use (g_game_dir/BITMAP/HDKSYS00.BMP); 0 if missing
// Draws a sprite of HDKSYS00 (index 0 transparent) into an XRGB buffer bw pixels wide, 480 rows, clipped.
void hk_art_blit(uint32_t *buf, int bw, int dx, int dy, int sx, int sy, int w, int h);
void hk_art_frame(uint32_t *buf, int bw, int x, int y, int w, int h);   // the title's menu box (w, h >= 32)
void hk_art_dot(uint32_t *buf, int bw, int x, int y);                   // the red selection dot, 8x8, top left at x, y
// Text in the game's font in the title-menu style; sc = 1 or 2 (pixel doubled). Latin-1; accented letters are
// composed from the base letter and a drawn mark. Returns the advance.
int hk_gf_text(uint32_t *buf, int bw, int x, int y, const char *s, int style, int sc);
int hk_gf_width(const char *s, int sc);
// The same with the scale in halves (h = 2 is 1x, 3 is 1.5x, 4 is 2x; nearest neighbour, so at odd h every other
// source pixel is two pixels wide). The pause menus pick the largest scale that fits (port/vita/pause_menu.h).
int hk_gf_text_h(uint32_t *buf, int bw, int x, int y, const char *s, int style, int h);
int hk_gf_width_h(const char *s, int h);
void hk_art_dot_h(uint32_t *buf, int bw, int x, int y, int h);   // the red dot scaled by h / 2
void hk_gf_upper(char *dst, const char *src, int cap);   // Latin-1 upper case
#endif
