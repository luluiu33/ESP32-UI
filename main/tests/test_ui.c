/* ============================================================
 *  test_ui.c — 五方向按键测试实现
 *
 *  5-bit 掩码编码五个方向状态。掩码不变时跳过重绘。
 *  激活的方向箭头实心填充，中心按钮按下时绘制实心圆。
 * ============================================================ */
#include "test_ui.h"
#include "ui_common.h"
#include "display.h"
#include "display_layout.h"

static uint8_t prev_mask = 0xFF;   /* 上次绘制时的掩码 (初始 != 任何状态) */

void test_ui_reset(void)
{
    prev_mask = 0xFF;               /* 强制第一次重绘 */
}

void test_ui_draw(joy_state_t *state)
{
    uint8_t mask = state->up | (state->down << 1) | (state->left << 2) |
                   (state->right << 3) | (state->center << 4);
    if (mask == prev_mask) return;  /* 无变化 → 跳过 */
    prev_mask = mask;

    uint8_t arrows[4];
    arrows[MAP_UP]    = state->up;
    arrows[MAP_DOWN]  = state->down;
    arrows[MAP_LEFT]  = state->left;
    arrows[MAP_RIGHT] = state->right;

    display_clear();

    for (int i = 0; i < 4; i++)
        draw_arrow(pos[i][0], pos[i][1], i, arrows[i]);
    display_draw_circle(pos[4][0], pos[4][1], CIRCLE_R, state->center);

    display_update();
}
