#include "sprite.h"

#include <assert.h>

int main(void)
{
    const uint32_t frame_us = 100000;
    const uint32_t frame_count = 3;

    assert(sprite_animation_frame(0, frame_us, frame_count) == 0);
    assert(sprite_animation_frame(99999, frame_us, frame_count) == 0);
    assert(sprite_animation_frame(100000, frame_us, frame_count) == 1);
    assert(sprite_animation_frame(299999, frame_us, frame_count) == 2);
    assert(sprite_animation_frame(300000, frame_us, frame_count) == 0);
    assert(sprite_animation_frame(700000, frame_us, frame_count) == 1);
    assert(sprite_animation_frame(UINT64_MAX, 1, 7) == 1);
    assert(sprite_animation_frame(UINT64_MAX, frame_us, 1) == 0);
    assert(sprite_animation_frame(100000, 0, frame_count) == 0);
    assert(sprite_animation_frame(100000, frame_us, 0) == 0);
    return 0;
}
