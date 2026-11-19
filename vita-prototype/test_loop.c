#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>

/* Screen storage rounded to 256 KiB for each of the two frames. */
static uint32_t frames[2 * 2304 * 1024 / sizeof(uint32_t)];
static uint32_t visible_pixels[1024 * 544];
static const uint32_t *front_buffer;
static unsigned time_reads;
static unsigned pad_reads;
static unsigned vblanks;
static unsigned display_calls;
static int freed;
static int fail_pad_read;
static int fail_get_base;
static int fail_detach;
enum { LOOP_VBLANKS = 14, TOTAL_VBLANKS = LOOP_VBLANKS + 1 };

int smoke_main(void);

SceUID sceKernelAllocMemBlock(const char *name, SceKernelMemBlockType type,
                             SceSize size, SceKernelAllocMemBlockOpt *opt)
{
    assert(name && type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW);
    assert(size == sizeof(frames) && !opt);
    return 1;
}

int sceKernelGetMemBlockBase(SceUID uid, void **base)
{
    assert(uid == 1);
    if (fail_get_base)
        return -1;
    *base = frames;
    return 0;
}

int sceKernelFreeMemBlock(SceUID uid)
{
    assert(uid == 1 && !freed && !fail_detach);
    if (fail_get_base)
        assert(display_calls == 0 && vblanks == 0);
    else if (fail_pad_read)
        assert(display_calls == 2 && vblanks == 2);
    else
        assert(display_calls == 8 && vblanks == TOTAL_VBLANKS);
    freed = 1;
    return 0;
}

int sceDisplaySetFrameBuf(const SceDisplayFrameBuf *frame, SceDisplaySetBufSync sync)
{
    static const struct {
        unsigned vblank;
        unsigned x;
        unsigned y;
        uint32_t middle;
        uint32_t sprite;
    } expected[] = {
        {  0, 472, 264, 0xff00ff00, 0xffffffff },
        {  7, 476, 264, 0xff00ffff, 0xffffffff },
        {  8, 476, 264, 0xff00ff00, 0xffffffff },
        {  9, 476, 264, 0xff00ffff, 0xffffffff },
        { 10, 476, 268, 0xff00ffff, 0xffff00ff },
        { 11, 472, 268, 0xff00ffff, 0xffffffff },
        { 12, 472, 264, 0xff00ffff, 0xffffffff },
    };

    assert(sync == SCE_DISPLAY_SETBUF_NEXTFRAME);
    if (front_buffer)
        assert(memcmp(front_buffer, visible_pixels, sizeof(visible_pixels)) == 0);
    if (frame) {
        assert(!fail_pad_read || display_calls == 0);
        assert(display_calls < sizeof(expected) / sizeof(expected[0]));
        assert(frame->base == frames + (display_calls % 2) * (sizeof(frames) / sizeof(frames[0]) / 2));
        assert(vblanks == expected[display_calls].vblank);
        assert(frame->size == sizeof(*frame));
        assert(frame->width == 960 && frame->height == 544);
        assert(frame->pitch == 1024);
        assert(frame->pixelformat == SCE_DISPLAY_PIXELFORMAT_A8B8G8R8);
        const uint32_t *pixels = frame->base;
        const unsigned sprite_x = expected[display_calls].x;
        const unsigned sprite_y = expected[display_calls].y;
        for (unsigned y = 0; y < frame->height; ++y) {
            const uint32_t *row = pixels + y * frame->pitch;
            for (unsigned x = 0; x < frame->pitch; ++x) {
                uint32_t color = 0xff000000;
                if (x < 320)
                    color = 0xff0000ff;
                else if (x < 640)
                    color = expected[display_calls].middle;
                else if (x < 960)
                    color = 0xffff0000;
                if (x >= sprite_x && x < sprite_x + 16 && y >= sprite_y && y < sprite_y + 16)
                    color = expected[display_calls].sprite;
                assert(row[x] == color);
            }
        }
        memcpy(visible_pixels, pixels, sizeof(visible_pixels));
        front_buffer = pixels;
    } else if (fail_pad_read) {
        assert(display_calls == 1 && vblanks == 1);
    } else {
        assert(display_calls == sizeof(expected) / sizeof(expected[0]) && vblanks == LOOP_VBLANKS);
    }
    ++display_calls;
    if (!frame && fail_detach)
        return -1;
    return 0;
}

int sceDisplayWaitVblankStart(void)
{
    assert(vblanks < TOTAL_VBLANKS);
    ++vblanks;
    return 0;
}

SceInt64 sceKernelGetSystemTimeWide(void)
{
    /* Units of 10 ms: count 20 and 30 before the pause, then 70 after resuming. */
    static const SceInt64 readings[] = {
        100, 120, 110, 140, 1000, 2000, 3000, 4000, 4010, 4020, 4040, 4050, 4060, 4070, 5000,
    };
    assert(time_reads < sizeof(readings) / sizeof(readings[0]));
    return readings[time_reads++] * 10000;
}

int sceCtrlPeekBufferPositive(int port, SceCtrlData *pad, int count)
{
    static const uint32_t buttons[] = {
        0, 0, SCE_CTRL_CIRCLE, 0, SCE_CTRL_CIRCLE | SCE_CTRL_CROSS | SCE_CTRL_LEFT,
        SCE_CTRL_UP, SCE_CTRL_CIRCLE | SCE_CTRL_CROSS | SCE_CTRL_RIGHT, 0,
        SCE_CTRL_CROSS, SCE_CTRL_CROSS | SCE_CTRL_DOWN,
        SCE_CTRL_CROSS | SCE_CTRL_LEFT, SCE_CTRL_CROSS | SCE_CTRL_UP,
        SCE_CTRL_CIRCLE | SCE_CTRL_LEFT,
        SCE_CTRL_START | SCE_CTRL_CIRCLE | SCE_CTRL_CROSS,
    };
    assert(port == 0 && count == 1);
    assert(pad_reads < sizeof(buttons) / sizeof(buttons[0]));
    const unsigned sample = pad_reads++;
    if (fail_pad_read) {
        assert(sample == 0);
        return -1;
    }
    /* A missing sample while CIRCLE is held must preserve the pause. */
    if (sample == 3)
        return 0;
    pad->buttons = buttons[sample];
    return 1;
}

static void reset_observations(void)
{
    time_reads = 0;
    pad_reads = 0;
    vblanks = 0;
    display_calls = 0;
    freed = 0;
    front_buffer = NULL;
}

int main(void)
{
    assert(smoke_main() == 0);
    assert(time_reads == LOOP_VBLANKS + 1 && pad_reads == LOOP_VBLANKS && freed);

    reset_observations();
    fail_detach = 1;
    assert(smoke_main() == 1);
    assert(time_reads == LOOP_VBLANKS + 1 && pad_reads == LOOP_VBLANKS);
    assert(display_calls == 8 && vblanks == LOOP_VBLANKS && !freed);
    fail_detach = 0;

    reset_observations();
    fail_pad_read = 1;
    assert(smoke_main() == 1);
    assert(time_reads == 2 && pad_reads == 1 && freed);

    reset_observations();
    fail_pad_read = 0;
    fail_get_base = 1;
    assert(smoke_main() == 1);
    assert(time_reads == 0 && pad_reads == 0 && freed);
    return 0;
}
