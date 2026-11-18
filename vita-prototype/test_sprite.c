#include "sprite.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#define A UINT32_C(0xff0000ff)
#define B UINT32_C(0xff00ff00)
#define C UINT32_C(0xffff0000)

static void check_blit(int x, int y, const uint32_t expected[15])
{
    const uint32_t sprite[] = {
        A, 0x0000ffff, C,
        B, C, A,
    };
    uint32_t frame[15] = {0};
    sprite_blit(frame, (SpriteSurface){4, 3, 5},
                sprite, (SpriteSurface){2, 2, 3}, x, y);
    assert(memcmp(frame, expected, sizeof(frame)) == 0);
}

int main(void)
{
    check_blit(1, 1, (uint32_t[15]){[6] = A, [11] = B, [12] = C});
    check_blit(-1, -1, (uint32_t[15]){[0] = C});
    check_blit(3, 2, (uint32_t[15]){[13] = A});
    check_blit(-2, 0, (uint32_t[15]){0});
    check_blit(0, 3, (uint32_t[15]){0});
    check_blit(INT_MIN, 0, (uint32_t[15]){0});
    check_blit(0, INT_MAX, (uint32_t[15]){0});
    return 0;
}
