// PS Vita entry point for the recompiled Handkerchief: SDL2 window 960x544 (game 640x480 letterboxed),
// game files in ux0:data/handkerchief/ (the player's own copy, including HDKWIN.EXE), log in the same folder.
// Structure follows port/pc/main.c; startup settings follow the vita-port skill (assets/vita_runtime.c).
#include "rt.h"
#include "plat.h"
#include <psp2/ctrl.h>
#include <psp2/io/stat.h>
#include <psp2/power.h>
#include <stdlib.h>

#define HK_DATA_DIR "ux0:data/handkerchief"

// newlib heap: guest memory (HK_GUEST_MB) plus host allocations. Budget against 365 MB (ATTRIBUTE2=12).
#define HK_GUEST_MB 128
unsigned int _newlib_heap_size_user = (HK_GUEST_MB + 64) * 1024 * 1024;
unsigned int sceLibcHeapSize = 4 * 1024 * 1024;
const unsigned int sceUserMainThreadStackSize = 8 * 1024 * 1024;

static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture *g_tex;

SDL_Window *plat_sdl_window(void) { return g_win; }
int plat_width(void) { return 640; }
int plat_height(void) { return 480; }
void plat_show_window(void) {}

int plat_message_box(const char *title, const char *text, unsigned type) {
    rt_log("[msgbox] %s: %s (type %u)\n", title ? title : "", text ? text : "", type);
    return 0;   // no implicit answer
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

// Pad mapping, from key tests in stage A0 on 2026-11-20 (docs/status.md): X shoots, A or Up jumps, S crouches.
// Menus confirm with Enter or space and go back with Z/X/C (0x4117D4), and the game checks confirm first, so
// Cross sends X + space: it shoots in play and confirms in menus. Circle = A (jump), down = arrow + S (crouch),
// Square = Z, Triangle = C, L = Shift, R = Ctrl, Start = Enter, Select = Esc.
int plat_vk_down(int vk) {
    static SceCtrlData pad;
    static uint32_t last_frame = 0xffffffffu;
    extern unsigned g_frame;
    if (last_frame != g_frame) {   // one read per presented frame is enough for GetKeyState polling
        static int init;
        if (!init) { sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG); init = 1; }
        sceCtrlPeekBufferPositive(0, &pad, 1);
        last_frame = g_frame;
    }
    uint32_t b = pad.buttons;
    int down = (b & SCE_CTRL_DOWN) || pad.ly > 192;
    switch (vk) {
    case 0x25: return (b & SCE_CTRL_LEFT) || pad.lx < 64;
    case 0x26: return (b & SCE_CTRL_UP) || pad.ly < 64;
    case 0x27: return (b & SCE_CTRL_RIGHT) || pad.lx > 192;
    case 0x28: return down;
    case 'S': return down;
    case 'X': return (b & SCE_CTRL_CROSS) != 0;
    case 0x20: return (b & SCE_CTRL_CROSS) != 0;
    case 'A': return (b & SCE_CTRL_CIRCLE) != 0;
    case 'Z': return (b & SCE_CTRL_SQUARE) != 0;
    case 'C': return (b & SCE_CTRL_TRIANGLE) != 0;
    case 0x0D: return (b & SCE_CTRL_START) != 0;
    case 0x1B: return (b & SCE_CTRL_SELECT) != 0;
    case 0x10: return (b & SCE_CTRL_LTRIGGER) != 0;
    case 0x11: return (b & SCE_CTRL_RTRIGGER) != 0;
    default: return 0;
    }
}

void rt_platform_fatal(const char *msg) { (void)msg; }   // already in the log; nothing to show it with

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir(HK_DATA_DIR, 0777);
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
    SDL_strlcpy(g_game_dir, HK_DATA_DIR, sizeof g_game_dir);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER)) {
        rt_log("SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    g_win = SDL_CreateWindow("Handkerchief", 0, 0, 960, 544, 0);
    g_ren = g_win ? SDL_CreateRenderer(g_win, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (g_ren) SDL_RenderSetLogicalSize(g_ren, 640, 480);
    rt_log("[vita] window %p renderer %p\n", (void *)g_win, (void *)g_ren);
    if (rt_init(HK_GUEST_MB << 20)) rt_fatal("guest memory: allocation of %u MB failed", HK_GUEST_MB);
    int r = rt_load_exe(HK_DATA_DIR "/HDKWIN.EXE");
    if (r) rt_fatal("cannot load " HK_DATA_DIR "/HDKWIN.EXE (%d): copy your game files there", r);
    rt_fill_iat();
    rt_log("game loaded from %s\n", g_game_dir);
    rt_run_entry();
    SDL_Quit();
    return 0;
}
