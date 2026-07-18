#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <stdint.h>

#define CX         64
#define CY         40
#define GAP        16
#define ARROW_SZ   4
#define CIRCLE_R   5

#define MAP_UP     0
#define MAP_DOWN   1
#define MAP_LEFT   2
#define MAP_RIGHT  3

extern const uint8_t pos[5][2];

void draw_arrow(uint8_t cx, uint8_t cy, uint8_t dir, uint8_t fill);

#endif
