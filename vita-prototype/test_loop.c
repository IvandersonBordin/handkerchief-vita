#include <assert.h>
#include <stdint.h>

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>

/* Screen storage rounded to 256 KiB for each of the two frames. */
static uint32_t frames[2 * 2304 * 1024 / sizeof(uint32_t)];
static unsigned time_reads;
static unsigned pad_reads;
static unsigned vblanks;
static unsigned display_calls;
static int freed;
enum { LOOP_VBLANKS = 12, TOTAL_VBLANKS = LOOP_VBLANKS + 1 };

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
    *base = frames;
    return 0;
}

int sceKernelFreeMemBlock(SceUID uid)
{
    assert(uid == 1 && display_calls == 5 && vblanks == TOTAL_VBLANKS && !freed);
    freed = 1;
    return 0;
}

int sceDisplaySetFrameBuf(const SceDisplayFrameBuf *frame, SceDisplaySetBufSync sync)
{
    assert(sync == SCE_DISPLAY_SETBUF_NEXTFRAME);
    if (display_calls == 0) {
        assert(frame && frame->base == frames);
    } else if (display_calls < 4) {
        const uint32_t *expected = display_calls == 2
            ? frames : frames + sizeof(frames) / sizeof(frames[0]) / 2;
        assert(frame && frame->base == expected);
        assert(vblanks == 6 + display_calls);
    } else {
        assert(display_calls == 4 && !frame && vblanks == LOOP_VBLANKS);
    }
    if (frame) {
        assert(frame->size == sizeof(*frame));
        assert(frame->width == 960 && frame->height == 544);
        assert(frame->pitch == 1024);
        assert(frame->pixelformat == SCE_DISPLAY_PIXELFORMAT_A8B8G8R8);
        const uint32_t *pixels = frame->base;
        const uint32_t middle = frame->base == frames ? 0xff00ff00 : 0xff00ffff;
        for (unsigned y = 0; y < frame->height; ++y) {
            const uint32_t *row = pixels + y * frame->pitch;
            assert(row[0] == 0xff0000ff && row[319] == 0xff0000ff);
            assert(row[320] == middle && row[639] == middle);
            assert(row[640] == 0xffff0000 && row[959] == 0xffff0000);
            assert(row[960] == 0xff000000 && row[1023] == 0xff000000);
        }
    }
    ++display_calls;
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
    /* Count 20 and 30 before the pause, then 70 after resuming. */
    static const SceInt64 readings[] = {
        100, 120, 110, 140, 1000, 2000, 3000, 4000, 4010, 4020, 4040, 4070, 5000,
    };
    assert(time_reads < sizeof(readings) / sizeof(readings[0]));
    return readings[time_reads++];
}

int sceCtrlPeekBufferPositive(int port, SceCtrlData *pad, int count)
{
    static const uint32_t buttons[] = {
        0, 0, SCE_CTRL_CIRCLE, 0, SCE_CTRL_CIRCLE | SCE_CTRL_CROSS,
        0, SCE_CTRL_CIRCLE | SCE_CTRL_CROSS, 0, SCE_CTRL_CROSS, SCE_CTRL_CROSS,
        SCE_CTRL_CIRCLE,
        SCE_CTRL_START | SCE_CTRL_CIRCLE | SCE_CTRL_CROSS,
    };
    assert(port == 0 && count == 1);
    assert(pad_reads < sizeof(buttons) / sizeof(buttons[0]));
    const unsigned sample = pad_reads++;
    /* A missing sample while CIRCLE is held must preserve the pause. */
    if (sample == 3)
        return 0;
    pad->buttons = buttons[sample];
    return 1;
}

int main(void)
{
    assert(smoke_main() == 0);
    assert(time_reads == LOOP_VBLANKS + 1 && pad_reads == LOOP_VBLANKS && freed);
    return 0;
}
