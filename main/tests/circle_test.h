/* ============================================================
 *  circle_test.h — 圆轨迹追踪测试
 *  摇杆控制亮点沿圆周移动，超出边界时按比例压缩到圆上。
 *  像素位置不变化时跳过重绘 (惰性优化)。
 * ============================================================ */
#ifndef CIRCLE_TEST_H
#define CIRCLE_TEST_H

#include "input.h"

void circle_test_reset(void);               /* 重置光标位置 */
void circle_test_draw(joy_state_t *js);     /* 绘制圆 + 光标 */

#endif
