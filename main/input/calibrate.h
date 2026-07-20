/* ============================================================
 *  calibrate.h — 4 步摇杆校准向导
 *
 *  步骤：UP → DOWN → LEFT → RIGHT
 *  每步需将摇杆推向极限并保持 300ms 采集。
 *  完成后自动计算 swap_xy / invert_x / invert_y 并存入 NVS。
 * ============================================================ */
#ifndef CALIBRATE_H
#define CALIBRATE_H

#include <stdint.h>

void cal_init(void);            /* 进入校准模式 (重置步骤) */
void cal_draw(void);            /* 绘制当前步骤画面 */
uint8_t cal_process(void);     /* 处理校准流程；返回 1 表示完成 */

#endif
