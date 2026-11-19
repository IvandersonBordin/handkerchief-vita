#ifndef HANDKERCHIEF_SPRITE_H
#define HANDKERCHIEF_SPRITE_H

#include <stdint.h>

/* Surfaces use A8B8G8R8 and positive dimensions. Pitch is in pixels, >= width. */
typedef struct {
    int width;
    int height;
    int pitch;
} SpriteSurface;

/* Buffers must be valid and separate. Zero alpha skips the pixel. */
void sprite_blit(uint32_t *destination, SpriteSurface screen,
                 const uint32_t *pixels, SpriteSurface sprite, int x, int y);

/* Uniform-duration loop. Zero duration or frame count selects frame zero. */
uint32_t sprite_animation_frame(uint64_t active_us, uint32_t frame_us,
                                uint32_t frame_count);

#endif
