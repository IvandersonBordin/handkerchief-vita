#ifndef ROUND_SCENE_H
#define ROUND_SCENE_H

#include <stdint.h>

#include "demo_round.h"

enum {
    ROUND_WIDTH = 960,
    ROUND_HEIGHT = 544,
    ROUND_PITCH = 1024,
    ROUND_SCENE_WIDTH = 4464,
    ROUND_SCENE_HEIGHT = 384,
    ROUND_SCENE_TOP = 80,
};

bool round_scene_load(const char *path);
void round_scene_draw(uint32_t *pixels, const DemoRound *round);

#endif
