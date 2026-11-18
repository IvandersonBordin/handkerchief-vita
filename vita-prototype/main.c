#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>

#include "sprite.h"

enum {
    SCREEN_WIDTH = 960,
    SCREEN_HEIGHT = 544,
    SCREEN_PITCH = 1024,
    CDRAM_ALIGNMENT = 256 * 1024,
    FRAME_BYTES = SCREEN_PITCH * SCREEN_HEIGHT * sizeof(uint32_t),
    FRAME_STORAGE = (FRAME_BYTES + CDRAM_ALIGNMENT - 1) / CDRAM_ALIGNMENT * CDRAM_ALIGNMENT,
    SPRITE_SIZE = 16,
    SPRITE_STEP = 4,
    SPRITE_FRAME_US = 250000,
};

static void draw_bars(uint32_t *pixels, uint32_t middle_color)
{
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        uint32_t *row = pixels + y * SCREEN_PITCH;
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            if (x < SCREEN_WIDTH / 3)
                row[x] = 0xff0000ff; /* Red in A8B8G8R8. */
            else if (x < 2 * SCREEN_WIDTH / 3)
                row[x] = middle_color;
            else
                row[x] = 0xffff0000; /* Blue. */
        }
        /* The display pitch includes pixels beyond the visible width. */
        for (int x = SCREEN_WIDTH; x < SCREEN_PITCH; ++x)
            row[x] = 0xff000000;
    }
}

static int move_axis(int position, uint32_t buttons, uint32_t negative,
                     uint32_t positive, int limit)
{
    position += SPRITE_STEP * (!!(buttons & positive) - !!(buttons & negative));
    if (position < 0)
        return 0;
    if (position > limit)
        return limit;
    return position;
}

int main(void)
{
    const SceUID block = sceKernelAllocMemBlock("smoke_frames",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 2 * FRAME_STORAGE, NULL);
    if (block < 0)
        return 1;

    void *base = NULL;
    int result = 1;
    if (sceKernelGetMemBlockBase(block, &base) < 0)
        goto cleanup;

    uint32_t *back_buffer = (uint32_t *)((uint8_t *)base + FRAME_STORAGE);
    const SpriteSurface screen = { SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_PITCH };
    const SpriteSurface sprite = { SPRITE_SIZE, SPRITE_SIZE, SPRITE_SIZE };
    uint32_t sprite_pixels[2][SPRITE_SIZE * SPRITE_SIZE];
    for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE; ++i) {
        sprite_pixels[0][i] = 0xffffffff; /* White. */
        sprite_pixels[1][i] = 0xffff00ff; /* Magenta. */
    }

    int sprite_x = (SCREEN_WIDTH - SPRITE_SIZE) / 2;
    int sprite_y = (SCREEN_HEIGHT - SPRITE_SIZE) / 2;
    uint32_t sprite_frame = 0;
    uint32_t middle_color = 0xff00ff00;
    draw_bars(base, middle_color);
    sprite_blit(base, screen, sprite_pixels[sprite_frame], sprite, sprite_x, sprite_y);

    SceDisplayFrameBuf frame = {
        .size = sizeof(frame),
        .base = base,
        .pitch = SCREEN_PITCH,
        .pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
    };
    if (sceDisplaySetFrameBuf(&frame, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0)
        goto cleanup;

    uint32_t previous_buttons = 0;
    int paused = 0;
    uint64_t active_time_us = 0;
    uint64_t previous_time_us = sceKernelGetSystemTimeWide();
    while (sceDisplayWaitVblankStart() >= 0) {
        const uint64_t now_us = sceKernelGetSystemTimeWide();
        /* Charge the interval to the state before reading this frame's input. */
        if (!paused && now_us >= previous_time_us)
            active_time_us += now_us - previous_time_us;
        previous_time_us = now_us;

        SceCtrlData pad = {0};
        const int samples = sceCtrlPeekBufferPositive(0, &pad, 1);
        if (samples < 0)
            break;
        if (samples == 0)
            continue;
        if (pad.buttons & SCE_CTRL_START) {
            printf("Active loop time: %" PRIu64 " us\n", active_time_us);
            result = 0;
            break;
        }

        const uint32_t pressed_buttons = pad.buttons & ~previous_buttons;
        previous_buttons = pad.buttons;
        if (pressed_buttons & SCE_CTRL_CIRCLE)
            paused = !paused;
        if (paused)
            continue;

        const int next_x = move_axis(sprite_x, pad.buttons, SCE_CTRL_LEFT,
                                     SCE_CTRL_RIGHT, SCREEN_WIDTH - SPRITE_SIZE);
        const int next_y = move_axis(sprite_y, pad.buttons, SCE_CTRL_UP,
                                     SCE_CTRL_DOWN, SCREEN_HEIGHT - SPRITE_SIZE);
        const uint32_t next_sprite_frame = sprite_animation_frame(active_time_us,
                                                                   SPRITE_FRAME_US, 2);
        const uint32_t next_middle_color = (pad.buttons & SCE_CTRL_CROSS)
            ? 0xff00ffff : 0xff00ff00;
        if (next_x == sprite_x && next_y == sprite_y && next_sprite_frame == sprite_frame
            && next_middle_color == middle_color)
            continue;

        draw_bars(back_buffer, next_middle_color);
        sprite_blit(back_buffer, screen, sprite_pixels[next_sprite_frame], sprite, next_x, next_y);

        /* The vblank at the start of the iteration released the old buffer. */
        uint32_t *old_front = frame.base;
        frame.base = back_buffer;
        if (sceDisplaySetFrameBuf(&frame, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0)
            break;
        back_buffer = old_front;
        sprite_x = next_x;
        sprite_y = next_y;
        sprite_frame = next_sprite_frame;
        middle_color = next_middle_color;
    }

    /* Let process teardown reclaim the block if the display still owns it. */
    if (sceDisplaySetFrameBuf(NULL, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0 ||
        sceDisplayWaitVblankStart() < 0)
        return 1;

cleanup:
    if (sceKernelFreeMemBlock(block) < 0)
        return 1;
    return result;
}
