#include "input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"

#define JOY_X       ADC1_CHANNEL_6
#define JOY_Y       ADC1_CHANNEL_7
#define BTN_GPIO    GPIO_NUM_32

#define ADC_LO      1600
#define ADC_HI      2400
#define DEBOUNCE_MS 50

typedef enum { JS_IDLE, JS_X_LEFT, JS_X_RIGHT, JS_Y_UP, JS_Y_DOWN } joy_axis_t;

void input_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(JOY_X, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(JOY_Y, ADC_ATTEN_DB_11);

    gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_GPIO, GPIO_PULLUP_ONLY);
}

static uint32_t last_btn_ms = 0;
static joy_axis_t js = JS_IDLE;

input_event_t input_read(void)
{
    input_event_t ev = EV_NONE;
    int ax = adc1_get_raw(JOY_X);
    int ay = adc1_get_raw(JOY_Y);
    int btn = gpio_get_level(BTN_GPIO);

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (!btn && (now - last_btn_ms > DEBOUNCE_MS)) {
        last_btn_ms = now;
        ev = EV_CONFIRM;
    }

    switch (js) {
        case JS_IDLE:
            if (ax < ADC_LO)           { js = JS_X_LEFT;  ev = EV_LEFT; }
            else if (ax > ADC_HI)      { js = JS_X_RIGHT; ev = EV_RIGHT; }
            else if (ay < ADC_LO)      { js = JS_Y_UP;    ev = EV_UP; }
            else if (ay > ADC_HI)      { js = JS_Y_DOWN;  ev = EV_DOWN; }
            break;
        case JS_X_LEFT:
            if (ax >= ADC_LO) js = JS_IDLE;
            break;
        case JS_X_RIGHT:
            if (ax <= ADC_HI) js = JS_IDLE;
            break;
        case JS_Y_UP:
            if (ay >= ADC_LO) js = JS_IDLE;
            break;
        case JS_Y_DOWN:
            if (ay <= ADC_HI) js = JS_IDLE;
            break;
    }

    return ev;
}

joy_state_t input_get_state(void)
{
    joy_state_t s;
    int ax = adc1_get_raw(JOY_X);
    int ay = adc1_get_raw(JOY_Y);
    s.left   = (ax < ADC_LO);
    s.right  = (ax > ADC_HI);
    s.up     = (ay < ADC_LO);
    s.down   = (ay > ADC_HI);
    s.center = !gpio_get_level(BTN_GPIO);
    return s;
}
