/* ============================================================
 *  ui_common.h — 共享绘图原语 (箭头绘制 + 方向布局)
 *  用于校准向导和五方向测试。
 * ============================================================ */
#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <stdint.h>
#include "display_layout.h"

#define ARROW_SZ   4               /* 三角形箭头边长 (像素) */

/* 方向枚举 (也用于校准步骤索引) */
#define MAP_UP     0
#define MAP_DOWN   1
#define MAP_LEFT   2
#define MAP_RIGHT  3

/* 五方向布局: [UP, DOWN, LEFT, RIGHT, CENTER] 像素坐标 */
extern const uint8_t pos[5][2];

/* 绘制三角形箭头 (dir=0~3, fill=1 实心/0 空心) */
void draw_arrow(uint8_t cx, uint8_t cy, uint8_t dir, uint8_t fill);

#endif
