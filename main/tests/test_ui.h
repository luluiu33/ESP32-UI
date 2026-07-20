/* ============================================================
 *  test_ui.h — 五方向按键测试
 *  十字箭头 + 中心按钮的状态指示。
 *  激活时箭头实心，中心按钮按下时实心圆。
 *  5-bit 状态掩码不变时跳过重绘。
 * ============================================================ */
#ifndef TEST_UI_H
#define TEST_UI_H

#include "input.h"

void test_ui_reset(void);                   /* 重置状态掩码 */
void test_ui_draw(joy_state_t *state);     /* 绘制方向测试画面 */

#endif
