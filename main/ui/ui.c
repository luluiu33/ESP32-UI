#include "ui.h"
#include "input.h"
#include "menu.h"
#include "test_ui.h"
#include "calibrate.h"
#include "about.h"
#include "circle_test.h"
#include "display_test.h"
#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LONG_PRESS_MS 500

typedef enum { UI_MENU, UI_CIRCLE, UI_TEST, UI_DISP_TEST, UI_CAL, UI_ABOUT } ui_mode_t;

static ui_mode_t mode = UI_MENU;
static uint32_t center_press_ms = 0;

void ui_init(void)
{
    ssd1306_init();
    menu_draw();
}

uint8_t ui_is_test(void)
{
    return mode == UI_TEST;
}

void ui_exit_test(void)
{
    mode = UI_MENU;
    menu_reset();
    menu_draw();
}

void ui_process(input_event_t ev, joy_state_t *js)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (js->center) {
        if (center_press_ms == 0)
            center_press_ms = now;
    } else {
        center_press_ms = 0;
    }

    if (center_press_ms && (now - center_press_ms) > LONG_PRESS_MS && mode != UI_MENU) {
        if (mode == UI_CAL) cal_cancel();
        ui_exit_test();
        return;
    }

    switch (mode) {

    case UI_MENU:
        if (ev == EV_UP)          menu_focus_up();
        else if (ev == EV_DOWN)   menu_focus_down();
        else if (ev == EV_RIGHT) {
            int f = menu_get_focus();
            if (f == 0) {
                mode = UI_CIRCLE;
                circle_test_reset();
                ssd1306_clear();
                return;
            } else if (f == 1) {
                mode = UI_TEST;
                test_ui_reset();
                ssd1306_clear();
                return;
            } else if (f == 2) {
                mode = UI_DISP_TEST;
                disp_test_enter(0);
                ssd1306_clear();
                return;
            } else if (f == 3) {
                mode = UI_CAL;
                cal_init();
                cal_draw();
                return;
            } else if (f == 4) {
                mode = UI_ABOUT;
                about_enter();
                about_draw();
                return;
            }
        }
        if (menu_is_dirty()) menu_draw();
        break;

    case UI_DISP_TEST:
        if (ev == EV_UP) {
            disp_test_prev();
            ssd1306_clear();
        } else if (ev == EV_DOWN) {
            disp_test_next();
            ssd1306_clear();
        }
        disp_test_draw(js);
        break;

    case UI_CIRCLE:
        circle_test_draw(js);
        break;

    case UI_TEST:
        test_ui_draw(js);
        break;

    case UI_CAL:
        if (cal_process()) {
            mode = UI_MENU;
            menu_reset();
            menu_draw();
        }
        break;

    case UI_ABOUT:
        if (ev == EV_DOWN) {
            about_scroll(1);
        } else if (ev == EV_UP) {
            about_scroll(-1);
        }
        break;
    }
}
