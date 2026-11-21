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
