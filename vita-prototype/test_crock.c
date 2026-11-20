#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { CELL_SIZE = 4, WIDTH = 4, HEIGHT = 5 };

typedef struct {
    int x;
    int y;
} Point;

/* Synthetic row-major grid. The last column matches mask 1 throughout. */
static const uint8_t grid[HEIGHT][WIDTH] = {
    {0, 2, 0, 1},
    {1, 2, 0, 1},
    {3, 0, 0, 1},
    {1, 2, 0, 1},
    {0, 2, 1, 1},
};

/* Standalone model of the vertical scan in docs/hdk-format.md. */
static Point query_vertical(Point point, uint8_t mask)
{
    /* Local fixture policy for inputs outside the documented scan. */
    if (point.x < 0 || point.x >= WIDTH * CELL_SIZE ||
        point.y < 0 || point.y >= HEIGHT * CELL_SIZE)
        return point;

    const int col = point.x / CELL_SIZE;
    const int row = point.y / CELL_SIZE;
    const int matches = (grid[row][col] & mask) != 0;
    const int step = matches ? -1 : 1;

    for (int next = row + step; next >= 0 && next < HEIGHT; next += step) {
        if (((grid[next][col] & mask) != 0) != matches) {
            point.y = next * CELL_SIZE;
            break;
        }
    }
    return point;
}

int main(void)
{
    static const struct {
        const char *name;
        Point input;
        uint8_t mask;
        int expected_y;
    } cases[] = {
        {"down from a partial cell", {3, 3}, 1, 4},
        {"up across a run to the top row", {3, 15}, 1, 0},
        {"combined bits in a single cell", {2, 1}, 2, 8},
        {"any bit in a combined mask", {1, 1}, 3, 4},
        {"exact column boundary", {4, 9}, 2, 12},
        {"bottom row is a valid transition", {11, 3}, 1, 16},
        {"bottom row with upward transition", {11, 19}, 1, 12},
        {"rightmost column with no upward transition", {15, 19}, 1, 19},
        {"first row already matches", {7, 3}, 2, 3},
        {"last row has no downward transition", {7, 19}, 1, 19},
        {"mask selects no bits", {3, 7}, 0, 7},
        {"negative x", {-1, 3}, 1, 3},
        {"negative y", {3, -1}, 1, -1},
        {"x past the last column", {16, 3}, 1, 3},
        {"y past the last row", {11, 20}, 1, 20},
    };
    const size_t count = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < count; ++i) {
        const Point input = cases[i].input;
        const Point result = query_vertical(input, cases[i].mask);
        if (result.x != input.x || result.y != cases[i].expected_y) {
            fprintf(stderr, "%s: got (%d, %d), expected (%d, %d)\n",
                    cases[i].name, result.x, result.y, input.x, cases[i].expected_y);
            return EXIT_FAILURE;
        }
    }
    printf("CRock: %zu cases passed\n", count);
    return EXIT_SUCCESS;
}
