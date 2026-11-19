#include <stdio.h>
#include <string.h>

#include "round_scene.h"

static uint32_t world[ROUND_SCENE_HEIGHT][ROUND_SCENE_WIDTH];
static bool loaded;

bool round_scene_load(const char *path)
{
    loaded = false;
    FILE *file = fopen(path, "rb");
    if (!file)
        return false;

    const bool complete = fread(world, 1, sizeof(world), file) == sizeof(world)
                          && fgetc(file) == EOF && !ferror(file);
    loaded = fclose(file) == 0 && complete;
    return loaded;
}

static int camera_x(int player_x)
{
    const int left = player_x + DEMO_PLAYER_SIZE / 2 - ROUND_WIDTH / 2;
    if (left < 0)
        return 0;
    if (left > ROUND_SCENE_WIDTH - ROUND_WIDTH)
        return ROUND_SCENE_WIDTH - ROUND_WIDTH;
    return left;
}

static void fill_rect(uint32_t *pixels, int left, int top, int right, int bottom,
                      uint32_t color)
{
    if (left < 0)
        left = 0;
    if (right > ROUND_WIDTH)
        right = ROUND_WIDTH;
    for (int y = top; y < bottom; ++y)
        for (int x = left; x < right; ++x)
            pixels[y * ROUND_PITCH + x] = color;
}

void round_scene_draw(uint32_t *pixels, const DemoRound *round)
{
    const int left = loaded ? camera_x(round->x) : 0;
    for (int y = 0; y < ROUND_HEIGHT; ++y) {
        uint32_t *row = pixels + y * ROUND_PITCH;
        if (loaded && y >= ROUND_SCENE_TOP &&
            y < ROUND_SCENE_TOP + ROUND_SCENE_HEIGHT) {
            memcpy(row, &world[y - ROUND_SCENE_TOP][left],
                   ROUND_WIDTH * sizeof(*row));
        } else {
            uint32_t color = round->finished ? 0xff234d23 : 0xff302028;
            if (!loaded && y >= DEMO_FLOOR_Y)
                color = 0xff524545;
            else if (loaded && !round->finished)
                color = 0xff281b14;
            const int width = loaded ? ROUND_WIDTH : ROUND_PITCH;
            for (int x = 0; x < width; ++x)
                row[x] = color;
        }
    }

    fill_rect(pixels, DEMO_OBSTACLE_LEFT - left, DEMO_OBSTACLE_TOP,
              DEMO_OBSTACLE_RIGHT - left, DEMO_FLOOR_Y, 0xff0088dd);
    fill_rect(pixels, DEMO_EXIT_X - left, DEMO_FLOOR_Y - 80,
              DEMO_EXIT_X + 4 - left, DEMO_FLOOR_Y, 0xff55ef70);
    fill_rect(pixels, round->x - left, round->feet_y - DEMO_PLAYER_SIZE,
              round->x + DEMO_PLAYER_SIZE - left, round->feet_y, 0xffffffff);
}
