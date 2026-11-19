/* Host regression with synthetic pixels. Discard the unused Vita entry point. */
#include <assert.h>
#define main vita_entry_point
#include "main.c"
#undef main

int main(void)
{
    static uint32_t frame[SCREEN_HEIGHT][SCREEN_PITCH];
    assert(camera_x(0) == 0);
    assert(camera_x(1000) == 528);
    const int right = camera_x(WORLD_WIDTH - SPRITE_SIZE);
    assert(right == WORLD_WIDTH - SCREEN_WIDTH);
    world[0][0] = 123;
    world[WORLD_HEIGHT - 1][WORLD_WIDTH - 1] = 456;
    draw_scene(&frame[0][0], 0, 1);
    assert(frame[SCENE_TOP][0] == 123);
    draw_scene(&frame[0][0], right, 1);
    assert(frame[SCENE_TOP + WORLD_HEIGHT - 1][SCREEN_WIDTH - 1] == 456);
    assert(frame[0][0] == 0xff281b14);
    draw_scene(&frame[0][0], 0, 0);
    assert(frame[SCENE_TOP][0] == 0xff2020a0);
    return 0;
}
