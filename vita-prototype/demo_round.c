#include "demo_round.h"

void demo_round_init(DemoRound *round)
{
    *round = (DemoRound){
        .x = DEMO_START_X,
        .feet_y = DEMO_FLOOR_Y,
        .grounded = true,
    };
}

void demo_round_step(DemoRound *round, bool left, bool right, bool x_held)
{
    const bool x_pressed = x_held && !round->x_held;
    round->x_held = x_held;

    if (round->finished) {
        if (x_pressed) {
            demo_round_init(round);
            round->x_held = true;
        }
        return;
    }

    if (x_pressed && round->grounded)
        round->velocity_y = -10;

    int next_x = round->x + 4 * (right - left);
    if (next_x < 0)
        next_x = 0;
    if (round->feet_y > DEMO_OBSTACLE_TOP) {
        if (round->x + DEMO_PLAYER_SIZE <= DEMO_OBSTACLE_LEFT &&
            next_x + DEMO_PLAYER_SIZE > DEMO_OBSTACLE_LEFT)
            next_x = DEMO_OBSTACLE_LEFT - DEMO_PLAYER_SIZE;
        if (round->x >= DEMO_OBSTACLE_RIGHT && next_x < DEMO_OBSTACLE_RIGHT)
            next_x = DEMO_OBSTACLE_RIGHT;
    }
    round->walking = next_x != round->x;
    if (round->walking)
        round->facing_left = next_x < round->x;
    round->x = next_x;

    /* The obstacle top is a landing surface only when approached from above. */
    int surface_y = DEMO_FLOOR_Y;
    if (round->feet_y <= DEMO_OBSTACLE_TOP &&
        next_x < DEMO_OBSTACLE_RIGHT &&
        next_x + DEMO_PLAYER_SIZE > DEMO_OBSTACLE_LEFT)
        surface_y = DEMO_OBSTACLE_TOP;

    ++round->velocity_y;
    round->feet_y += round->velocity_y;
    round->grounded = round->feet_y >= surface_y;
    if (round->grounded) {
        round->feet_y = surface_y;
        round->velocity_y = 0;
    }
    round->finished = round->x >= DEMO_EXIT_X;
}
