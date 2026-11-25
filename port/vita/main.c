// PS Vita entry point for the recompiled Handkerchief: SDL2 window 960x544 (game 640x480 letterboxed, or 848x480 with wide_native=1),
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
#include "pause_menu.h"
#include "safe_write.h"
#include "dlg_menu.h"
void w32_post_message(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp);
void w32_set_lang_overlay(const char *dir);
#define HWND_MAIN_DLG 0x00010010u   // port/hk/dialog.c

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
// ExitProcess path (hk_host_shutdown): the present worker and audio are already stopped; release the renderer and
// window before the process exits so the GPU is idle and unused when the system tears it down.
void plat_shutdown(void) {
    SDL_Renderer *r = g_ren; SDL_Window *w = g_win;
    g_ren = NULL; g_win = NULL;
    if (r) SDL_DestroyRenderer(r);
    if (w) SDL_DestroyWindow(w);
    SDL_Quit();
}
int plat_width(void) { return 640; }
int plat_height(void) { return 480; }
void plat_show_window(void) {}

static int msgbox_loop(const char *title, const char *text, unsigned type);   // below, after the frame helpers
int plat_message_box(const char *title, const char *text, unsigned type) {
    rt_dlog("[msgbox] %s: %s (type %u)\n", title ? title : "", text ? text : "", type);
    int r = msgbox_loop(title, text, type);
    rt_dlog("[msgbox] answer %d\n", r);
    return r;
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
    rt_dlog("[ime] sceImeDialogInit -> %08x\n", (unsigned)r);
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
// menu_scale=2|4 (hkport.cfg only): largest text scale of the pause menus (1x, 2x; default 4; 3 counts as 2).
// wide_native=1 (hkport.cfg only, off by default; no pause toggle yet): native widescreen, the game draws an
// 848x480 screen (port/hk/wide.c, docs/widescreen-plan.md) that fills the 960x544 screen at the 4:3 setting.
#define HK_CFG HK_DATA_DIR "/hkport.cfg"
#include "hkdev.h"
#if HK_DEVTOOLS   // developer build only (cmake -DHK_DEVTOOLS=ON): test aids and their hkport.cfg keys
extern int g_hk_prof;   // port/hk/ddraw.c: hkport.cfg prof=1 logs per-frame timings to hklog.txt
extern int g_h_maxpages;   // port/win32/handles.c: hkport.cfg handle_pages=<n> caps the handle table (1 = old 1024 slots)
extern int g_hk_memlog;   // port/hk/memlog.c: hkport.cfg memlog=1 logs "[mem]" lines to hklog.txt (2: + guest heap histogram)
void hk_stress_init(const char *spec);   // port/hk/memlog.c
int hk_stress_key(int vk);
static char g_stress_spec[32];   // hkport.cfg stress=<kind>:<n> (area, territory, save, menus, idle): in-process stress driver, a test aid
#endif
#define HK_PT_DEFAULT 3   // asynchronous present: stable over exit/pause/dialogs/game-over/options in the console soak (2026-11-24)
static int g_present_thread = HK_PT_DEFAULT;   // hkport.cfg present_thread=0|1|2|3 (see plat_present_threaded); player build too
static int g_map[PM_NBTN];   // gameplay button map (pause_menu.h), hkport.cfg map_<button>=<action>
static const unsigned g_btn_mask[PM_NBTN] = {SCE_CTRL_CROSS, SCE_CTRL_SQUARE, SCE_CTRL_CIRCLE, SCE_CTRL_TRIANGLE, SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER, SCE_CTRL_SELECT};
#define HK_LANG_DIR HK_DATA_DIR "/lang/ptbr"   // the player's copy of the PT-BR fan translation (release/README.md)
static int g_lang, g_lang_ok, g_started_lang;   // hkport.cfg lang=pt; g_lang_ok: lang/ptbr/HDKWINBR.EXE exists
static int lang_files_ok(void) { FILE *f = fopen(HK_LANG_DIR "/HDKWINBR.EXE", "rb"); if (f) fclose(f); return f != NULL; }
static int g_wide, g_fps, g_wide_native = 1, g_vol = 7, g_mus = 6;   // fresh install: native widescreen (owner decision)
// g_vol: master volume 1..10 (pause screen, Left/Right)
// rec=1 (hkport.cfg only): the final sound output is recorded to HK_DATA_DIR/rec.wav (last 60 s, the minute before in
// rec.prev.wav; port/hk/dsound.c, recorder thread on core 2) so a bad-sounding moment can be analysed on a PC.
#if HK_DEVTOOLS
static int g_rec;
extern const char *g_hk_rec_path;   // port/hk/dsound.c
#endif
extern float g_hk_volume;   // port/hk/dsound.c
extern float g_hk_music_volume;   // port/hk/dsound.c
static void apply_volume(void) { if (g_mus < 1) g_mus = 1; if (g_mus > 10) g_mus = 10; g_hk_music_volume = 0.18f * (float)g_mus; if (g_vol < 1) g_vol = 1; if (g_vol > 10) g_vol = 10; g_hk_volume = 0.06f * (float)g_vol; }
static void apply_aspect(void) {
    if (!g_ren) return;
    if (g_wide) SDL_RenderSetLogicalSize(g_ren, 0, 0);
    else SDL_RenderSetLogicalSize(g_ren, g_hk_ws_w, 480);
}

// Debug / experiment switches read from hkport.cfg as `key=number` and handed to the port as environment variables
// (the same variables work on the PC). Kept across save_cfg so changing a setting in the pause menu does not drop them.
//   hitch=1 (HK_HITCH) frame hitch log; hitch_ms=N (HK_HITCH_MS) threshold, default 60; hitch_files=N (HK_HITCH_FILES) files listed
//   preopen=1 (HK_PREOPEN) open preloaded files from RAM; userfast=1 (HK_USERFAST) temp save files written in place;
//   preload_se=1 (HK_PRELOAD_SE) sound effect WAVs in the preload cache; savefast=1 (HK_SAVEFAST) save slot RAM cache, lazy .bak, buffered saves
// Player build: the four improvements are on by default (hkport.cfg preopen=0 etc. turns one off for troubleshooting)
// and a key is written back only when it was set to 0; hitch/hitch_ms/hitch_files exist in the developer build only.
static const struct { const char *cfg, *env; } g_dbgkey[] = {
#if HK_DEVTOOLS
    {"hitch", "HK_HITCH"}, {"hitch_ms", "HK_HITCH_MS"}, {"hitch_files", "HK_HITCH_FILES"},
#endif
    {"preopen", "HK_PREOPEN"}, {"userfast", "HK_USERFAST"}, {"preload_se", "HK_PRELOAD_SE"}, {"savefast", "HK_SAVEFAST"}};
#define NDBGKEY ((int)(sizeof g_dbgkey / sizeof g_dbgkey[0]))

int hk_gameover_hiding(void);   // port/hk/gameover.c: the hidden load chain runs
extern int g_hk_last_slot;   // port/hk/gameover.c: slot last saved to / loaded from, 1..10
extern void (*hk_last_slot_cb)(void);
static void load_cfg(void) {
    pm_map_defaults(g_map);
    sw_recover(HK_CFG);   // a crash between the two renames of save_cfg leaves hkport.cfg.bak
    FILE *f = fopen(HK_CFG, "r");
    if (!f) return;
    char line[64];
    int ver = 0, nmap = 0, got[PM_NBTN] = {0};   // nmap: map_ lines read (a cfg without any keeps the current defaults); got: buttons the cfg sets
    while (fgets(line, sizeof line, f)) {
        char bk[16], ak[16];
        if (sscanf(line, "map_%15[a-z]=%15s", bk, ak) == 2 && ++nmap)
            for (int b = 0; b < PM_NBTN; b++) if (!strcmp(bk, pm_btn_key[b])) { got[b] = 1;
                for (int a = 0; a < PM_NACT; a++) if (!strcmp(ak, pm_act_key[a])) g_map[b] = a; } { char lv[8]; if (sscanf(line, "lang=%7s", lv) == 1) g_lang = !strcmp(lv, "pt"); } sscanf(line, "wide=%d", &g_wide); sscanf(line, "fps=%d", &g_fps); sscanf(line, "volume=%d", &g_vol); sscanf(line, "music=%d", &g_mus); sscanf(line, "last_slot=%d", &g_hk_last_slot);
                                     sscanf(line, "wide_native=%d", &g_wide_native); 
                                     sscanf(line, "present_thread=%d", &g_present_thread);
#if HK_DEVTOOLS
                                     sscanf(line, "prof=%d", &g_hk_prof); sscanf(line, "memlog=%d", &g_hk_memlog); sscanf(line, "handle_pages=%d", &g_h_maxpages); sscanf(line, "stress=%31s", g_stress_spec); sscanf(line, "rec=%d", &g_rec);
                { int hv; if (sscanf(line, "heaptrace=%d", &hv) == 1 && hv) setenv("HK_TRACE_HEAP", "1", 1); }
#endif
                                     sscanf(line, "menu_scale=%d", &g_pm_ui); sscanf(line, "map_version=%d", &ver);
                { int hv; if (sscanf(line, "heapfix=%d", &hv) == 1) setenv("HK_HEAPFIX", hv ? "1" : "0", 1); }
                { char kk[16]; int vv; if (sscanf(line, "%15[a-z_]=%d", kk, &vv) == 2) for (int k = 0; k < NDBGKEY; k++) if (!strcmp(kk, g_dbgkey[k].cfg)) { char t[16]; snprintf(t, sizeof t, "%d", vv); setenv(g_dbgkey[k].env, t, 1); } } }
    fclose(f);
#if HK_DEVTOOLS
    if (g_rec) g_hk_rec_path = HK_DATA_DIR "/rec.wav";
#endif
    // Before map_version 3 Select was the game menu (v1) or RESULTS (v2, now Start > Results); both become the tips &
    // controls card once. (A saved map_select=results no longer parses, so it already falls back to the default.)
    if (ver < 3 && g_map[PB_SELECT] == ACT_MENU) g_map[PB_SELECT] = ACT_TIPS;
    // Before map_version 4 ACT_BOMB sent Z and ACT_SWITCH sent C (labels swapped). Swap the two in a saved map so every
    // button keeps doing what it did; L, which sent Shift (no gameplay action in the game), becomes Turncoat bomb.
    // Only buttons the cfg sets: a button left at its (new) default is already right.
    if (nmap && ver < 4)
        for (int i = 0; i < PM_NBTN; i++) {
            if (!got[i]) continue;
            if (g_map[i] == ACT_BOMB) g_map[i] = ACT_SWITCH;
            else if (g_map[i] == ACT_SWITCH) g_map[i] = ACT_BOMB;
            else if (i == PB_L && g_map[i] == ACT_SHIFT) g_map[i] = ACT_BOMB;
        }
}
static void save_cfg(void) {
    FILE *f = fopen(HK_CFG ".tmp", "w");   // written whole or not at all (safe_write.h): tmp, then commit
    if (!f) return;
    fprintf(f, "lang=%s\nwide=%d\nfps=%d\nwide_native=%d\nvolume=%d\nmusic=%d\n", g_lang ? "pt" : "en", g_wide, g_fps, g_wide_native, g_vol, g_mus);
#if HK_DEVTOOLS
    fprintf(f, "present_thread=%d\nprof=%d\nmenu_scale=%d\n", g_present_thread, g_hk_prof, g_pm_ui);
    if (g_rec) fprintf(f, "rec=%d\n", g_rec);
#else
    if (g_pm_ui != 4) fprintf(f, "menu_scale=%d\n", g_pm_ui);   // only when the player changed it by hand
    if (g_present_thread != HK_PT_DEFAULT) fprintf(f, "present_thread=%d\n", g_present_thread);   // kept if set by hand (0 included)
#endif
    if (g_hk_last_slot >= 1 && g_hk_last_slot <= 10) fprintf(f, "last_slot=%d\n", g_hk_last_slot);
    fprintf(f, "map_version=4\n");
    for (int k = 0; k < NDBGKEY; k++) {
        const char *v = getenv(g_dbgkey[k].env);
        if (v && (HK_DEVTOOLS || !strcmp(v, "0"))) fprintf(f, "%s=%s\n", g_dbgkey[k].cfg, v);   // player build: only an explicit 0 is kept
    }
    if (getenv("HK_HEAPFIX") && !strcmp(getenv("HK_HEAPFIX"), "0")) fprintf(f, "heapfix=0\n");
    for (int b = 0; b < PM_NBTN; b++) fprintf(f, "map_%s=%s\n", pm_btn_key[b], pm_act_key[g_map[b]]);
    if (fclose(f) == 0) sw_commit(HK_CFG);
    else remove(HK_CFG ".tmp");
}
// Text on a g_hk_ws_w x 480 XRGB buffer with the port's bitmap font (cell 18).
static void draw_text(uint32_t *buf, int x, int y, const char *s, uint32_t color) {
    const int bw = g_hk_ws_w;
    const HkFont *f = &hk_fonts[1];
    for (; *s; s++) {
        uint8_t ch = (uint8_t)*s;
        const HkGlyph *g = &f->glyphs[(ch < 0x20 ? '?' : ch) - 0x20];
        for (int gy = 0; gy < f->cell; gy++)
            for (int gx = 0; gx < g->width; gx++)
                if (((f->rows[g->offset + gy] >> gx) & 1u) && x + gx >= 0 && x + gx < bw && y + gy >= 0 && y + gy < 480)
                    buf[(y + gy) * bw + x + gx] = color;
        x += g->advance;
    }
}


#if HK_DEVTOOLS
extern uint64_t g_hk_plat_upd_t, g_hk_plat_pres_t, g_hk_plat_pre_t, g_hk_plat_clr_t;
#define HK_PLAT_TIMES(t0, t1, t2) do { g_hk_plat_upd_t = (t1) - (t0); g_hk_plat_clr_t = (t2) - (t1); g_hk_plat_pres_t = SDL_GetPerformanceCounter() - (t2); \
    g_hk_plat_pre_t = g_plat_enter ? (t0) - g_plat_enter : 0; } while (0)
#else
#define HK_PLAT_TIMES(t0, t1, t2) ((void)0)
#endif
static uint64_t g_plat_enter;   // plat_present_xrgb entry tick (0 = not entered through it)
// port/hk/hitch.c; hitch log: this frame's SDL_UpdateTexture and SDL_RenderPresent time (ticks)
static void show_frame(const uint32_t *pixels, int width, int height, int pitch_bytes) {
    if (!g_ren) return;
    if (!g_tex) g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, HK_WS_MAX, 480);   // fixed size, never recreated
    SDL_Rect src = {0, 0, width, height};
    uint64_t t0 = SDL_GetPerformanceCounter();
    SDL_UpdateTexture(g_tex, &src, pixels, pitch_bytes);
    uint64_t t1 = SDL_GetPerformanceCounter();
    SDL_RenderClear(g_ren);
    SDL_RenderCopy(g_ren, g_tex, &src, NULL);
    uint64_t t2 = SDL_GetPerformanceCounter();
    SDL_RenderPresent(g_ren);
    HK_PLAT_TIMES(t0, t1, t2); g_plat_enter = 0;
}

// Menu overlays (pause_menu.h "Overlay"): 960x544 ARGB canvases shown 1:1 at the native output resolution over the
// (scaled) game frame, so the menu text is not filtered. 0 = pause menu / message box, 1 = RESULTS panel.
// Boot breadcrumbs (player build too): written synchronously so a death right after them still leaves the line in hklog.txt.
#define BOOT(...) do { rt_log(__VA_ARGS__); rt_log_flush(); } while (0)
static SDL_Texture *g_ovtex[2];
static uint32_t g_ovl[2][PM_OW * PM_OH];
static const char *g_loading_built;   // text overlay 0 holds for plat_show_loading; any other write to overlay 0 invalidates it
static void ov_upload(int i) {
    if (i == 0) g_loading_built = NULL;
    if (!g_ren) return;
    int fresh = 0;
    if (!g_ovtex[i]) {
        fresh = 1;
        g_ovtex[i] = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, PM_OW, PM_OH);
        if (!g_ovtex[i]) { BOOT("[boot] overlay %d texture failed: %s\n", i, SDL_GetError()); return; }
        BOOT("[boot] overlay %d texture ok\n", i);
        SDL_SetTextureBlendMode(g_ovtex[i], SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2, 0, 12)
        SDL_SetTextureScaleMode(g_ovtex[i], SDL_ScaleModeNearest);
#endif
    }
    // pm_overlay_build says which part of the buffer changed since the last build (= since the last upload): upload only that.
    if (!fresh && g_pm_dirty_ovl == g_ovl[i]) {
        const PmRect d = g_pm_dirty;
        if (d.x1 <= d.x0) return;   // identical to what the texture holds
        const SDL_Rect r = {d.x0, d.y0, d.x1 - d.x0, d.y1 - d.y0};
        SDL_UpdateTexture(g_ovtex[i], &r, &g_ovl[i][d.y0 * PM_OW + d.x0], PM_OW * 4);
        return;
    }
    SDL_UpdateTexture(g_ovtex[i], NULL, g_ovl[i], PM_OW * 4);
}
// The game frame as show_frame, then (ov >= 0) the frame darkened by dim/255 and overlay texture ov at the full 960x544
// output (the logical size is lifted for that). Main (game) thread only, like every SDL call here.
static void show_frame_ov(const uint32_t *pixels, int width, int height, int pitch_bytes, int ov, int dim) {
    if (!g_ren) return;
    if (!g_tex) g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, HK_WS_MAX, 480);
    SDL_Rect src = {0, 0, width, height};
    uint64_t t0 = SDL_GetPerformanceCounter();
    SDL_UpdateTexture(g_tex, &src, pixels, pitch_bytes);
    uint64_t t1 = SDL_GetPerformanceCounter();
    SDL_RenderClear(g_ren);
    SDL_RenderCopy(g_ren, g_tex, &src, NULL);
    if (ov >= 0) {
        SDL_RenderSetLogicalSize(g_ren, 0, 0);
        if (dim) {
            SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(g_ren, 0, 0, 0, (Uint8)dim);
            SDL_RenderFillRect(g_ren, NULL);
        }
        if (g_ovtex[ov]) SDL_RenderCopy(g_ren, g_ovtex[ov], NULL, NULL);
        apply_aspect();
    }
    uint64_t t2 = SDL_GetPerformanceCounter();
    SDL_RenderPresent(g_ren);
    HK_PLAT_TIMES(t0, t1, t2); g_plat_enter = 0;
}

// The last frame shown (plat_show_xrgb): the background of the message box.
static const uint32_t *g_last_px;
static int g_last_w, g_last_h, g_last_pitch;
extern uint64_t g_pause_qpc;
// MessageBoxA (port/win32/user32.c): the game asks with MB_OK (0x40F1EB) and MB_OKCANCEL (0x40F2A0, answer == IDOK:
// "Stop this game ?" from File > Stop / F4, "Delete this game ?" from the Delete button of GAME DATA 1500). The text
// is drawn over the darkened last frame; Cross = OK / Yes, Circle = Cancel / No. Other button sets get no answer (0).
// The game's clocks skip the time spent here, as on the pause screen.
static int msgbox_loop(const char *title, const char *text, unsigned type) {
    const unsigned t = type & 15u;
    if ((t != 0 && t != 1 && t != 3 && t != 4) || !g_last_px || !g_ren) return 0;
    // Same look as the pause menu: the title screen's menu box, the game's font, the answers with button symbols.
    const int two = t == 1 || t >= 3;   // two answers
    const char *l1 = t == 3 || t == 4 ? hk_tr(S_YES) : "OK", *l2 = t == 3 || t == 4 ? hk_tr(S_NO) : hk_tr(S_CANCEL);
    pm_overlay_msgbox(g_ovl[0], title, text, l1, two ? l2 : NULL);
    ov_upload(0);
    uint64_t t0 = SDL_GetPerformanceCounter();
    SceCtrlData pad;
    int armed = 0, r = 0;
    for (;;) {
        show_frame_ov(g_last_px, g_last_w, g_last_h, g_last_pitch, 0, 192);   // the last frame at 25% brightness
        sceCtrlPeekBufferPositive(0, &pad, 1);
        const int x = (pad.buttons & SCE_CTRL_CROSS) != 0, o = (pad.buttons & SCE_CTRL_CIRCLE) != 0;
        if (!armed) { armed = !x && !o; }   // the press that opened the box does not answer it
        else if (x) { r = t >= 3 ? 6 : 1; break; }   // IDYES / IDOK
        else if (o) { r = t == 4 ? 7 : t == 3 ? 2 : t == 1 ? 2 : 1; break; }   // IDNO / IDCANCEL (MB_OK: IDOK)
        SDL_Delay(16);
    }
    while (sceCtrlPeekBufferPositive(0, &pad, 1) >= 0 && (pad.buttons & (SCE_CTRL_CROSS | SCE_CTRL_CIRCLE))) SDL_Delay(16);
    g_pause_qpc += SDL_GetPerformanceCounter() - t0;
    return r;
}

// Start pauses: the last frame is shown darkened with a pause sign until Start is pressed again. The game's
// clocks skip the paused time (g_pause_qpc, read by w32_qpc/w32_tick_ms) so it does not try to catch up.
extern uint64_t g_pause_qpc;
// Select on the pause screen leaves it and opens the game's menu bar (File: save / stop / exit, Option: Key and Game
// Configuration, About): plat_vk_down reports F10 for a few frames, which USER32 turns into the game's right click
// (port/hk/dialog.c hk_menu_key). In a stage Select (default map) shows the tips & controls card instead (plat_show_xrgb); RESULTS is a pause menu item;
// the menu bar is Start > Game menu there, or the "Game menu" action if the player assigns it to a button. Outside
// a stage (title, no counters) Select still opens the menu bar directly; Exit is in its File menu.
int hk_results_read(uint32_t out[13]);   // port/hk/results.c
int hk_gameover_ask_pending(void);       // port/hk/gameover.c: "Continue from the last save?" after GAME OVER
void hk_gameover_answer(int yes);
int hk_gameover_key(int vk);
static unsigned g_menu_until;
extern int g_hk_in_stage;  // port/hk/wide.c
int hk_dlg_active(void);   // port/hk/dialog.c (also declared below)
// Cheats (pause screen): the game's own Secret Option (dialog 1490, typed H D K on a
// keyboard) has "Unrivaled Mode": while it is on, 'u' toggles invincibility and 'm' adds money. Its checkbox 1417
// is stored at 0x4AC84C (0x4154F5 / 0x41555D). L sets that flag; R and Circle queue 'U' and 'M' presses that are
// delivered to the game after the pause, a few frames each (plat_vk_down).
#define HK_UNRIVALED 0x4AC84Cu
#define HK_INVINCIBLE 0x47BCD8u   // starts at 1 in .data; each 'u' XORs it (0x455138), effective with Unrivaled Mode
static char g_keyq[32];
static int g_keyq_n;
static unsigned g_keyq_at;
static void keyq_push(char k) { if (g_keyq_n < (int)sizeof g_keyq) g_keyq[g_keyq_n++] = k; }
static int keyq_down(int vk) { extern unsigned g_frame; return g_keyq_n && g_keyq[0] == vk && g_frame >= g_keyq_at && g_frame < g_keyq_at + 3; }
static void keyq_step(void) {   // once per presented frame: 3 frames down, 3 frames up, next key
    extern unsigned g_frame;
    if (g_keyq_n && g_frame >= g_keyq_at + 6) { memmove(g_keyq, g_keyq + 1, (size_t)--g_keyq_n); g_keyq_at = g_frame + 1; }
}
// Pause menu (Start): drawn by pause_menu.h (previewed on PC with port/vita/pause_preview.c). D-pad Up/Down moves,
// Cross selects or toggles, Left/Right changes a value, Circle goes back, Start resumes from anywhere, Select opens the
// game's menu bar directly. Save game queues F2 and Quit game queues Esc (the game's own commands), delivered after
// the pause like the cheat keys.
static int g_opt_mode;   // 1: opened from the title menu's OPTIONS item: starts on Settings, Back / Circle / Start leave
static void pause_loop(const uint32_t *pixels, int width, int height, int pitch_bytes) {
    static int started_native = -1;
    if (started_native < 0) started_native = g_wide_native;
    uint64_t t0 = SDL_GetPerformanceCounter();
    typedef struct { PmScreenArg a; int map[PM_NBTN]; } PmKey;
    static PmKey shown_key;   // what g_ovl[0] currently holds (the menu is redrawn only when it changes)
    memset(&shown_key, 0xff, sizeof shown_key);
    SceCtrlData pad;
    unsigned prev = 0xffffffffu;   // buttons of the previous poll: everything starts "held" so the opening press does nothing
    int capture = 0, released = 0, screen = g_opt_mode ? PM_SETTINGS : PM_MAIN, sel = 0, money = 0, amt = 1, confirm = 0, inv = 0, action = 0;   // action: 1 save, 2 game menu, 3 quit
    extern unsigned g_frame;
    uint32_t res_v[13] = {0};
    for (;;) {
        PmState st = {g_wide, g_fps, g_wide_native, started_native, g_vol, g_mus, 0, 0, amt, g_hk_in_stage, g_map, capture, g_lang, g_lang_ok, g_started_lang};
        st.cheats = *(uint32_t *)(g_mem + HK_UNRIVALED) != 0;
        st.inv_on = (*(uint32_t *)(g_mem + HK_INVINCIBLE) != 0) ^ inv;
        int ids[16], n = pm_rows(screen, &st, ids);
        if (sel >= n) sel = n - 1;
        PmKey cur;
        memset(&cur, 0, sizeof cur);
        cur.a.screen = screen; cur.a.sel = sel; cur.a.st = st;
        memcpy(cur.map, g_map, sizeof cur.map);
        if (confirm) {   // Cheats > Add money: "Add money xN?" Yes (Cross) / No (Circle) in the pause menu's message box
            if (shown_key.a.screen != -2) {
                char q[64];
                snprintf(q, sizeof q, hk_tr(S_M_CONF), amt);
                pm_overlay_msgbox(g_ovl[0], hk_tr(S_CHEATS_T), q, hk_tr(S_YES), hk_tr(S_NO));
                ov_upload(0);
                memset(&shown_key, 0, sizeof shown_key);
                shown_key.a.screen = -2;
            }
        } else if (memcmp(&cur, &shown_key, sizeof cur)) {
            if (screen == PM_RESULTS) pm_overlay_results_now(g_ovl[0], res_v);
            else pm_overlay_screen(g_ovl[0], screen, sel, &st);
            ov_upload(0);
            shown_key = cur;
        }
        show_frame_ov(pixels, width, height, pitch_bytes, 0, 128);   // the frame at 50% brightness under the menu
        sceCtrlPeekBufferPositive(0, &pad, 1);
        unsigned b = pad.buttons, press = b & ~prev;
        prev = b;
        if (capture) {   // Controls: the next face/shoulder button pressed gets the action; D-pad cancels
            for (int k = 0; k < PM_NBTN; k++) if (press & g_btn_mask[k]) { pm_map_assign(g_map, k, capture); capture = 0; save_cfg(); break; }
            if (press & (SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) capture = 0;
            if (!(b & SCE_CTRL_START)) released = 1;
            else if (released) break;
            SDL_Delay(16);
            continue;
        }
        if (confirm) {
            if (press & SCE_CTRL_CROSS) { money += amt; if (money > 20) money = 20; confirm = 0; }   // applied when the menu closes, like the other cheat keys
            else if (press & SCE_CTRL_CIRCLE) confirm = 0;
            if (!(b & SCE_CTRL_START)) released = 1;
            else if (released) break;
            SDL_Delay(16);
            continue;
        }
        int dx = ((press & (SCE_CTRL_RTRIGGER | SCE_CTRL_RIGHT)) != 0) - ((press & (SCE_CTRL_LTRIGGER | SCE_CTRL_LEFT)) != 0);
        if (press & SCE_CTRL_UP) sel = (sel + n - 1) % n;
        if (press & SCE_CTRL_DOWN) sel = (sel + 1) % n;
        int cross = (press & SCE_CTRL_CROSS) != 0, id = ids[sel], step = dx ? dx : cross;
        if (step) switch (id) {
        case R_CONTINUE: if (cross) action = -1; break;
        case R_SAVE: if (cross && g_hk_in_stage) action = 1; break;
        case R_CHEATS: if (cross) { screen = PM_CHEATS; sel = 0; } break;
        case R_SETTINGS: if (cross) { screen = PM_SETTINGS; sel = 0; } break;
        case R_RESULTS: if (cross && g_hk_in_stage && hk_results_read(res_v)) { screen = PM_RESULTS; sel = 0; } break;   // frozen counters while paused
        case R_LOAD: if (cross) action = 4; break;
        case R_LANG: if (step) { g_lang = !g_lang; g_lang_ok = lang_files_ok(); save_cfg(); } break;   // applies on restart
        case R_GAMEMENU: if (cross) action = 2; break;
        case R_QUIT: if (cross) action = 3; break;
        case R_BACK: if (cross) { if (g_opt_mode && screen == PM_SETTINGS) action = -1; else if (screen == PM_CONTROLS) { sel = 5; screen = PM_SETTINGS; } else { sel = screen == PM_CHEATS ? 3 : screen == PM_RESULTS ? 5 : 4; screen = PM_MAIN; } } break;
        case R_CONTROLS: if (cross) { screen = PM_CONTROLS; sel = 0; } break;
        case R_RESET: if (cross) { pm_map_defaults(g_map); save_cfg(); } break;
        case R_CHEAT_ON: *(uint32_t *)(g_mem + HK_UNRIVALED) = !st.cheats; break;
        case R_INV: inv = !inv; break;
        case R_MONEY: if (dx) { amt += dx; if (amt < 1) amt = 20; if (amt > 20) amt = 1; } else if (cross && money < 20) confirm = 1; break;
        case R_SCREEN: g_wide = !g_wide; apply_aspect(); save_cfg(); break;
        case R_NATIVE: g_wide_native = !g_wide_native; save_cfg(); break;   // surfaces are created at startup: applies on restart
        case R_FPS: g_fps = !g_fps; save_cfg(); break;
        case R_VOL: g_vol += step > 0 ? 1 : -1; if (!dx) g_vol = g_vol % 10 + 1; apply_volume(); save_cfg(); break;
        case R_MUS: g_mus += step > 0 ? 1 : -1; if (!dx) g_mus = g_mus % 10 + 1; apply_volume(); save_cfg(); break;
        default: if (cross && id >= R_ACT0) capture = id - R_ACT0 + 1; break;
        }
        if (press & SCE_CTRL_CIRCLE) { if (screen == PM_MAIN || (g_opt_mode && screen == PM_SETTINGS)) action = -1; else if (screen == PM_CONTROLS) { sel = 5; screen = PM_SETTINGS; } else { sel = screen == PM_CHEATS ? 3 : screen == PM_RESULTS ? 5 : 4; screen = PM_MAIN; } }
        if (press & SCE_CTRL_SELECT) action = 2;
        if (action) break;
        if (!(b & SCE_CTRL_START)) released = 1;
        else if (released) break;
        SDL_Delay(16);
    }
    if (action == 2) g_menu_until = g_frame + 3;
    if (*(uint32_t *)(g_mem + HK_UNRIVALED)) {
        if (inv) keyq_push('U');
        while (money-- > 0) keyq_push('M');
    }
    if (action == 1) keyq_push(0x71);   // F2: the game's Save This Game
    if (action == 4) { void hk_load_game_request(void); hk_load_game_request(); }   // title: File > Continue opens dialog 1500; in a stage: Stop This Game first, then Continue (port/hk/dialog.c)
    if (action == 3) keyq_push(0x1B);   // Esc: the game's exit prompt
    g_keyq_at = g_frame + 2;
    while (sceCtrlPeekBufferPositive(0, &pad, 1) >= 0 && (pad.buttons & (SCE_CTRL_START | SCE_CTRL_SELECT | SCE_CTRL_CROSS | SCE_CTRL_CIRCLE))) SDL_Delay(16);
    g_pause_qpc += SDL_GetPerformanceCounter() - t0;
}

static int g_start_was;
#if HK_DEVTOOLS
#define HK_DLOG(...) rt_dlog(__VA_ARGS__)
#else
#define HK_DLOG(...) ((void)0)
#endif
// The title menu's OPTIONS item (NETWORK in the original; the port has no network mode): the game's DialogBoxParamA
// for the network dialog 1488 is answered with Cancel and this opens the pause menu's Settings page instead
// (port/hk/dialog.c). Runs on the game thread while the title frame is still on screen; Back / Circle / Start return.
void hk_title_options(void) {
    const uint32_t *hk_dd_snapshot(int *w, int *h, int *pitch_bytes);
    int w = 0, h = 0, pitch = 0;
    const uint32_t *px = g_ren ? hk_dd_snapshot(&w, &h, &pitch) : NULL;   // own frame: g_last_px is not kept with present_thread=3
    HK_DLOG("[opt] reached: present_thread=%d frame=%s ren=%d\n", g_present_thread, px ? "ok" : "none", g_ren != NULL);
    if (!px) return;
    g_opt_mode = 1;
    HK_DLOG("[opt] settings page opened\n");
    pause_loop(px, w, h, pitch);
    HK_DLOG("[opt] settings page closed\n");
    g_opt_mode = 0;
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(0, &pad, 1);
    g_start_was = (pad.buttons & SCE_CTRL_START) != 0;
}
static int tips_held(void);
// Game thread, once per frame when presenting through the present thread: 1 = Start was just pressed, so the
// caller drains the thread and presents synchronously (plat_present_xrgb opens the pause screen).
int plat_poll_main(void) {
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(0, &pad, 1);
    if ((pad.buttons & SCE_CTRL_START) && !g_start_was) return 1;
    g_start_was = (pad.buttons & SCE_CTRL_START) != 0;
    if (g_present_thread == 3 && tips_held()) return 1;   // the tips card needs game state: synchronous frame
    keyq_step();
    return 0;
}
// Default 3 (asynchronous present, see ddraw.c pa_thread). An early run with mode 1 closed the game when stage A0
// started (2026-11-21); mode 3 has been stable since. hkport.cfg present_thread=0 turns the worker off.
int plat_present_threaded(void) { return g_present_thread; }
void plat_present_xrgb(uint32_t *pixels, int width, int height, int pitch_bytes) {
    g_plat_enter = SDL_GetPerformanceCounter();
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(0, &pad, 1);
    int start = (pad.buttons & SCE_CTRL_START) != 0;
    if (start && !g_start_was) { pause_loop(pixels, width, height, pitch_bytes); start = 0; }
    g_start_was = start;
    keyq_step();
    plat_show_xrgb(pixels, width, height, pitch_bytes);
    if (hk_gameover_ask_pending()) hk_gameover_answer(msgbox_loop("GAME OVER", hk_tr(S_GO_ASK), 4) == 6);   // Cross = Yes, Circle = No
}
static int tips_held(void) {   // the tips button (Select by default) is held in a stage, no dialog or menu bar open
    if (!g_hk_in_stage || hk_dlg_active()) return 0;
    unsigned m = 0;
    for (int i = 0; i < PM_NBTN; i++) if (g_map[i] == ACT_TIPS) m |= g_btn_mask[i];
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(0, &pad, 1);
    return m && (pad.buttons & m);
}
// The game's own hint texts (string resources of the running EXE: English, or Portuguese with the PT-BR EXE), shown
// one at a time. %s is the key the game would name; the port fills the button mapped to the action it describes.
int w32_load_string(uint32_t id, char *out, size_t cap);   // port/win32/kernel32.c
static void tips_load(int page, int npages_out[1], PmTipsArg *a) {
    static const struct { int id, act; } tips[] = {{500, ACT_BOMB}, {504, 0}, {506, 0}, {508, ACT_SWITCH}, {512, ACT_BOMB}, {600, 0}};
    int n = 0, want = page < 0 ? 0 : page;
    npages_out[0] = 0;
    for (size_t i = 0; i < sizeof tips / sizeof *tips; i++) {
        char raw[300];
        if (w32_load_string((uint32_t)tips[i].id, raw, sizeof raw) <= 0) continue;
        if (n == want) pm_tip_fill(a->tip, sizeof a->tip, raw, g_map, tips[i].act);
        n++;
    }
    npages_out[0] = n;
}
static void fps_overlay(uint32_t *pixels, int width, int height, int pitch_bytes) {
    // FPS: frames shown in the last full second (the game targets 25).
    static uint64_t t_sec;
    static int count, fps;
    uint64_t now = SDL_GetPerformanceCounter(), hz = SDL_GetPerformanceFrequency();
    count++;
    if (!t_sec) t_sec = now;
    if (now - t_sec >= hz) { fps = (int)((uint64_t)count * hz / (now - t_sec)); count = 0; t_sec = now; }
    if (g_fps && width == g_hk_ws_w && height <= 480 && pitch_bytes == width * 4) {   // drawn into the frame in place
        uint32_t *buf = pixels;
        char txt[16];
        snprintf(txt, sizeof txt, "%d FPS", fps);
        for (int y = 2; y < 22; y++)
            for (int x = 2; x < 74; x++) buf[y * width + x] = (buf[y * width + x] >> 2) & 0x3f3f3fu;
        draw_text(buf, 6, 3, txt, fps >= 24 ? 0x80ff80u : 0xffff60u);
    }
}
void plat_show_loading(uint32_t *pixels, int width, int height, int pitch_bytes) {
    const char *txt = hk_tr(S_LOADING);
    if (g_loading_built != txt) { pm_overlay_msgbox(g_ovl[0], "", txt, NULL, NULL); ov_upload(0); g_loading_built = txt; }   // text pointer: a language change rebuilds
    show_frame_ov(pixels, width, height, pitch_bytes, 0, 160);
}
// present_thread=3: runs on the present worker; no game state (no tips, no dialogs, no g_last_* for the pause screen).
void plat_present_async(uint32_t *pixels, int width, int height, int pitch_bytes) {
    fps_overlay(pixels, width, height, pitch_bytes);
    show_frame(pixels, width, height, pitch_bytes);
}
void plat_show_xrgb(uint32_t *pixels, int width, int height, int pitch_bytes) {
    static int first_frame;
    if (!first_frame) { first_frame = 1; BOOT("[boot] title loop: first frame %dx%d\n", width, height); }
    fps_overlay(pixels, width, height, pitch_bytes);
    g_last_px = pixels; g_last_w = width; g_last_h = height; g_last_pitch = pitch_bytes;
    // Tips & controls card while its button (Select by default) is held in a stage: D-pad Left/Right page through the tips (movement is suppressed meanwhile, see plat_vk_down).
    static unsigned tips_prev = 0xffffffffu;
    static int tips_page = 0, tips_shown = 0;
    static PmTipsArg tips_key;
    // The game's dialogs and menu bar: the pause menu's style at 960x544 (dlg_menu.h), rebuilt only when the view changes.
    // Dialogs run on the game thread (present_thread=3 presents synchronously while one is up, see present_primary).
    static HkDlgView dv;
    static uint64_t dv_hash;
    static int dv_valid;
    uint64_t pt0 = SDL_GetPerformanceCounter();
    if (width <= HK_WS_MAX && height <= 480 && hk_dlg_view(&dv)) {
        { static int once; if (!once) { once = 1; BOOT("[boot] first dialog view kind %d\n", (int)dv.kind); } }
        const uint64_t hh = hk_dlgview_hash(&dv);
        const uint64_t pt1 = SDL_GetPerformanceCounter();
        int rebuilt = 0;
        if (!dv_valid || hh != dv_hash) { pm_overlay_dlg(g_ovl[1], &dv); ov_upload(1); dv_hash = hh; dv_valid = 1; rebuilt = 1; }
        const uint64_t pt2 = SDL_GetPerformanceCounter();
        show_frame_ov(pixels, width, height, pitch_bytes, 1, dv.kind == DV_MENU ? 96 : 128);
#if HK_DEVTOOLS
        {   // dev: one line per second with the per-frame cost of the dialog view (hklog.txt)
            static uint64_t t_win, a_view, a_build, a_show; static unsigned n, nb;
            const uint64_t t3 = SDL_GetPerformanceCounter(), hz = SDL_GetPerformanceFrequency();
            a_view += pt1 - pt0; a_build += pt2 - pt1; a_show += t3 - pt2; n++; nb += (unsigned)rebuilt;
            if (!t_win) t_win = t3;
            if (t3 - t_win >= hz) {
                const double ms = 1e3 / (double)hz;
                HK_DLOG("[dlgperf] %u shows, %u rebuilds in %.2f s: view %.2f ms, rebuild+upload %.2f ms (per rebuild), show_frame_ov %.2f ms (per show)\n",
                        n, nb, (double)(t3 - t_win) / (double)hz, (double)a_view * ms / n, nb ? (double)a_build * ms / nb : 0.0, (double)a_show * ms / n);
                t_win = t3; a_view = a_build = a_show = 0; n = nb = 0;
            }
        }
#endif
        tips_shown = 0; tips_prev = 0xffffffffu;
        return;
    }
    dv_valid = 0;
    if (width <= HK_WS_MAX && height <= 480 && tips_held()) {
        SceCtrlData pad;
        sceCtrlPeekBufferPositive(0, &pad, 1);
        unsigned press = pad.buttons & ~tips_prev;
        tips_prev = pad.buttons;
        int np = 0;
        PmTipsArg cur;
        memset(&cur, 0, sizeof cur);
        tips_load(tips_page, &np, &cur);
        if (np > 0) {
            tips_page = (tips_page + np + ((press & SCE_CTRL_RIGHT) != 0) - ((press & SCE_CTRL_LEFT) != 0)) % np;
            memset(&cur, 0, sizeof cur);
            tips_load(tips_page, &np, &cur);
        }
        cur.map = g_map; cur.page = tips_page; cur.npages = np;
        static int shown_lang = -1, shown_ui = -1, shown_map[PM_NBTN];
        if (!tips_shown || memcmp(cur.tip, tips_key.tip, sizeof cur.tip) || cur.page != tips_key.page || shown_lang != g_lang || shown_ui != g_pm_ui || memcmp(shown_map, g_map, sizeof shown_map)) {
            pm_overlay_tips(g_ovl[1], &cur);
            ov_upload(1);
            tips_key = cur; shown_lang = g_lang; shown_ui = g_pm_ui; memcpy(shown_map, g_map, sizeof shown_map);
            tips_shown = 1;
        }
        show_frame_ov(pixels, width, height, pitch_bytes, 1, 0);
        return;
    }
    tips_prev = 0xffffffffu; tips_shown = 0;
    show_frame(pixels, width, height, pitch_bytes);
}

// Pad mapping. Key tests in stage A0 (2026-11-20) show X shoots, A or Up jumps, S crouches; the
// game's Key Configuration dialog (resource 1492) lists Shoot & Bomb, Turncoat Bomb, Switch & Rotation, Jump and
// Crouch. Menus confirm with Enter or space and go back with Z/X/C (0x4117D4), checking confirm first, so Cross
// sends X + space: it shoots in play and confirms in menus. Square = A (jump), down = arrow + S (crouch),
// Circle = Z (Switch & Rotation), Triangle = C (Turncoat Bomb), as the Key Configuration defaults show; L = Shift,
// R = Ctrl, Select = Esc. Start is the port's own pause (plat_present_xrgb), not a game key. In the game's dialogs
// (port/hk/dialog.c) Cross presses the focused control, Triangle presses OK and Circle or Select cancel.
int hk_dlg_active(void);   // port/hk/dialog.c
int plat_vk_down(int vk) {
#if HK_DEVTOOLS
    if (hk_stress_key(vk)) return 1;
#endif
    static SceCtrlData pad;
    static uint32_t last_frame = 0xffffffffu, tap_at = 0xffffffffu;
    extern unsigned g_frame;
    if (last_frame != g_frame) {   // one read per presented frame is enough for GetKeyState polling
        static int init;
        if (!init) { sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG); init = 1; }
        sceCtrlPeekBufferPositive(0, &pad, 1);
        last_frame = g_frame;
        // The game keeps the character prone after Down until Down is pressed AGAIN (or another direction). To be
        // prone only while Down is held, when Down is released with no other direction held the port reports Down
        // released for 2 frames and then presses it for 2 frames: that press stands him up without a jump (checked
        // on PC with HK_KEYS: Down 30 frames, released 2, Down 2, then standing, no jump in between). Cancelled if
        // Down or another direction is pressed first, or a dialog or menu opens.
        int d_now = (pad.buttons & SCE_CTRL_DOWN) || pad.ly > 192;
        int other = (pad.buttons & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT | SCE_CTRL_UP)) || pad.lx < 64 || pad.lx > 192 || pad.ly < 64;
        static int d_prev, skip_release;
        if (d_prev && !d_now) {
            if (skip_release) skip_release = 0;   // his real re-press already stood him up
            else if (!other && !hk_dlg_active() && g_frame >= g_menu_until && g_hk_in_stage) tap_at = g_frame + 2;   // stages only: in menus the tap moved the cursor twice
        } else if (d_now && !d_prev && tap_at != 0xffffffffu && g_frame < tap_at) {
            tap_at = 0xffffffffu;   // pressed again before the synthetic press: the real press does the job
            skip_release = 1;
        } else if (other || hk_dlg_active()) tap_at = 0xffffffffu;
        d_prev = d_now;
    }
    if (hk_gameover_hiding()) return hk_gameover_key(vk);   // hidden load chain after GAME OVER: only its injected keys, never the player's
    const int dlg = hk_dlg_active();
    int tap = !dlg && tap_at != 0xffffffffu && g_frame >= tap_at && g_frame < tap_at + 2;   // never in dialogs
    uint32_t b = pad.buttons;
    if (tips_held()) b &= ~(unsigned)(SCE_CTRL_LEFT | SCE_CTRL_RIGHT);   // D-pad Left/Right page the tips card: no movement meanwhile
    int down = (b & SCE_CTRL_DOWN) || pad.ly > 192 || tap;
    // Menus, dialogs and the title screen use fixed buttons: Cross confirms (space), Circle cancels (Z), Triangle = C
    // (default in dialogs), Select = Esc in dialogs and the menu bar otherwise. Only in a stage (no dialog) the
    // gameplay map applies, so jump on Cross never also sends space; there Select is the tips card (no key, see plat_show_xrgb).
    int menu = !g_hk_in_stage || hk_dlg_active();
    unsigned m[PM_NACT] = {0};
    for (int i = 0; i < PM_NBTN; i++) m[g_map[i]] |= g_btn_mask[i];
    switch (vk) {
    case 0x25: return (b & SCE_CTRL_LEFT) || pad.lx < 64;
    case 0x26: return (b & SCE_CTRL_UP) || pad.ly < 64;
    case 0x27: return (b & SCE_CTRL_RIGHT) || pad.lx > 192;
    case 0x28: return down || hk_gameover_key(0x28);
    case 0x0D: return hk_gameover_key(0x0D);   // injected Enter: OK of "Stop this game ?" after GAME OVER (port/hk/gameover.c)
    case 'S': return down;
    // Rapid fire (held): the game's shoot key pressed 2 frames and released 2 (about 6 shots per second at 25 fps).
    case 'X': return !menu && ((b & m[ACT_SHOOT]) || ((b & m[ACT_RAPID]) && (g_frame & 3u) < 2u));
    case 0x20: return (menu && (b & SCE_CTRL_CROSS)) || hk_gameover_key(0x20);
    case 'U': case 'M': case 0x71: return keyq_down(vk);   // cheats, Save (F2) from the pause menu
    case 'A': return !menu && (b & m[ACT_JUMP]);
    // In dialogs Circle is Esc, not Z: the GAME DATA loop (1500, 0x4328CD) reads the game's own input object, where
    // Z/X/C/Shift/Ctrl are confirm ([+8], 0x4118EF..0x411921 -> 0x4028FB -> WM_COMMAND 0x9CA4) and only Esc is
    // cancel ([+0x10], 0x4118A1 -> 0x40216C -> 0x9CA5); the port's dialogs treat Z and Esc alike.
    // The game's key table (0x47B548, read at 0x414C2A): Z = Switch & Rotation (board/leave vehicles, tip 508), C = Turncoat
    // bomb (tips 500/512). Before map_version 4 these two actions were labelled the other way round.
    case 'Z': return menu ? (!dlg && (b & SCE_CTRL_CIRCLE)) : (b & m[ACT_SWITCH]) != 0;
    case 'C': return menu ? (b & SCE_CTRL_TRIANGLE) != 0 : (b & m[ACT_BOMB]) != 0;
    case 0x1B: return (dlg && (b & (SCE_CTRL_SELECT | SCE_CTRL_CIRCLE))) || keyq_down(0x1B);   // Esc: also Quit game from the pause menu
    // GAME DATA (1500) Rename / Delete buttons (port/hk/dialog.c: R and D in that dialog): Square and L.
    case 'R': return dlg && (b & SCE_CTRL_SQUARE);
    case 'D': return dlg && (b & SCE_CTRL_LTRIGGER);
    case 0x79: return g_frame < g_menu_until || (!hk_dlg_active() && (b & (menu ? SCE_CTRL_SELECT : m[ACT_MENU])));
    case 0x10: return !menu && (b & m[ACT_SHIFT]);
    case 0x11: return 0;
    default: return 0;
    }
}

void rt_platform_fatal(const char *msg) { (void)msg; }   // already in the log; nothing to show it with

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir(HK_DATA_DIR, 0777);
    sceIoMkdir(HK_DATA_DIR "/USER", 0777);   // saves live here; the builder does not ship it, so an update never overwrites them
    hk_dlg_set_pretty(1);
    BOOT("[boot] main\n");
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
    hk_last_slot_cb = save_cfg;
    apply_volume();
    hk_wide_set(g_wide_native ? 848 : 640);
    apply_aspect();
    BOOT("[boot] cfg loaded\n");
    rt_log("[vita] window %p renderer %p\n", (void *)g_win, (void *)g_ren);
    if (rt_init(HK_GUEST_MB << 20)) rt_fatal("guest memory: allocation of %u MB failed", HK_GUEST_MB);
    BOOT("[boot] guest memory ok\n");
    g_lang_ok = lang_files_ok();
    g_started_lang = g_lang;
    const char *exe = HK_DATA_DIR "/HDKWIN.EXE";
    if (g_lang && g_lang_ok) { exe = HK_LANG_DIR "/HDKWINBR.EXE"; w32_set_lang_overlay(HK_LANG_DIR); }
    int r = rt_load_exe(exe);
    if (r) rt_fatal("cannot load " HK_DATA_DIR "/HDKWIN.EXE (%d): copy your game files there", r);
    rt_fill_iat();
    { void hk_patch_data(void); hk_patch_data(); }
    rt_log("game loaded from %s\n", g_game_dir);
    BOOT("[boot] overlay init ok, entering the game\n");
#if HK_DEVTOOLS
    hk_stress_init(g_stress_spec);
#endif
    rt_run_entry();
    BOOT("[boot] rt_run_entry returned\n");
    SDL_Quit();
    return 0;
}
