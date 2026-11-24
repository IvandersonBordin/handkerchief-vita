// Colours of the port's UI theme (pause menu, dialogs, menu bar, message box), taken from the game's title menu art
// in HDKSYS00.BMP (hk/gameart.h): the menu box's purples (palette 118 0x6C5BC5, 89 0x5947A5, 67 0x433A89, 32
// 0x2C1E6D), the menu words' cream (213 0xFBFDDA, 208 0xDCD6C6) and taupe (161 0x998979), the red dot (109 0xF6261C).
// 0xRRGGBB.
#ifndef HK_THEME_H
#define HK_THEME_H
#define HK_TH_BORDER 0x6c5bc5u      // frame (light purple of the menu box)
#define HK_TH_BORDER_IN 0x433a89u   // inner frame, separators
#define HK_TH_FILL 0x0c0a16u        // panel (blended over the game frame)
#define HK_TH_TEXT 0xdcd6c6u        // body text (cream, lower rows of the selected menu word)
#define HK_TH_SEL 0xfbfddau         // selected / focused (cream of the selected menu word)
#define HK_TH_DIM 0x7d7466u         // disabled
#define HK_TH_HEAD 0x9a8cf0u        // group titles (lighter purple, readable in the 1-bit font)
#define HK_TH_RED 0xf6261cu         // selection dots (when drawn as shapes)
#define HK_TH_BTN 0x1c1838u         // opaque dark control (push button, edit, list)
#define HK_TH_BAR 0x3a2f7au         // opaque selection bar / pressed slot
#define HK_TH_INK 0x0a0814u         // field background

#include <stdint.h>
// PlayStation button symbol (kind 1 cross, 2 circle, 3 triangle, 4 square) in a 16x16 cell (pixel-replicated sc times) at x,y of a buffer bw
// pixels wide, bh rows, in the colours of the real buttons.
static inline void hk_th_glyph_s(uint32_t *buf, int bw, int bh, int x, int y, int kind, int sc) {
    static const uint32_t col[5] = {0, 0x70b0ffu, 0xff6a6au, 0x60e0a0u, 0xf090d0u};
    for (int j = 0; j < 16; j++)
        for (int i = 0; i < 16; i++) {
            int dx = i - 8, dy = j - 8, ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy, on = 0;
            if (kind == 1) on = (dx - dy >= -1 && dx - dy <= 1 ? 1 : dx + dy >= -1 && dx + dy <= 1) && ax <= 6 && ay <= 6;
            else if (kind == 2) on = dx * dx + dy * dy >= 30 && dx * dx + dy * dy <= 56;
            else if (kind == 4) { int m = ax > ay ? ax : ay; on = m >= 5 && m <= 6; }
            else if (kind == 3) { int r = j - 1, hw = r * 7 / 13; on = r >= 0 && r <= 13 && ax <= hw && (ax >= hw - 1 || r >= 12); }
            if (!on) continue;
            for (int v = 0; v < sc; v++)
                for (int u = 0; u < sc; u++) {
                    int px = x + i * sc + u, py = y + j * sc + v;
                    if (px >= 0 && px < bw && py >= 0 && py < bh) buf[py * bw + px] = col[kind];
                }
        }
}
// The same at 1x on a 480-row buffer (dialogs).
static inline void hk_th_glyph(uint32_t *buf, int bw, int x, int y, int kind) { hk_th_glyph_s(buf, bw, 480, x, y, kind, 1); }
#endif
