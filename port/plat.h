// Platform layer used by the Win32/DirectX emulation (PC: port/pc/, Vita: port/vita/).
// Interface derived from sh3vita port/src/plat.h (GPL-3.0).
#pragma once
#include <SDL.h>
#include <stdint.h>

void plat_show_window(void);
// Returns the Win32 button ID, or zero if the dialogue could not be shown.
int plat_message_box(const char *title, const char *text, unsigned type);
int plat_pump_events(void);   // 1 if the user asked to quit
int plat_width(void);         // game resolution (640x480)
int plat_height(void);
SDL_Window *plat_sdl_window(void);
// 1 while the Windows virtual key vk (VK_LEFT 0x25, 'Z' 0x5A, VK_RETURN 0x0D...) is held.
int plat_vk_down(int vk);
// Shows one 640x480 frame of 32-bit XRGB pixels (DirectDraw primary surface after palette conversion).
void plat_present_xrgb(const uint32_t *pixels, int width, int height, int pitch_bytes);

// Text entry for the game's EDIT controls (port/hk/dialog.c), two models:
// - a keyboard (PC): while plat_text_enable(1) is on, plat_text_typed hands out what was typed since the last call
//   (Latin-1 bytes; '\b' for each Backspace), and the dialog edits the control in place;
// - a system keyboard (Vita IME): plat_text_open shows it with the control's text and returns 1; plat_text_status
//   then returns 0 while it is up, 1 with the accepted text in out, or 2 when it was closed without accepting.
int plat_has_keyboard(void);
void plat_text_enable(int on);
int plat_text_typed(char *out, int cap);
int plat_text_open(const char *title, const char *text, int maxlen);
int plat_text_status(char *out, int cap);
