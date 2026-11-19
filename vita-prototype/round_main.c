#include <stddef.h>
#include <stdint.h>

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>

#include "round_scene.h"

enum {
    CDRAM_ALIGNMENT = 256 * 1024,
    FRAME_BYTES = ROUND_PITCH * ROUND_HEIGHT * sizeof(uint32_t),
    FRAME_STORAGE = (FRAME_BYTES + CDRAM_ALIGNMENT - 1) / CDRAM_ALIGNMENT * CDRAM_ALIGNMENT,
};

int main(void)
{
    round_scene_load("ux0:data/HKVW00001/vista0.rgba");

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
    round_scene_draw(base, &round);

    uint32_t *back_buffer = (uint32_t *)((uint8_t *)base + FRAME_STORAGE);
    SceDisplayFrameBuf frame = {
        .size = sizeof(frame),
        .base = base,
        .pitch = ROUND_PITCH,
        .pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,
        .width = ROUND_WIDTH,
        .height = ROUND_HEIGHT,
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

        round_scene_draw(back_buffer, &round);
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
