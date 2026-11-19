#include "sprite.h"

#include <stddef.h>

uint32_t sprite_animation_frame(uint64_t active_us, uint32_t frame_us,
                                uint32_t frame_count)
{
    if (frame_us == 0 || frame_count == 0) {
        return 0;
    }

    return (uint32_t)((active_us / frame_us) % frame_count);
}

void sprite_blit(uint32_t *destination, SpriteSurface screen,
                 const uint32_t *pixels, SpriteSurface sprite, int x, int y)
{
    if (x >= screen.width || y >= screen.height ||
        x <= -sprite.width || y <= -sprite.height)
        return;

    const int left = x < 0 ? 0 : x;
    const int top = y < 0 ? 0 : y;
    const int width = screen.width - left;
    const int height = screen.height - top;
    const int source_x = left - x;
    const int source_y = top - y;
    const int visible_width = sprite.width - source_x;
    const int visible_height = sprite.height - source_y;
    const int columns = width < visible_width ? width : visible_width;
    const int rows = height < visible_height ? height : visible_height;

    for (int row = 0; row < rows; ++row) {
        const uint32_t *source = pixels + (size_t)(source_y + row) * sprite.pitch + source_x;
        uint32_t *target = destination + (size_t)(top + row) * screen.pitch + left;
        for (int column = 0; column < columns; ++column) {
            if (source[column] >> 24)
                target[column] = source[column];
        }
    }
}
