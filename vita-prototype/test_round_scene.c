#include <assert.h>
#include <stdio.h>

#include "round_scene.h"

static uint32_t pixels[ROUND_HEIGHT * ROUND_PITCH];

#define PIXEL(y, x) pixels[(y) * ROUND_PITCH + (x)]

int main(int argc, char **argv)
{
    assert(argc == 2);
    DemoRound round;
    demo_round_init(&round);

    assert(!round_scene_load(argv[1]));
    round_scene_draw(pixels, &round);
    assert(PIXEL(ROUND_SCENE_TOP, 0) == 0xff302028);
    assert(PIXEL(DEMO_FLOOR_Y, 0) == 0xff524545);

    FILE *file = fopen(argv[1], "wbx");
    assert(file);
    assert(fclose(file) == 0);
    assert(!round_scene_load(argv[1]));

    file = fopen(argv[1], "wb");
    assert(file);
    uint32_t row[ROUND_SCENE_WIDTH];
    for (int x = 0; x < ROUND_SCENE_WIDTH; ++x)
        row[x] = 0xff000000 | (uint32_t)x;
    for (int y = 0; y < ROUND_SCENE_HEIGHT; ++y)
        assert(fwrite(row, sizeof(row), 1, file) == 1);
    assert(fclose(file) == 0);
    assert(round_scene_load(argv[1]));

    round_scene_draw(pixels, &round);
    assert(PIXEL(DEMO_FLOOR_Y - 16, DEMO_START_X) == row[DEMO_START_X]);
    assert(PIXEL(DEMO_FLOOR_Y - 11, DEMO_START_X + 6) == 0xffffff80);
    assert(PIXEL(ROUND_SCENE_TOP, 0) == row[0]);
    uint32_t initial_robot[DEMO_PLAYER_SIZE][DEMO_PLAYER_SIZE];
    for (int y = 0; y < DEMO_PLAYER_SIZE; ++y)
        for (int x = 0; x < DEMO_PLAYER_SIZE; ++x)
            initial_robot[y][x] = PIXEL(DEMO_FLOOR_Y - DEMO_PLAYER_SIZE + y,
                                       DEMO_START_X + x);

    demo_round_step(&round, false, true, false);
    round_scene_draw(pixels, &round);
    assert(PIXEL(ROUND_SCENE_TOP, 0) == row[4]);
    assert(PIXEL(ROUND_SCENE_TOP, ROUND_WIDTH - 1) == row[ROUND_WIDTH + 3]);

    uint32_t first_stride = PIXEL(DEMO_FLOOR_Y - 3, DEMO_START_X + 3);
    for (int step = 0; step < 6; ++step)
        demo_round_step(&round, false, true, false);
    round_scene_draw(pixels, &round);
    assert(PIXEL(DEMO_FLOOR_Y - 3, DEMO_START_X + 3) != first_stride);

    for (int step = 0; step < 30; ++step)
        demo_round_step(&round, false, true, false);
    assert(round.x == DEMO_OBSTACLE_LEFT - DEMO_PLAYER_SIZE);
    demo_round_step(&round, false, true, true);
    round_scene_draw(pixels, &round);
    assert(round.feet_y < DEMO_FLOOR_Y);
    assert(PIXEL(round.feet_y - DEMO_PLAYER_SIZE, DEMO_START_X + 6) == 0xff332322);
    assert(PIXEL(round.feet_y - 1, DEMO_START_X + 7) == row[round.x + 7]);
    assert(PIXEL(DEMO_OBSTACLE_TOP, DEMO_OBSTACLE_LEFT - (round.x - DEMO_START_X))
           == 0xff0088dd);

    for (int step = 0; step < 60 && !round.finished; ++step)
        demo_round_step(&round, false, true, false);
    assert(round.finished);
    round_scene_draw(pixels, &round);
    assert(PIXEL(0, 0) == 0xff234d23);
    assert(PIXEL(ROUND_SCENE_TOP, 0) == row[DEMO_EXIT_X - DEMO_START_X]);

    demo_round_step(&round, false, false, true);
    round_scene_draw(pixels, &round);
    assert(round.x == DEMO_START_X && !round.finished);
    assert(PIXEL(0, 0) == 0xff281b14);
    assert(PIXEL(ROUND_SCENE_TOP, 0) == row[0]);
    for (int y = 0; y < DEMO_PLAYER_SIZE; ++y)
        for (int x = 0; x < DEMO_PLAYER_SIZE; ++x)
            assert(PIXEL(DEMO_FLOOR_Y - DEMO_PLAYER_SIZE + y, DEMO_START_X + x)
                   == initial_robot[y][x]);

    file = fopen(argv[1], "ab");
    assert(file);
    assert(fputc(0, file) != EOF);
    assert(fclose(file) == 0);
    assert(!round_scene_load(argv[1]));
    round_scene_draw(pixels, &round);
    assert(PIXEL(ROUND_SCENE_TOP, 0) == 0xff302028);
    assert(remove(argv[1]) == 0);
    return 0;
}
