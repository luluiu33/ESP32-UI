/* ============================================================
 *  display_test.h — 6 种动画演示
 *   0 Sine Wave   128px 正弦波相位自旋
 *   1 Bounce Ball 重力反弹小球 (含重力加速度)
 *   2 Progress    0→100%→0 往返进度条
 *   3 Shapes      静态几何构图
 *   4 Charset     ASCII 32~126 字符矩阵
 *   5 Marquee     四角爬行亮线 (36px 尾迹)
 *  左右键切换模式，每帧全屏刷新。
 * ============================================================ */
#ifndef DISPLAY_TEST_H
#define DISPLAY_TEST_H

#include "input.h"

#define DISP_TEST_COUNT 6

void disp_test_enter(int index);            /* 切换到指定动画 */
void disp_test_draw(joy_state_t *js);       /* 绘制当前动画帧 */
void disp_test_next(void);                  /* 下一动画 */
void disp_test_prev(void);                  /* 上一动画 */

#endif
