/* ============================================================
 *  主程序入口 — ESP32 OLED/LCD 双屏显示项目
 *
 *  功能：初始化硬件 → 主循环（~62.5fps 固定帧率）
 *        每帧：读取摇杆输入 → 更新 UI → LED 指示 → FPS/CPU 统计
 * ============================================================ */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "input.h"
#include "ui.h"
#include "display.h"
#include "display_layout.h"

/* 指示灯 GPIO：摇杆有动作时点亮 */
#define LED_GPIO  GPIO_NUM_2

void app_main(void)
{
    /* ---- 硬件初始化 ---- */
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    nvs_flash_init();                   /* NVS 存储（校准数据） */

    input_init();                       /* ADC + 按键 */
    input_load_cal();                   /* 加载校准参数 */
    ui_init();                          /* 显示屏 + 菜单 */

    /* ---- 性能统计变量 ---- */
    int64_t last = esp_timer_get_time(); /* 上次统计时间点 */
    int64_t work_sum = 0;                /* 累计工作时间 (us) */
    int sample = 0;                      /* 采样帧计数 */

    /* ---- 固定帧率主循环 (16ms = ~62.5fps) ---- */
    while (1) {
        int64_t t0 = esp_timer_get_time();

        /* ① 读取输入 + 更新 UI */
        input_event_t ev = input_read();
        joy_state_t js = input_get_state();
        ui_process(ev, &js);
        /* ② 摇杆有方向或按键时点亮 LED */
        gpio_set_level(LED_GPIO, js.up || js.down || js.left || js.right || js.center);

        /* ③ 统计实际工作时间 */
        int64_t t1 = esp_timer_get_time();
        work_sum += t1 - t0;
        sample++;

        /* ④ 每 50 帧（~0.8s）输出一次 CPU 占用率和 FPS */
        if (sample >= 50) {
            int64_t now = esp_timer_get_time();
            int elapsed_ms = (int)((now - last) / 1000);
            int cpu = (elapsed_ms > 0) ? (int)((int64_t)work_sum * 100 / (elapsed_ms * 1000)) : 0;
            display_update_fps(elapsed_ms);
            printf("CPU:%d%%\n", cpu);
            last = now;
            work_sum = 0;
            sample = 0;
        }

        /* ⑤ 刷新标题栏（含 FPS 显示） */
        display_update_title();

        /* ⑥ 精确填充至 16ms 固定周期 */
        int sleep_us = 16000 - (int)(esp_timer_get_time() - t0);
        if (sleep_us > 10000)
            vTaskDelay(1);               /* 空闲较多，让出 CPU */
        while ((int)(esp_timer_get_time() - t0) < 16000) { } /* 忙等待填充 */
    }
}
