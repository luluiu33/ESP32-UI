#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <stdint.h>
#include "display_layout.h"

#define ARROW_SZ   4

#define MAP_UP     0
#define MAP_DOWN   1
#define MAP_LEFT   2
#define MAP_RIGHT  3

extern const uint8_t pos[5][2];

void draw_arrow(uint8_t cx, uint8_t cy, uint8_t dir, uint8_t fill);

#endif
