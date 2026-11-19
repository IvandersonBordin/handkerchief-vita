#ifndef DEMO_ROUND_H
#define DEMO_ROUND_H

#include <stdbool.h>

enum {
    DEMO_START_X = 472,
    DEMO_PLAYER_SIZE = 16,
    DEMO_FLOOR_Y = 400,
    DEMO_OBSTACLE_LEFT = 560,
    DEMO_OBSTACLE_RIGHT = 592,
    DEMO_OBSTACLE_TOP = 380,
    DEMO_EXIT_X = 720,
};

typedef struct {
    int x;
    int feet_y;
    int velocity_y;
    bool walking;
    bool facing_left;
    bool grounded;
    bool finished;
    bool x_held;
} DemoRound;

void demo_round_init(DemoRound *round);
/* Advance one fixed simulation step. Buttons are their current held states. */
void demo_round_step(DemoRound *round, bool left, bool right, bool x_held);

#endif
