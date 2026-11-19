#include <stddef.h>
#include <stdint.h>

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>

#include "demo_round.h"

enum {
    WIDTH = 960,
    HEIGHT = 544,
    PITCH = 1024,
    CDRAM_ALIGNMENT = 256 * 1024,
    FRAME_BYTES = PITCH * HEIGHT * sizeof(uint32_t),
    FRAME_STORAGE = (FRAME_BYTES + CDRAM_ALIGNMENT - 1) / CDRAM_ALIGNMENT * CDRAM_ALIGNMENT,
};

static void fill_rect(uint32_t *pixels, int left, int top, int right, int bottom,
                      uint32_t color)
{
    for (int y = top; y < bottom; ++y)
        for (int x = left; x < right; ++x)
            pixels[y * PITCH + x] = color;
}

static void draw_round(uint32_t *pixels, const DemoRound *round)
{
    fill_rect(pixels, 0, 0, PITCH, DEMO_FLOOR_Y,
              round->finished ? 0xff234d23 : 0xff302028);
    fill_rect(pixels, 0, DEMO_FLOOR_Y, PITCH, HEIGHT, 0xff524545);
    fill_rect(pixels, DEMO_OBSTACLE_LEFT, DEMO_OBSTACLE_TOP,
              DEMO_OBSTACLE_RIGHT, DEMO_FLOOR_Y, 0xff0088dd);
    fill_rect(pixels, DEMO_EXIT_X, DEMO_FLOOR_Y - 80,
              DEMO_EXIT_X + 4, DEMO_FLOOR_Y, 0xff55ef70);
    fill_rect(pixels, round->x, round->feet_y - DEMO_PLAYER_SIZE,
              round->x + DEMO_PLAYER_SIZE, round->feet_y, 0xffffffff);
}

int main(void)
{
    const SceUID block = sceKernelAllocMemBlock("round_frames",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 2 * FRAME_STORAGE, NULL);
    if (block < 0)
        return 1;

    void *base = NULL;
    int result = 1;
    if (sceKernelGetMemBlockBase(block, &base) < 0)
        goto cleanup;

    DemoRound round;
    demo_round_init(&round);
    draw_round(base, &round);

    uint32_t *back_buffer = (uint32_t *)((uint8_t *)base + FRAME_STORAGE);
    SceDisplayFrameBuf frame = {
        .size = sizeof(frame),
        .base = base,
        .pitch = PITCH,
        .pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,
        .width = WIDTH,
        .height = HEIGHT,
    };
    if (sceDisplaySetFrameBuf(&frame, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0)
        goto cleanup;

    while (sceDisplayWaitVblankStart() >= 0) {
        SceCtrlData pad = {0};
        const int samples = sceCtrlPeekBufferPositive(0, &pad, 1);
        if (samples < 0)
            break;
        if (samples == 0)
            continue;
        if (pad.buttons & SCE_CTRL_START) {
            result = 0;
            break;
        }

        const DemoRound previous = round;
        demo_round_step(&round, pad.buttons & SCE_CTRL_LEFT,
                        pad.buttons & SCE_CTRL_RIGHT, pad.buttons & SCE_CTRL_CROSS);
        if (round.x == previous.x && round.feet_y == previous.feet_y &&
            round.finished == previous.finished)
            continue;

        draw_round(back_buffer, &round);
        uint32_t *old_front = frame.base;
        frame.base = back_buffer;
        if (sceDisplaySetFrameBuf(&frame, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0)
            break;
        back_buffer = old_front;
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
