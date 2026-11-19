#include <assert.h>

#include "demo_round.h"

int main(void)
{
    DemoRound round;
    demo_round_init(&round);
    assert(round.x == DEMO_START_X && round.feet_y == DEMO_FLOOR_Y);
    assert(round.grounded && !round.finished && !round.x_held);

    demo_round_step(&round, true, false, false);
    assert(round.x == DEMO_START_X - 4);
    demo_round_step(&round, true, true, false);
    assert(round.x == DEMO_START_X - 4);
    for (int step = 0; step < 30; ++step)
        demo_round_step(&round, false, true, false);
    assert(round.x + DEMO_PLAYER_SIZE == DEMO_OBSTACLE_LEFT);

    demo_round_step(&round, false, true, true);
    assert(round.feet_y < DEMO_FLOOR_Y && !round.grounded);
    for (int step = 0; step < 12; ++step)
        demo_round_step(&round, false, true, true);
    assert(round.x >= DEMO_OBSTACLE_LEFT && round.x < DEMO_OBSTACLE_RIGHT);
    for (int step = 0; step < 12; ++step)
        demo_round_step(&round, false, false, true);
    assert(round.grounded && round.feet_y == DEMO_OBSTACLE_TOP);

    for (int step = 0; step < 14; ++step)
        demo_round_step(&round, false, true, true);
    assert(round.x > DEMO_OBSTACLE_RIGHT && round.grounded);
    assert(round.feet_y == DEMO_FLOOR_Y);
    for (int step = 0; step < 20; ++step)
        demo_round_step(&round, true, false, true);
    assert(round.x == DEMO_OBSTACLE_RIGHT && round.feet_y == DEMO_FLOOR_Y);

    for (int step = 0; step < 40; ++step)
        demo_round_step(&round, false, true, true);
    assert(round.finished && round.x == DEMO_EXIT_X);
    demo_round_step(&round, true, false, true);
    assert(round.finished && round.x == DEMO_EXIT_X);
    demo_round_step(&round, false, false, false);
    demo_round_step(&round, false, false, true);
    assert(!round.finished && round.x == DEMO_START_X && round.grounded);
    demo_round_step(&round, false, false, true);
    assert(round.grounded && round.feet_y == DEMO_FLOOR_Y);
    return 0;
}
