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
    assert(uid == 1 && display_calls == 2 && vblanks == 6 && !freed);
    freed = 1;
    return 0;
}

int sceDisplaySetFrameBuf(const SceDisplayFrameBuf *frame, SceDisplaySetBufSync sync)
{
    assert(sync == SCE_DISPLAY_SETBUF_NEXTFRAME);
    if (display_calls == 0) {
        assert(frame && frame->base == frames);
    } else {
        assert(display_calls == 1 && !frame && vblanks == 5);
    }
    ++display_calls;
    return 0;
}

int sceDisplayWaitVblankStart(void)
{
    assert(vblanks < 6);
    ++vblanks;
    return 0;
}

SceInt64 sceKernelGetSystemTimeWide(void)
{
    /* Count 20, skip the backward step, count 30, then pause. */
    static const SceInt64 readings[] = {100, 120, 110, 140, 1000, 2000};
    assert(time_reads < sizeof(readings) / sizeof(readings[0]));
    return readings[time_reads++];
}

int sceCtrlPeekBufferPositive(int port, SceCtrlData *pad, int count)
{
    static const uint32_t buttons[] = {
        0, 0, SCE_CTRL_CIRCLE, 0, SCE_CTRL_START | SCE_CTRL_CIRCLE | SCE_CTRL_CROSS,
    };
    assert(port == 0 && count == 1);
    assert(pad_reads < sizeof(buttons) / sizeof(buttons[0]));
    pad->buttons = buttons[pad_reads++];
    return 1;
}

int main(void)
{
    assert(smoke_main() == 0);
    assert(time_reads == 6 && pad_reads == 5 && freed);
    return 0;
}
