/* ============================================================
 *  snake.h — 贪吃蛇游戏 (仅 LCD 模式)
 *
 *  16×18 网格，200ms 步进，方向键禁止 180° 调头。
 *  撞墙/撞身 → Game Over，按中心键重新开始。
 * ============================================================ */
#ifndef SNAKE_H
#define SNAKE_H

#include <stdint.h>
#include "input.h"

void snake_init(void);                           /* 初始化蛇 (长度 3，向右) */
void snake_draw(input_event_t ev, joy_state_t *js);  /* 每帧调用，含移动逻辑 */

#endif
