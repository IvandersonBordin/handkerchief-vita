// PC entry point for the recompiled Handkerchief: SDL2 window, game exe loading, recompiled code start.
// Structure derived from sh3vita port/src/pc/main.c (GPL-3.0).
#include "rt.h"
void w32_set_lang_overlay(const char *dir);
#include "plat.h"
#include "cp1252.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include "hk/font_data.h"
#include "vita/pause_menu.h"
#include "vita/dlg_menu.h"

static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture *g_tex;

SDL_Window *plat_sdl_window(void) { return g_win; }
int plat_width(void) { return 640; }
int plat_height(void) { return 480; }
void plat_show_window(void) { SDL_ShowWindow(g_win); }

void plat_shutdown(void) {}
int plat_message_box(const char *title, const char *text, unsigned type) {
    if ((type & 15u) || getenv("HK_HEADLESS")) return 0;   // only MB_OK is answered; no fabricated confirmation
    unsigned icon = (type & 0xf0u) == 0x10u ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_INFORMATION;
    char title_utf8[768], text_utf8[6144];
    rt_cp1252_utf8(title_utf8, sizeof title_utf8, title ? title : "");
    rt_cp1252_utf8(text_utf8, sizeof text_utf8, text ? text : "");
    return SDL_ShowSimpleMessageBox(icon, title_utf8, text_utf8, g_win) == 0 ? 1 : 0;
}

// Typed text (plat.h): SDL_TEXTINPUT and Backspace while an EDIT control has the focus. HK_TEXT=frame:text[,...]
// types text at a presented frame for headless tests ('~' stands for Backspace).
static int g_text_on;
static char g_typed[256];
static int g_ntyped;
static void typed_add(const char *utf8) {
    for (const unsigned char *p = (const unsigned char *)utf8; *p && g_ntyped < (int)sizeof g_typed - 1; p++) {
        unsigned ch = *p;
        if (ch >= 0xC2 && ch <= 0xC3 && (p[1] & 0xC0) == 0x80) { ch = ((ch & 0x1F) << 6) | (p[1] & 0x3F); p++; }   // U+0080-U+00FF
        else if (ch >= 0x80) { while ((p[1] & 0xC0) == 0x80) p++; ch = '?'; }
        if (ch >= 0x20 || ch == '\b') g_typed[g_ntyped++] = (char)ch;
    }
}
int plat_has_keyboard(void) { return 1; }
void plat_text_enable(int on) {
    if (on == g_text_on) return;
    g_text_on = on;
    g_ntyped = 0;
    if (on) SDL_StartTextInput(); else SDL_StopTextInput();
}
int plat_text_typed(char *out, int cap) {
    const char *s = getenv("HK_TEXT");
    extern unsigned g_frame;
    static unsigned done_frame;
    while (s && *s && g_text_on) {   // scripted text, once per entry
        char *e;
        unsigned at = (unsigned)strtoul(s, &e, 10);
        if (*e != ':') break;
        const char *t = e + 1, *end = strchr(t, ',');
        size_t n = end ? (size_t)(end - t) : strlen(t);
        if (g_frame >= at && at > done_frame) {
            for (size_t i = 0; i < n && g_ntyped < (int)sizeof g_typed - 1; i++) g_typed[g_ntyped++] = t[i] == '~' ? '\b' : t[i];
            done_frame = at;
        }
        s = end ? end + 1 : t + n;
    }
    int n = g_ntyped < cap ? g_ntyped : cap;
    memcpy(out, g_typed, (size_t)n);
    memmove(g_typed, g_typed + n, (size_t)(g_ntyped - n));
    g_ntyped -= n;
    return n;
}
int plat_text_open(const char *title, const char *text, int maxlen) { (void)title; (void)text; (void)maxlen; return 0; }
int plat_text_status(char *out, int cap) { (void)out; (void)cap; return 2; }

int plat_pump_events(void) {
    SDL_Event e;
    int quit = 0;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = 1;
        else if (e.type == SDL_TEXTINPUT && g_text_on) typed_add(e.text.text);
        else if (e.type == SDL_KEYDOWN && g_text_on && e.key.keysym.sym == SDLK_BACKSPACE) typed_add("\b");
    }
    return quit;
}

// HK_SHOT=N[,M...]: writes shot_N.ppm (in the working folder) when frame N is presented.
static void maybe_shot(const uint32_t *pixels, int width, int height, int pitch_bytes) {
    static unsigned n;
    const char *list = getenv("HK_SHOT");
    n++;
    for (const char *s = list; s && *s;) {
        char *end;
        unsigned want = (unsigned)strtoul(s, &end, 10);
        if (end == s) break;
        if (want == n) {
            char path[64];
            snprintf(path, sizeof path, "shot_%u.ppm", n);
            FILE *f = fopen(path, "wb");
            if (f) {
                fprintf(f, "P6 %d %d 255\n", width, height);
                for (int y = 0; y < height; y++)
                    for (int x = 0; x < width; x++) {
                        uint32_t v = pixels[y * (pitch_bytes / 4) + x];
                        unsigned char rgb[3] = {(unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v};
                        fwrite(rgb, 1, 3, f);
                    }
                fclose(f);
                rt_log("[shot] %s\n", path);
            }
        }
        s = *end == ',' ? end + 1 : end;
    }
}

// RESULTS panel (the Vita's Select, port/vita/pause_menu.h pm_results): while Tab is held in a stage, or for the test
// aid HK_RESULTS_AT=frame[:frames] (default 50 frames), drawn over a copy of the frame; the game keeps running.
int hk_results_read(uint32_t out[13]);   // port/hk/results.c
static int results_shown(void) {
    extern unsigned g_frame;
    static int at = -2, len = 50;
    if (at == -2) { const char *e = getenv("HK_RESULTS_AT"); at = e ? atoi(e) : -1; if (e && strchr(e, ':')) len = atoi(strchr(e, ':') + 1); }
    if (at >= 0 && g_frame >= (unsigned)at && g_frame < (unsigned)at + (unsigned)len) return 1;
    const Uint8 *k = getenv("HK_HEADLESS") ? NULL : SDL_GetKeyboardState(NULL);
    return k && k[SDL_SCANCODE_TAB];
}
static int g_loading;   // plat_show_loading: the frame is shown darkened under the "Loading..." box
void plat_show_xrgb(uint32_t *pixels, int width, int height, int pitch_bytes) {
    uint32_t rv[13];
    // RESULTS is a 960x544 overlay at the output resolution (as on the Vita): composited on the CPU into a 960x544
    // frame here (letterboxed 4:3), shown through its own texture with the logical size lifted.
    static uint32_t ovl[PM_OW * PM_OH], comp[PM_OW * PM_OH];
    static HkDlgView dv;
    static uint64_t dv_hash;
    static uint32_t dovl[PM_OW * PM_OH];
    static int dv_valid;
    int dlgv = !g_loading && width <= HK_WS_MAX && height <= 480 && hk_dlg_view(&dv);
    int res = dlgv || g_loading || (results_shown() && width <= HK_WS_MAX && height <= 480 && hk_results_read(rv));
    if (!dlgv) dv_valid = 0;
    if (dlgv) {   // the game's dialogs / menu bar in the pause menu's style (port/vita/dlg_menu.h), over the dimmed frame
        const uint64_t hh = hk_dlgview_hash(&dv);
        if (!dv_valid || hh != dv_hash) { pm_overlay_dlg(dovl, &dv); dv_hash = hh; dv_valid = 1; }
        pm_composite(comp, pixels, width, height, pitch_bytes / 4, 0, dv.kind == DV_MENU ? 96 : 128, dovl);
    } else if (g_loading) {
        pm_overlay_msgbox(ovl, "", hk_tr(S_LOADING), NULL, NULL);
        pm_composite(comp, pixels, width, height, pitch_bytes / 4, 0, 160, ovl);
    } else if (res) {
        dv_valid = 0;
        pm_overlay_results(ovl, rv);
        pm_composite(comp, pixels, width, height, pitch_bytes / 4, 0, 0, ovl);
    }
    maybe_shot(res ? comp : pixels, res ? PM_OW : width, res ? PM_OH : height, res ? PM_OW * 4 : pitch_bytes);
    if (!g_ren) return;
    if (!g_tex) g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, HK_WS_MAX, 480);   // fixed size, never recreated
    static SDL_Texture *rtex;
    SDL_Rect src = {0, 0, width, height};
    SDL_RenderClear(g_ren);
    if (res) {
        if (!rtex) rtex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, PM_OW, PM_OH);
        SDL_UpdateTexture(rtex, NULL, comp, PM_OW * 4);
        SDL_RenderSetLogicalSize(g_ren, 0, 0);
        SDL_RenderCopy(g_ren, rtex, NULL, NULL);
        SDL_RenderSetLogicalSize(g_ren, g_hk_ws_w, 480);
    } else {
        SDL_UpdateTexture(g_tex, &src, pixels, pitch_bytes);
        SDL_RenderCopy(g_ren, g_tex, &src, NULL);
    }
    SDL_RenderPresent(g_ren);
}

// "Continue from the last save?" after GAME OVER (port/hk/gameover.c): the Vita's message box look (960x544 overlay over
// the darkened last frame). Enter / X = Yes, Esc / Z = No. Test aid HK_GAMEOVER_ANSWER=y|n: writes shot_gameover.ppm
// (the composited question) and answers at once; headless without it answers No.
int hk_gameover_ask_pending(void);
void hk_gameover_answer(int yes);
int hk_gameover_key(int vk);
int hk_gameover_hiding(void);
static int gameover_ask(uint32_t *pixels, int width, int height, int pitch_bytes) {
    static uint32_t ovl[PM_OW * PM_OH], comp[PM_OW * PM_OH];
    const char *auto_ans = getenv("HK_GAMEOVER_ANSWER");
    pm_overlay_msgbox(ovl, "GAME OVER", hk_tr(S_GO_ASK), hk_tr(S_YES), hk_tr(S_NO));
    pm_composite(comp, pixels, width, height, pitch_bytes / 4, 0, 192, ovl);
    if (auto_ans) {
        FILE *f = fopen("shot_gameover.ppm", "wb");
        if (f) {
            fprintf(f, "P6 %d %d 255\n", PM_OW, PM_OH);
            for (int i = 0; i < PM_OW * PM_OH; i++) { unsigned char rgb[3] = {(unsigned char)(comp[i] >> 16), (unsigned char)(comp[i] >> 8), (unsigned char)comp[i]}; fwrite(rgb, 1, 3, f); }
            fclose(f);
        }
        return *auto_ans == 'y';
    }
    if (getenv("HK_HEADLESS") || !g_ren) return 0;
    static SDL_Texture *gtex;
    if (!gtex) gtex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, PM_OW, PM_OH);
    SDL_UpdateTexture(gtex, NULL, comp, PM_OW * 4);
    for (int armed = 0;;) {
        if (plat_pump_events()) return 0;
        const Uint8 *k = SDL_GetKeyboardState(NULL);
        const int yes = k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_X], no = k[SDL_SCANCODE_ESCAPE] || k[SDL_SCANCODE_Z];
        if (!armed) armed = !yes && !no;
        else if (yes || no) return yes;
        SDL_RenderClear(g_ren);
        SDL_RenderSetLogicalSize(g_ren, 0, 0);
        SDL_RenderCopy(g_ren, gtex, NULL, NULL);
        SDL_RenderSetLogicalSize(g_ren, g_hk_ws_w, 480);
        SDL_RenderPresent(g_ren);
        SDL_Delay(16);
    }
}

// HK_PRESENT_MS=n: each present takes at least n ms, to reproduce a slow console present on PC (the Vita needs ~45 ms
// per frame at 848x480 with the dialog overlay).
void plat_present_xrgb(uint32_t *pixels, int width, int height, int pitch_bytes) {
    static int ms = -1;
    if (ms < 0) ms = getenv("HK_PRESENT_MS") ? atoi(getenv("HK_PRESENT_MS")) : 0;
    if (ms > 0) SDL_Delay((uint32_t)ms);
    // HK_CHEAT=1 (test aid, like the Vita pause-screen L): Unrivaled Mode flag 0x4AC84C on; the invincibility flag
    // 0x47BCD8 starts at 1 in .data, so the player is invincible (docs/save-and-stages.md).
    static int cheat = -1;
    if (cheat < 0) cheat = getenv("HK_CHEAT") != NULL;
    if (cheat) *(uint32_t *)(g_mem + 0x4AC84Cu) = 1;
    plat_show_xrgb(pixels, width, height, pitch_bytes);
    if (hk_gameover_ask_pending()) hk_gameover_answer(gameover_ask(pixels, width, height, pitch_bytes));
}
void plat_show_loading(uint32_t *pixels, int width, int height, int pitch_bytes) {
    g_loading = 1; plat_show_xrgb(pixels, width, height, pitch_bytes); g_loading = 0;
}
int plat_present_threaded(void) { const char *e = getenv("HK_PRESENT_THREAD"); return e ? atoi(e) : 0; }
int plat_poll_main(void) { return 0; }
// present_thread=3 runs this on the worker: needs a renderer usable from any thread (SDL_RENDER_DRIVER=software).
void plat_present_async(uint32_t *pixels, int width, int height, int pitch_bytes) { plat_show_xrgb(pixels, width, height, pitch_bytes); }

// HK_KEYS=frame:vk:frames[,...] (hex vk): scripted key presses for headless tests, e.g. 1600:0d:10 holds Enter
// for 10 frames from presented frame 1600.
static int scripted_key(int vk) {
    extern unsigned g_frame;
    const char *s = getenv("HK_KEYS");
    while (s && *s) {
        char *e;
        unsigned at = (unsigned)strtoul(s, &e, 10);
        if (*e != ':') break;
        int key = (int)strtol(e + 1, &e, 16);
        if (*e != ':') break;
        unsigned len = (unsigned)strtoul(e + 1, &e, 10);
        if (key == vk && g_frame >= at && g_frame < at + len) return 1;
        s = *e == ',' ? e + 1 : e;
    }
    return 0;
}

#if HK_DEVTOOLS
int hk_stress_key(int vk);   // port/hk/memlog.c
#else
#define hk_stress_key(vk) 0
#endif
int plat_vk_down(int vk) {
    if (scripted_key(vk) || hk_stress_key(vk) || ((vk == 0x28 || vk == 0x20 || vk == 0x0D) && hk_gameover_key(vk))) return 1;
    if (hk_gameover_hiding()) return 0;   // hidden load chain after GAME OVER: only its injected keys (above), never the player's
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    if (!k) return 0;
    switch (vk) {
    case 0x25: return k[SDL_SCANCODE_LEFT];
    case 0x26: return k[SDL_SCANCODE_UP];
    case 0x27: return k[SDL_SCANCODE_RIGHT];
    case 0x28: return k[SDL_SCANCODE_DOWN];
    case 0x0D: return k[SDL_SCANCODE_RETURN];
    case 0x1B: return k[SDL_SCANCODE_ESCAPE];
    case 0x20: return k[SDL_SCANCODE_SPACE];
    case 0x10: return k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT];
    case 0x11: return k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL];
    case 0x12: return k[SDL_SCANCODE_LALT] || k[SDL_SCANCODE_RALT];
    case 0x70: return k[SDL_SCANCODE_F1];
    case 0x71: return k[SDL_SCANCODE_F2];   // Save This Game
    case 0x73: return k[SDL_SCANCODE_F4];   // Stop This Game
    case 0x79: return k[SDL_SCANCODE_F10];  // the game's menu bar (port: hk_menu_key)
    default:
        if (vk >= 'A' && vk <= 'Z') return k[SDL_SCANCODE_A + (vk - 'A')];
        return 0;
    }
}

void rt_platform_fatal(const char *msg) {
    if (getenv("HK_HEADLESS")) return;
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Handkerchief (recomp)", msg, g_win);
}

// HK_DUMP_AFTER=seconds: logs where the main thread is (rt_dump_hang: host function and guest return addresses).
static int dump_thread(void *arg) {
    int secs = (int)(intptr_t)arg;
    for (;;) { SDL_Delay((Uint32)secs * 1000u); rt_dump_hang(); }
    return 0;
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : getenv("HK_GAME_DIR") ? getenv("HK_GAME_DIR") : "original-data";
    SDL_strlcpy(g_game_dir, dir, sizeof g_game_dir);
    // Headless test runs never use the real audio device (agents' runs played through the owner's speakers);
    // SDL_AUDIODRIVER=disk can still be set explicitly to record the mix.
    if (getenv("HK_HEADLESS") && !getenv("SDL_AUDIODRIVER")) setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");   // keep default SIGINT/SIGTERM: `timeout` must stop test runs
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    // HK_WIDE=848: native widescreen (port/hk/wide.c, docs/widescreen-plan.md); unset = the original 640x480.
    hk_dlg_set_pretty(getenv("HK_DLG_CLASSIC") == NULL);   // HK_DLG_CLASSIC=1: the old 640x480 dialogs (before/after shots)
    hk_wide_set(getenv("HK_WIDE") ? atoi(getenv("HK_WIDE")) : 640);
    if (!getenv("HK_HEADLESS")) {
        g_win = SDL_CreateWindow("Handkerchief (recomp)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 g_hk_ws_w * 3 / 2, 720, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
        g_ren = g_win ? SDL_CreateRenderer(g_win, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
        if (g_ren) SDL_RenderSetLogicalSize(g_ren, g_hk_ws_w, 480);
    }
    {   // saves live in USER (the builder does not ship it): create it when no spelling of it exists yet
        struct stat st;
        char up[620];
        int have = 0;
        static const char *const nm[] = {"USER", "User", "user"};
        if (!stat(g_game_dir, &st))
            for (int i = 0; i < 3; i++) { snprintf(up, sizeof up, "%s/%s", g_game_dir, nm[i]); have |= !stat(up, &st); }
        if (!have && !stat(g_game_dir, &st)) { snprintf(up, sizeof up, "%s/USER", g_game_dir); mkdir(up, 0777); }
    }
    if (rt_init(256u << 20)) rt_fatal("guest memory: allocation failed");
    char exe[600];
    SDL_snprintf(exe, sizeof exe, "%s/HDKWIN.EXE", g_game_dir);
    // HK_LANG=pt: the PT-BR translation installed in <game dir>/lang/ptbr (HDKWINBR.EXE and BITMAP/), as on the Vita.
    if (getenv("HK_LANG") && !strcmp(getenv("HK_LANG"), "pt")) {
        char ov[600], br[700];
        SDL_snprintf(ov, sizeof ov, "%s/lang/ptbr", g_game_dir);
        SDL_snprintf(br, sizeof br, "%s/HDKWINBR.EXE", ov);
        FILE *tf = fopen(br, "rb");
        if (tf) { fclose(tf); SDL_strlcpy(exe, br, sizeof exe); w32_set_lang_overlay(ov); }
        else fprintf(stderr, "HK_LANG=pt: %s not found, using English\n", br);
    }
    int r = rt_load_exe(exe);
    if (r) rt_fatal("cannot load %s (%d)", exe, r);
    rt_fill_iat();
    { void hk_patch_data(void); hk_patch_data(); }
    rt_log("game loaded from %s\n", g_game_dir);
#if HK_DEVTOOLS
    {   // memory log and stress driver (port/hk/memlog.c)
        extern int g_hk_memlog; void hk_stress_init(const char *);
        if (getenv("HK_MEMLOG")) g_hk_memlog = atoi(getenv("HK_MEMLOG"));
        extern int g_h_maxpages;   // port/win32/handles.c: 1 = the old fixed 1024-slot table
        if (getenv("HK_HANDLE_PAGES")) g_h_maxpages = atoi(getenv("HK_HANDLE_PAGES"));
        hk_stress_init(getenv("HK_STRESS"));
    }
#endif
    if (getenv("HK_DUMP_AFTER")) SDL_CreateThread(dump_thread, "hkdump", (void *)(intptr_t)atoi(getenv("HK_DUMP_AFTER")));
    rt_run_entry();
    SDL_Quit();
    return 0;
}

// Title menu OPTIONS item (port/hk/dialog.c): the Vita opens its Settings page here; the PC build has no such screen
// (its settings are in the game's own File menu), so the item just returns to the title.
void hk_title_options(void) { fprintf(stderr, "[opt] title OPTIONS selected\n"); }
