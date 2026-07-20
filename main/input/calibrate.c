/* ============================================================
 *  calibrate.c — 4 步摇杆校准向导实现
 *
 *  流程：等待松开 (回中) → 等待推向极限 → 保持 300ms 采样
 *  → 推进到下一步 → 完成后调用 input_calibrate() + input_save_cal()
 * ============================================================ */
#include "calibrate.h"
#include "input.h"
#include "ui_common.h"
#include "display.h"
#include "display_layout.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CAL_PUSH_MS 300         /* 每步需保持极限 300ms */

/* ---- 校准状态 ---- */
static uint8_t  step;           /* 当前步骤：0=UP 1=DOWN 2=LEFT 3=RIGHT */
static int16_t  raw[4][2];      /* 4 方向 ADC 采样值 */
static uint32_t push_ms;        /* 开始偏离的时刻 */
static uint8_t  wait_release;   /* 等待摇杆回到中位标志 */

static const char *names[4] = { "UP", "DOWN", "LEFT", "RIGHT" };

void cal_init(void)
{
    step = 0;
    wait_release = 1;
    push_ms = 0;
}

/* 绘制当前校准步骤的画面 */
void cal_draw(void)
{
    display_clear();

    display_printf(28, CAL_TITLE_Y, "Cal %s", names[step]);
    display_printf(24, CAL_STEP_Y, "Step %d/4", step + 1);

    draw_arrow(CAL_ARROW_CX, CAL_ARROW_CY, step, 1);   /* 指示方向 */

    display_printf(24, CAL_LABEL_Y, "[%s]", names[step]);

    display_update();
}

void cal_cancel(void)
{
    step = 0;
    wait_release = 1;
    push_ms = 0;
}

/* 返回 1 表示 4 步全部完成 */
uint8_t cal_process(void)
{
    int16_t ax, ay;
    input_get_raw(&ax, &ay);

    /* 等待松开 (回到 ADC_LO~ADC_HI 死区) */
    if (wait_release) {
        if (ax > ADC_LO && ax < ADC_HI && ay > ADC_LO && ay < ADC_HI)
            wait_release = 0;
        return 0;
    }

    int moved = (ax < ADC_LO || ax > ADC_HI || ay < ADC_LO || ay > ADC_HI);
    if (!moved) {
        push_ms = 0;            /* 回到中位 → 复位计时 */
        return 0;
    }

    if (push_ms == 0) {
        push_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;   /* 开始计时 */
    } else if ((xTaskGetTickCount() * portTICK_PERIOD_MS - push_ms) >= CAL_PUSH_MS) {
        /* 已保持 300ms，采样记录 */
        raw[step][0] = ax;
        raw[step][1] = ay;
        step++;
        if (step >= 4) {
            input_calibrate(raw);     /* 推导校准参数 */
            input_save_cal();         /* 持久化到 NVS */
            return 1;                 /* 完成 */
        }
        push_ms = 0;
        wait_release = 1;             /* 准备下一步 */
        cal_draw();
    }
    return 0;
}
