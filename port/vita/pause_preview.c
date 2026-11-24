// PC preview of the Vita pause menu: gcc -I port -o pm_preview port/vita/pause_preview.c
// Usage: pm_preview <main|cheats|settings|controls|results|tips|msgbox> out.ppm [width 640|848] [background.ppm (P6) or -]
//        [capture action 0..7 (tips: page)];
// tips: reads the tip texts from the file $HK_TIPS_FILE (one per line, \n written as the two characters \ and n; made
// from the game's string resources 500 504 506 508 512 600 by a dev script, never committed) and fills %s with a button. HK_PM_UI=2|4 sets the menu text scale (1x, 2x); writes the 960x544 output
// The game's menu art is read from $HK_GAME_DIR/BITMAP/HDKSYS00.BMP (default build/hk-game, the copy of the game).
#include "hk/font_data.h"
#include "pause_menu.h"
#include "hk/gameart.c"
#include <stdlib.h>
int g_hk_lang;   // HK_LANG=pt in the environment shows the Portuguese texts
char g_game_dir[512];
int main(int argc, char **argv) {
    if (argc < 3) return 1;
    g_hk_lang = getenv("HK_LANG") != NULL;
    if (getenv("HK_PM_UI")) g_pm_ui = atoi(getenv("HK_PM_UI"));   // menu text scale in halves (2 1x, 4 2x)
    snprintf(g_game_dir, sizeof g_game_dir, "%s", getenv("HK_GAME_DIR") ? getenv("HK_GAME_DIR") : "build/hk-game");
    int bw = argc > 3 ? atoi(argv[3]) : 640;
    static uint32_t src[480 * 848], buf[480 * 848];
    for (int y = 0; y < 480; y++) for (int x = 0; x < bw; x++) src[y * bw + x] = ((60 + y / 4) << 16) | ((120 + x / 8) << 8) | 200;
    if (argc > 4 && strcmp(argv[4], "-")) {
        FILE *f = fopen(argv[4], "rb"); int w, h, m;
        if (f && fscanf(f, "P6 %d %d %d", &w, &h, &m) == 3) {
            fgetc(f);
            for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { int r = fgetc(f), g = fgetc(f), b = fgetc(f); if (x < bw && y < 480) src[y * bw + x] = r << 16 | g << 8 | b; }
        }
    }
    int map[PM_NBTN]; pm_map_defaults(map);
    PmState st = {.map = map, .capture = argc > 5 && strcmp(argv[1], "tips") ? atoi(argv[5]) : 0, .wide = 0, .fps = 1, .wide_native = 1, .started_native = 0, .vol = 6, .mus = 4, .cheats = 1, .inv_on = 1, .money = 2, .in_stage = 1, .lang = g_hk_lang, .lang_ok = 1, .started_lang = g_hk_lang};
    int scr = !strcmp(argv[1], "cheats") ? PM_CHEATS : !strcmp(argv[1], "settings") ? PM_SETTINGS : !strcmp(argv[1], "controls") ? PM_CONTROLS : PM_MAIN;
    static uint32_t ovl[PM_OW * PM_OH], out[PM_OW * PM_OH];
    int dim = 128;
    if (!strcmp(argv[1], "msgbox")) {   // MessageBoxA "Stop this game ?" (MB_OKCANCEL, 0x40F2A0; PT-BR HDKWINBR.EXE text)
        pm_overlay_msgbox(ovl, "Handkerchief", g_hk_lang ? "Deseja parar ?" : "Stop this game ?", "OK", hk_tr(S_CANCEL));   // caption 0x46E028
        dim = 192;
    } else if (!strcmp(argv[1], "tips")) {   // tips & controls card over the undimmed background
        static char tl[8][400];
        int nt = 0;
        FILE *tf = getenv("HK_TIPS_FILE") ? fopen(getenv("HK_TIPS_FILE"), "r") : NULL;
        char ln[400];
        while (tf && nt < 8 && fgets(ln, sizeof ln, tf)) {
            char *o = tl[nt], *p = ln;
            for (; *p && *p != '\n'; p++) { if (p[0] == '\\' && p[1] == 'n') { *o++ = '\n'; p++; } else *o++ = *p; }
            *o = 0; nt++;
        }
        PmTipsArg ta = {.map = map, .page = nt ? (argc > 5 ? atoi(argv[5]) % nt : 0) : 0, .npages = nt};
        static const int tact[8] = {ACT_BOMB, 0, 0, ACT_SWITCH, ACT_BOMB, 0, 0, 0};
        if (nt) pm_tip_fill(ta.tip, sizeof ta.tip, tl[ta.page], map, tact[ta.page]);
        st.capture = 0;
        pm_overlay_tips(ovl, &ta);
        dim = 0;
    } else if (!strcmp(argv[1], "results")) {   // RESULTS panel (held Select) over the undimmed background, sample counters
        static const uint32_t v[13] = {25 * 3723, 0, 58, 6, 4, 1, 2, 2, 0, 350, 3, 5, 1};
        pm_overlay_results(ovl, v);
        dim = 0;
    } else pm_overlay_screen(ovl, scr, scr == PM_SETTINGS ? 3 : 1, &st);
    // The 960x544 output: the frame letterboxed 4:3 (nearest here; the console filters it), the overlay 1:1 on top.
    pm_composite(out, src, bw, 480, bw, getenv("HK_STRETCH") != NULL, dim, ovl);
    FILE *o = fopen(argv[2], "wb");
    fprintf(o, "P6\n%d %d\n255\n", PM_OW, PM_OH);
    for (int i = 0; i < PM_OW * PM_OH; i++) { fputc(out[i] >> 16, o); fputc(out[i] >> 8, o); fputc(out[i], o); }
    fclose(o);
    return 0;
}
