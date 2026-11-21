// PC entry point for the recompiled Handkerchief: SDL2 window, game exe loading, recompiled code start.
// Structure derived from sh3vita port/src/pc/main.c (GPL-3.0).
#include "rt.h"
#include "plat.h"
#include "cp1252.h"
#include <stdlib.h>

static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture *g_tex;

SDL_Window *plat_sdl_window(void) { return g_win; }
int plat_width(void) { return 640; }
int plat_height(void) { return 480; }
void plat_show_window(void) { SDL_ShowWindow(g_win); }

int plat_message_box(const char *title, const char *text, unsigned type) {
    if ((type & 15u) || getenv("HK_HEADLESS")) return 0;   // only MB_OK is answered; no fabricated confirmation
    unsigned icon = (type & 0xf0u) == 0x10u ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_INFORMATION;
    char title_utf8[768], text_utf8[6144];
    rt_cp1252_utf8(title_utf8, sizeof title_utf8, title ? title : "");
    rt_cp1252_utf8(text_utf8, sizeof text_utf8, text ? text : "");
    return SDL_ShowSimpleMessageBox(icon, title_utf8, text_utf8, g_win) == 0 ? 1 : 0;
}

int plat_pump_events(void) {
    SDL_Event e;
    int quit = 0;
    while (SDL_PollEvent(&e))
        if (e.type == SDL_QUIT) quit = 1;
    return quit;
}

void plat_present_xrgb(const uint32_t *pixels, int width, int height, int pitch_bytes) {
    if (!g_ren) return;
    if (!g_tex) g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_UpdateTexture(g_tex, NULL, pixels, pitch_bytes);
    SDL_RenderClear(g_ren);
    SDL_RenderCopy(g_ren, g_tex, NULL, NULL);
    SDL_RenderPresent(g_ren);
}

void rt_platform_fatal(const char *msg) {
    if (getenv("HK_HEADLESS")) return;
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Handkerchief (recomp)", msg, g_win);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : getenv("HK_GAME_DIR") ? getenv("HK_GAME_DIR") : "original-data";
    SDL_strlcpy(g_game_dir, dir, sizeof g_game_dir);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (!getenv("HK_HEADLESS")) {
        g_win = SDL_CreateWindow("Handkerchief (recomp)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720,
                                 SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
        g_ren = g_win ? SDL_CreateRenderer(g_win, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
        if (g_ren) SDL_RenderSetLogicalSize(g_ren, 640, 480);
    }
    if (rt_init(64u << 20)) rt_fatal("guest memory: allocation failed");
    char exe[600];
    SDL_snprintf(exe, sizeof exe, "%s/HDKWIN.EXE", g_game_dir);
    int r = rt_load_exe(exe);
    if (r) rt_fatal("cannot load %s (%d)", exe, r);
    rt_fill_iat();
    rt_log("game loaded from %s\n", g_game_dir);
    rt_run_entry();
    SDL_Quit();
    return 0;
}
