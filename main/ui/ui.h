/* ============================================================
 *  ui.h — UI 状态机 (扁平枚举, switch-case 调度)
 *
 *  模式: MENU → CIRCLE / TEST / DISP_TEST / SNAKE / CAL / ABOUT
 *  长按中心按钮 (>500ms) 在任何非菜单模式下返回主菜单。
 * ============================================================ */
#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "input.h"

void ui_init(void);                     /* 初始化显示屏 + 绘制菜单 */
uint8_t ui_is_test(void);               /* 判断当前是否为按键测试模式 */
void ui_process(input_event_t ev, joy_state_t *js);  /* 主状态机 */
void ui_exit_test(void);                /* 退出到主菜单 */

#endif
