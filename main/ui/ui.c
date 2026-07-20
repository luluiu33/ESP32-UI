/* ============================================================
 *  ui.c — UI 状态机实现
 *
 *  扁平枚举 + switch-case 调度。
 *  主循环每帧调用 ui_process(ev, js) 处理输入和绘制。
 *  长按中心按钮 (>500ms) 全局生效：
 *    菜单模式下 → 进入校准
 *    非菜单模式 → 返回主菜单
 * ============================================================ */
#include "ui.h"
#include "input.h"
#include "menu.h"
#include "test_ui.h"
#include "calibrate.h"
#include "about.h"
#include "circle_test.h"
#include "display_test.h"
#include "snake.h"
#include "display.h"
#include "display_layout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LONG_PRESS_MS 500          /* 长按判定阈值 (ms) */

/* ---- 状态枚举 ---- */
typedef enum { UI_MENU, UI_CIRCLE, UI_TEST, UI_DISP_TEST, UI_SNAKE, UI_CAL, UI_ABOUT } ui_mode_t;

static ui_mode_t mode = UI_MENU;            /* 当前模式 */
static uint32_t center_press_ms = 0;        /* 中心按键按下时间戳 */

void ui_init(void)
{
    display_init();
    display_set_title("Main Menu");
    menu_draw();
}

uint8_t ui_is_test(void)
{
    return mode == UI_TEST;
}

/* 退出当前模式，回到主菜单 */
void ui_exit_test(void)
{
    mode = UI_MENU;
    display_set_title("Main Menu");
    menu_reset();
    menu_draw();
}

void ui_process(input_event_t ev, joy_state_t *js)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    /* ---- 长按检测 ---- */
    if (js->center) {
        if (center_press_ms == 0)
            center_press_ms = now;          /* 记录按下时刻 */
    } else {
        center_press_ms = 0;                /* 已释放，清零 */
    }

    if (center_press_ms && (now - center_press_ms) > LONG_PRESS_MS) {
        center_press_ms = 0;
        if (mode == UI_MENU) {
            mode = UI_CAL;                  /* 菜单模式长按 → 进入校准 */
            display_set_title("Calibrate");
            cal_init();
            cal_draw();
            return;
        }
        ui_exit_test();                     /* 非菜单模式 → 回到主菜单 */
        return;
    }

    /* ---- 模式调度 ---- */
    switch (mode) {

    case UI_MENU:
        if (ev == EV_UP)          menu_focus_up();
        else if (ev == EV_DOWN)   menu_focus_down();
        else if (ev == EV_RIGHT) {
            int f = menu_get_focus();
            if (f == 0) {
                mode = UI_CIRCLE;
                circle_test_reset();
                display_set_title("Circle");
                display_clear();
                return;
            } else if (f == 1) {
                mode = UI_TEST;
                test_ui_reset();
                display_set_title("Button");
                display_clear();
                return;
            } else if (f == 2) {
                mode = UI_DISP_TEST;
                disp_test_enter(0);
                display_clear();
                return;
            } else if (f == 3) {
                mode = UI_SNAKE;
                snake_init();
                display_set_title("Snake");
                display_clear();
                return;
            } else if (f == 4) {
                mode = UI_ABOUT;
                about_enter();
                display_set_title("About");
                about_draw();
                return;
            }
        }
        if (menu_is_dirty()) menu_draw();   /* 焦点变化时惰性重绘 */
        break;

    case UI_DISP_TEST:
        if (ev == EV_UP) {
            disp_test_prev();
            display_clear();
        } else if (ev == EV_DOWN) {
            disp_test_next();
            display_clear();
        }
        disp_test_draw(js);
        break;

    case UI_CIRCLE:
        circle_test_draw(js);
        break;

    case UI_TEST:
        test_ui_draw(js);
        break;

    case UI_SNAKE:
        snake_draw(ev, js);
        break;

    case UI_CAL:
        if (cal_process()) {
            mode = UI_MENU;                 /* 校准完成 → 回主菜单 */
            menu_reset();
            display_set_title("Main Menu");
            menu_draw();
        }
        break;

    case UI_ABOUT:
        if (ev == EV_DOWN)      about_scroll(1);
        else if (ev == EV_UP)   about_scroll(-1);
        break;
    }
}
