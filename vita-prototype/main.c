#include <stddef.h>
#include <stdint.h>

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>

enum {
    SCREEN_WIDTH = 960,
    SCREEN_HEIGHT = 544,
    SCREEN_PITCH = 1024,
    CDRAM_ALIGNMENT = 256 * 1024,
    FRAME_BYTES = SCREEN_PITCH * SCREEN_HEIGHT * sizeof(uint32_t),
    FRAME_STORAGE = (FRAME_BYTES + CDRAM_ALIGNMENT - 1) / CDRAM_ALIGNMENT * CDRAM_ALIGNMENT,
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

    /* Prepare both images before handing either one to the display. */
    void *pressed_frame = (uint8_t *)base + FRAME_STORAGE;
    draw_bars(base, 0xff00ff00);          /* Green. */
    draw_bars(pressed_frame, 0xff00ffff); /* Yellow. */

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

        void *next_frame = (pad.buttons & SCE_CTRL_CROSS) ? pressed_frame : base;
        if (next_frame != frame.base) {
            frame.base = next_frame;
            if (sceDisplaySetFrameBuf(&frame, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0)
                break;
        }
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
