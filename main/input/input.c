#include "input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"

#define JOY_X       ADC1_CHANNEL_6
#define JOY_Y       ADC1_CHANNEL_7
#define BTN_GPIO    GPIO_NUM_32
#define DEBOUNCE_MS 50

typedef enum { JS_IDLE, JS_LEFT, JS_RIGHT, JS_UP, JS_DOWN } joy_axis_t;

static uint8_t swap_xy  = 0;
static uint8_t invert_x = 0;
static uint8_t invert_y = 0;

static uint32_t last_btn_ms = 0;
static joy_axis_t js = JS_IDLE;

void input_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(JOY_X, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(JOY_Y, ADC_ATTEN_DB_11);

    gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_GPIO, GPIO_PULLUP_ONLY);
}

void input_get_raw(int16_t *ax, int16_t *ay)
{
    *ax = adc1_get_raw(JOY_X);
    *ay = adc1_get_raw(JOY_Y);
}

void input_get_calibrated(int16_t *cx, int16_t *cy)
{
    int ax = adc1_get_raw(JOY_X);
    int ay = adc1_get_raw(JOY_Y);

    int lr_raw = swap_xy ? ay : ax;
    int ud_raw = swap_xy ? ax : ay;

    *cx = invert_x ? (2048 - lr_raw) : (lr_raw - 2048);
    *cy = invert_y ? (2048 - ud_raw) : (ud_raw - 2048);
}

void input_calibrate(const int16_t raw[4][2])
{
    int dx_ud = raw[0][0] - raw[1][0]; if (dx_ud < 0) dx_ud = -dx_ud;
    int dy_ud = raw[0][1] - raw[1][1]; if (dy_ud < 0) dy_ud = -dy_ud;
    int dx_lr = raw[2][0] - raw[3][0]; if (dx_lr < 0) dx_lr = -dx_lr;
    int dy_lr = raw[2][1] - raw[3][1]; if (dy_lr < 0) dy_lr = -dy_lr;

    swap_xy = (dx_ud > dy_ud && dx_lr < dy_lr);

    int lr_left  = swap_xy ? raw[2][1] : raw[2][0];
    int lr_right = swap_xy ? raw[3][1] : raw[3][0];
    invert_x = (lr_left > lr_right);

    int ud_up   = swap_xy ? raw[0][0] : raw[0][1];
    int ud_down = swap_xy ? raw[1][0] : raw[1][1];
    invert_y = (ud_up > ud_down);

    js = JS_IDLE;
}

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

    int lr_raw = swap_xy ? ay : ax;
    int ud_raw = swap_xy ? ax : ay;

    switch (js) {
        case JS_IDLE:
            if (invert_x) {
                if (lr_raw > ADC_HI)           { js = JS_LEFT;  ev = EV_LEFT; }
                else if (lr_raw < ADC_LO)      { js = JS_RIGHT; ev = EV_RIGHT; }
            } else {
                if (lr_raw < ADC_LO)           { js = JS_LEFT;  ev = EV_LEFT; }
                else if (lr_raw > ADC_HI)      { js = JS_RIGHT; ev = EV_RIGHT; }
            }
            if (invert_y) {
                if (ud_raw > ADC_HI)           { js = JS_UP;    ev = EV_UP; }
                else if (ud_raw < ADC_LO)      { js = JS_DOWN;  ev = EV_DOWN; }
            } else {
                if (ud_raw < ADC_LO)           { js = JS_UP;    ev = EV_UP; }
                else if (ud_raw > ADC_HI)      { js = JS_DOWN;  ev = EV_DOWN; }
            }
            break;
        case JS_LEFT:
            if (invert_x ? (lr_raw <= ADC_HI) : (lr_raw >= ADC_LO)) js = JS_IDLE;
            break;
        case JS_RIGHT:
            if (invert_x ? (lr_raw >= ADC_LO) : (lr_raw <= ADC_HI)) js = JS_IDLE;
            break;
        case JS_UP:
            if (invert_y ? (ud_raw <= ADC_HI) : (ud_raw >= ADC_LO)) js = JS_IDLE;
            break;
        case JS_DOWN:
            if (invert_y ? (ud_raw >= ADC_LO) : (ud_raw <= ADC_HI)) js = JS_IDLE;
            break;
    }

    return ev;
}

joy_state_t input_get_state(void)
{
    int ax = adc1_get_raw(JOY_X);
    int ay = adc1_get_raw(JOY_Y);
    int lr_raw = swap_xy ? ay : ax;
    int ud_raw = swap_xy ? ax : ay;

    joy_state_t s;
    if (invert_x) {
        s.left   = (lr_raw > ADC_HI);
        s.right  = (lr_raw < ADC_LO);
    } else {
        s.left   = (lr_raw < ADC_LO);
        s.right  = (lr_raw > ADC_HI);
    }
    if (invert_y) {
        s.up     = (ud_raw > ADC_HI);
        s.down   = (ud_raw < ADC_LO);
    } else {
        s.up     = (ud_raw < ADC_LO);
        s.down   = (ud_raw > ADC_HI);
    }
    s.center = !gpio_get_level(BTN_GPIO);
    return s;
}

static const char *NVS_NS = "cal";
static const char *NVS_KEY = "map";

void input_save_cal(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        uint8_t buf[3] = { swap_xy, invert_x, invert_y };
        nvs_set_blob(h, NVS_KEY, buf, sizeof(buf));
        nvs_commit(h);
        nvs_close(h);
    }
}

void input_load_cal(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        uint8_t buf[3];
        size_t sz = sizeof(buf);
        if (nvs_get_blob(h, NVS_KEY, buf, &sz) == ESP_OK && sz == 3) {
            swap_xy  = buf[0];
            invert_x = buf[1];
            invert_y = buf[2];
        }
        nvs_close(h);
    }
}
