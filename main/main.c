#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "input.h"
#include "ui.h"
#include "ssd1306.h"

#define LED_GPIO  GPIO_NUM_2

#define FPS_SAMPLE_COUNT  20

void app_main(void)
{
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    nvs_flash_init();

    input_init();
    input_load_cal();
    ui_init();

    int64_t last = esp_timer_get_time();
    int64_t work_sum = 0;
    int sample = 0;

    while (1) {
        int64_t t0 = esp_timer_get_time();

        input_event_t ev = input_read();
        joy_state_t js = input_get_state();
        ui_process(ev, &js);
        gpio_set_level(LED_GPIO, js.up || js.down || js.left || js.right || js.center);

        int64_t t1 = esp_timer_get_time();
        work_sum += t1 - t0;
        sample++;

        if (sample >= 50) {
            int64_t now = esp_timer_get_time();
            int elapsed_ms = (int)((now - last) / 1000);
            int fps = ssd1306_get_frames() * 1000 / (elapsed_ms ? elapsed_ms : 1);
            int cpu = (elapsed_ms > 0) ? (int)((int64_t)work_sum * 100 / (elapsed_ms * 1000)) : 0;
            ssd1306_set_fps(fps > 99 ? 99 : (uint8_t)fps);
            printf("CPU:%d%%  FPS:%d\n", cpu, fps);
            last = now;
            work_sum = 0;
            sample = 0;
        }

        /* fixed 16ms frame period */
        int sleep_us = 16000 - (int)(esp_timer_get_time() - t0);
        if (sleep_us > 10000)
            vTaskDelay(1);
        while ((int)(esp_timer_get_time() - t0) < 16000) { }
    }
}