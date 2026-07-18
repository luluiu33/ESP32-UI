#include "test_ui.h"
#include "ui_common.h"
#include "ssd1306.h"

static uint8_t prev_mask = 0xFF;

void test_ui_reset(void)
{
    prev_mask = 0xFF;
}

void test_ui_draw(joy_state_t *state)
{
    uint8_t mask = state->up | (state->down << 1) | (state->left << 2) |
                   (state->right << 3) | (state->center << 4);
    if (mask == prev_mask) return;
    prev_mask = mask;

    uint8_t arrows[4];
    arrows[MAP_UP]    = state->up;
    arrows[MAP_DOWN]  = state->down;
    arrows[MAP_LEFT]  = state->left;
    arrows[MAP_RIGHT] = state->right;

    ssd1306_clear();
    ssd1306_draw_string(34, 0, "Button");

    for (int i = 0; i < 4; i++)
        draw_arrow(pos[i][0], pos[i][1], i, arrows[i]);
    ssd1306_draw_circle(pos[4][0], pos[4][1], CIRCLE_R, state->center);

    ssd1306_update();
}
