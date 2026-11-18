#include "sprite.h"

#include <stddef.h>

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
