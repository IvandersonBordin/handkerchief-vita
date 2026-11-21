// PS Vita entry point for the recompiled Handkerchief: SDL2 window 960x544 (game 640x480 letterboxed),
// game files in ux0:data/handkerchief/ (the player's own copy, including HDKWIN.EXE), log in the same folder.
// Structure follows port/pc/main.c; startup settings follow the vita-port skill (assets/vita_runtime.c).
#include "rt.h"
#include "plat.h"
#include <psp2/ctrl.h>
#include <psp2/io/stat.h>
#include <psp2/power.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ime_dialog.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hk/font_data.h"

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

// Text entry (plat.h): the system keyboard (IME dialog). SDL's GXM renderer calls sceCommonDialogUpdate on every
// SDL_RenderPresent, which draws the dialog; the game's dialog keeps presenting while it waits (port/hk/dialog.c).
// Not tested on the console yet.
static SceWChar16 g_ime_title[SCE_IME_DIALOG_MAX_TITLE_LENGTH], g_ime_init[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1],
    g_ime_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
static int g_ime_open;
static void latin1_to_utf16(SceWChar16 *out, int cap, const char *s) {
    int n = 0;
    for (; s && *s && n < cap - 1; s++) out[n++] = (uint8_t)*s;
    out[n] = 0;
}
int plat_has_keyboard(void) { return 0; }
void plat_text_enable(int on) { (void)on; }
int plat_text_typed(char *out, int cap) { (void)out; (void)cap; return 0; }
int plat_text_open(const char *title, const char *text, int maxlen) {
    if (g_ime_open) return 1;
    if (maxlen <= 0 || maxlen > SCE_IME_DIALOG_MAX_TEXT_LENGTH) maxlen = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
    latin1_to_utf16(g_ime_title, SCE_IME_DIALOG_MAX_TITLE_LENGTH, title);
    latin1_to_utf16(g_ime_init, maxlen + 1, text);
    memset(g_ime_text, 0, sizeof g_ime_text);
    SceImeDialogParam p;
    sceImeDialogParamInit(&p);
    p.supportedLanguages = SCE_IME_LANGUAGE_ENGLISH;
    p.languagesForced = SCE_TRUE;
    p.type = SCE_IME_TYPE_BASIC_LATIN;
    p.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_WITH_CLEAR;
    p.title = g_ime_title;
    p.maxTextLength = (SceUInt32)maxlen;
    p.initialText = g_ime_init;
    p.inputTextBuffer = g_ime_text;
    int r = sceImeDialogInit(&p);
    rt_log("[ime] sceImeDialogInit -> %08x\n", (unsigned)r);
    g_ime_open = r >= 0;
    return g_ime_open;
}
int plat_text_status(char *out, int cap) {
    if (!g_ime_open) return 2;
    if (sceImeDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) return 0;
    SceImeDialogResult r;
    memset(&r, 0, sizeof r);
    sceImeDialogGetResult(&r);
    sceImeDialogTerm();
    g_ime_open = 0;
    if (r.button != SCE_IME_DIALOG_BUTTON_ENTER) return 2;
    int n = 0;
    for (const SceWChar16 *w = g_ime_text; *w && n < cap - 1; w++) out[n++] = (char)(*w < 256 ? *w : '?');
    out[n] = 0;
    return 1;
}

int plat_pump_events(void) {
    SDL_Event e;
    int quit = 0;
    while (SDL_PollEvent(&e))
        if (e.type == SDL_QUIT) quit = 1;
    return quit;
}

// Aspect: 0 = 4:3 letterboxed (original proportions), 1 = stretched to the full 16:9 screen.
// Toggled with Triangle on the pause screen, saved in HK_DATA_DIR/hkport.cfg.
// FPS counter (frames shown per second, top left): toggled with Square on the pause screen, saved there too.
#define HK_CFG HK_DATA_DIR "/hkport.cfg"
static int g_wide, g_fps;
static void apply_aspect(void) {
    if (!g_ren) return;
    if (g_wide) SDL_RenderSetLogicalSize(g_ren, 0, 0);
    else SDL_RenderSetLogicalSize(g_ren, 640, 480);
}
static void load_cfg(void) {
    FILE *f = fopen(HK_CFG, "r");
    if (!f) return;
    char line[64];
    while (fgets(line, sizeof line, f)) { sscanf(line, "wide=%d", &g_wide); sscanf(line, "fps=%d", &g_fps); }
    fclose(f);
}
static void save_cfg(void) {
    FILE *f = fopen(HK_CFG, "w");
    if (f) { fprintf(f, "wide=%d\nfps=%d\n", g_wide, g_fps); fclose(f); }
}
// Text on a 640x480 XRGB buffer with the port's bitmap font (cell 18).
static void draw_text(uint32_t *buf, int x, int y, const char *s, uint32_t color) {
    const HkFont *f = &hk_fonts[1];
    for (; *s; s++) {
        uint8_t ch = (uint8_t)*s;
        const HkGlyph *g = &f->glyphs[(ch < 0x20 ? '?' : ch) - 0x20];
        for (int gy = 0; gy < f->cell; gy++)
            for (int gx = 0; gx < g->width; gx++)
                if (((f->rows[g->offset + gy] >> gx) & 1u) && x + gx >= 0 && x + gx < 640 && y + gy >= 0 && y + gy < 480)
                    buf[(y + gy) * 640 + x + gx] = color;
        x += g->advance;
    }
}

static void show_frame(const uint32_t *pixels, int width, int height, int pitch_bytes) {
    if (!g_ren) return;
    if (!g_tex) g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_UpdateTexture(g_tex, NULL, pixels, pitch_bytes);
    SDL_RenderClear(g_ren);
    SDL_RenderCopy(g_ren, g_tex, NULL, NULL);
    SDL_RenderPresent(g_ren);
}

// Start pauses: the last frame is shown darkened with a pause sign until Start is pressed again. The game's
// clocks skip the paused time (g_pause_qpc, read by w32_qpc/w32_tick_ms) so it does not try to catch up.
extern uint64_t g_pause_qpc;
// Select on the pause screen leaves it and opens the game's menu bar (File: save / stop / exit, Option: Key and Game
// Configuration, About): plat_vk_down reports F10 for a few frames, which USER32 turns into the game's right click
// (port/hk/dialog.c hk_menu_key). In play, Select stays Esc (the game's exit prompt).
static unsigned g_menu_until;
static void pause_loop(const uint32_t *pixels, int width, int height, int pitch_bytes) {
    static uint32_t dim[480][640];
    uint64_t t0 = SDL_GetPerformanceCounter();
    for (int y = 0; y < height && y < 480; y++)
        for (int x = 0; x < width && x < 640; x++)
            dim[y][x] = (pixels[y * (pitch_bytes / 4) + x] >> 2) & 0x3f3f3fu;
    for (int y = 200; y < 280; y++)
        for (int x = 0; x < 24; x++) { dim[y][296 + x] = 0xffffffu; dim[y][320 + x] = 0xffffffu; }
    draw_text(&dim[0][0], 230, 300, "Start: continue", 0xffffffu);
    SceCtrlData pad;
    int released = 0, tri_was = 1, sq_was = 1, sel_was = 1;
    for (;;) {
        static uint32_t shown[480][640];
        memcpy(shown, dim, sizeof shown);
        draw_text(&shown[0][0], 230, 324, g_wide ? "Triangle: screen 16:9" : "Triangle: screen 4:3", 0xffff80u);
        draw_text(&shown[0][0], 230, 348, g_fps ? "Square: FPS counter on" : "Square: FPS counter off", 0xffff80u);
        draw_text(&shown[0][0], 230, 372, "Select: game menu", 0xffff80u);
        show_frame(&shown[0][0], 640, 480, 640 * 4);
        sceCtrlPeekBufferPositive(0, &pad, 1);
        int tri = (pad.buttons & SCE_CTRL_TRIANGLE) != 0, sq = (pad.buttons & SCE_CTRL_SQUARE) != 0;
        if (tri && !tri_was) { g_wide = !g_wide; apply_aspect(); save_cfg(); }
        if (sq && !sq_was) { g_fps = !g_fps; save_cfg(); }
        int sel = (pad.buttons & SCE_CTRL_SELECT) != 0;
        if (sel && !sel_was) { extern unsigned g_frame; g_menu_until = g_frame + 3; break; }
        tri_was = tri;
        sq_was = sq;
        sel_was = sel;
        if (!(pad.buttons & SCE_CTRL_START)) released = 1;
        else if (released) break;
        SDL_Delay(16);
    }
    while (sceCtrlPeekBufferPositive(0, &pad, 1) >= 0 && (pad.buttons & (SCE_CTRL_START | SCE_CTRL_SELECT))) SDL_Delay(16);
    g_pause_qpc += SDL_GetPerformanceCounter() - t0;
}

void plat_present_xrgb(const uint32_t *pixels, int width, int height, int pitch_bytes) {
    static int start_was;
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(0, &pad, 1);
    int start = (pad.buttons & SCE_CTRL_START) != 0;
    if (start && !start_was) { pause_loop(pixels, width, height, pitch_bytes); start = 0; }
    start_was = start;
    // FPS: frames shown in the last full second (the game targets 25).
    static uint64_t t_sec;
    static int count, fps;
    uint64_t now = SDL_GetPerformanceCounter(), hz = SDL_GetPerformanceFrequency();
    count++;
    if (!t_sec) t_sec = now;
    if (now - t_sec >= hz) { fps = (int)((uint64_t)count * hz / (now - t_sec)); count = 0; t_sec = now; }
    if (g_fps && width == 640 && height <= 480) {
        static uint32_t buf[480][640];
        for (int y = 0; y < height; y++) memcpy(buf[y], pixels + y * (pitch_bytes / 4), 640 * 4);
        char txt[16];
        snprintf(txt, sizeof txt, "%d FPS", fps);
        for (int y = 2; y < 22; y++)
            for (int x = 2; x < 74; x++) buf[y][x] = (buf[y][x] >> 2) & 0x3f3f3fu;
        draw_text(&buf[0][0], 6, 3, txt, fps >= 24 ? 0x80ff80u : 0xffff60u);
        show_frame(&buf[0][0], 640, height, 640 * 4);
        return;
    }
    show_frame(pixels, width, height, pitch_bytes);
}

// Pad mapping. Key tests in stage A0 (2026-11-20, docs/status.md) show X shoots, A or Up jumps, S crouches; the
// game's Key Configuration dialog (resource 1492) lists Shoot & Bomb, Turncoat Bomb, Switch & Rotation, Jump and
// Crouch. Menus confirm with Enter or space and go back with Z/X/C (0x4117D4), checking confirm first, so Cross
// sends X + space: it shoots in play and confirms in menus. Square = A (jump), down = arrow + S (crouch),
// Circle = Z (Switch & Rotation), Triangle = C (Turncoat Bomb), as the Key Configuration defaults show; L = Shift,
// R = Ctrl, Select = Esc. Start is the port's own pause (plat_present_xrgb), not a game key. In the game's dialogs
// (port/hk/dialog.c) Cross presses the focused control, Triangle presses OK and Circle or Select cancel.
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
    case 'A': return (b & SCE_CTRL_SQUARE) != 0;
    case 'Z': return (b & SCE_CTRL_CIRCLE) != 0;
    case 'C': return (b & SCE_CTRL_TRIANGLE) != 0;
    case 0x1B: return (b & SCE_CTRL_SELECT) != 0;
    case 0x79: return g_frame < g_menu_until;   // F10: the game's menu, from the pause screen
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
    // The game's main thread keeps core 0 to itself; audio (port/hk/dsound.c), log and the game's helper
    // threads run on cores 1-2. Without this the audio thread could share the main thread's core and starve in
    // heavy scenes (sound dropping out when the house is destroyed, in tunnels and on map changes).
    sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(), SCE_KERNEL_CPU_MASK_USER_0);
    SDL_strlcpy(g_game_dir, HK_DATA_DIR, sizeof g_game_dir);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER)) {
        rt_log("SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    g_win = SDL_CreateWindow("Handkerchief", 0, 0, 960, 544, 0);
    g_ren = g_win ? SDL_CreateRenderer(g_win, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
    load_cfg();
    apply_aspect();
    rt_log("[vita] window %p renderer %p\n", (void *)g_win, (void *)g_ren);
    if (rt_init(HK_GUEST_MB << 20)) rt_fatal("guest memory: allocation of %u MB failed", HK_GUEST_MB);
    int r = rt_load_exe(HK_DATA_DIR "/HDKWIN.EXE");
    if (r) rt_fatal("cannot load " HK_DATA_DIR "/HDKWIN.EXE (%d): copy your game files there", r);
    rt_fill_iat();
    { void hk_patch_data(void); hk_patch_data(); }
    rt_log("game loaded from %s\n", g_game_dir);
    rt_run_entry();
    SDL_Quit();
    return 0;
}
