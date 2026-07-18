#include "calibrate.h"
#include "input.h"
#include "ui_common.h"
#include "display.h"
#include "display_layout.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CAL_PUSH_MS 300

static uint8_t  step;
static int16_t  raw[4][2];
static uint32_t push_ms;
static uint8_t  wait_release;

static const char *names[4] = { "UP", "DOWN", "LEFT", "RIGHT" };

void cal_init(void)
{
    step = 0;
    wait_release = 1;
    push_ms = 0;
}

void cal_draw(void)
{
    char buf[20];
    char title[12];
    display_clear();

    snprintf(title, sizeof(title), "Cal %s", names[step]);
    display_draw_string(28, CAL_TITLE_Y, title);
    snprintf(buf, sizeof(buf), "Step %d/4", step + 1);
    display_draw_string(24, CAL_STEP_Y, buf);

    draw_arrow(CAL_ARROW_CX, CAL_ARROW_CY, step, 1);

    snprintf(buf, sizeof(buf), "[%s]", names[step]);
    display_draw_string(24, CAL_LABEL_Y, buf);

    display_update();
}

void cal_cancel(void)
{
    step = 0;
    wait_release = 1;
    push_ms = 0;
}

uint8_t cal_process(void)
{
    int16_t ax, ay;
    input_get_raw(&ax, &ay);

    if (wait_release) {
        if (ax > ADC_LO && ax < ADC_HI && ay > ADC_LO && ay < ADC_HI)
            wait_release = 0;
        return 0;
    }

    int moved = (ax < ADC_LO || ax > ADC_HI || ay < ADC_LO || ay > ADC_HI);
    if (!moved) {
        push_ms = 0;
        return 0;
    }

    if (push_ms == 0) {
        push_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    } else if ((xTaskGetTickCount() * portTICK_PERIOD_MS - push_ms) >= CAL_PUSH_MS) {
        raw[step][0] = ax;
        raw[step][1] = ay;
        step++;
        if (step >= 4) {
            input_calibrate(raw);
            input_save_cal();
            return 1;
        }
        push_ms = 0;
        wait_release = 1;
        cal_draw();
    }
    return 0;
}
