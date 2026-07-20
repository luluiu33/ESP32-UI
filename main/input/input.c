/* ============================================================
 *  input.c — 摇杆输入处理实现
 *
 *  双轴 ADC (GPIO34/35, ADC1_CH6/CH7) + 按键 (GPIO32, 低有效)。
 *  校准参数 (swap_xy / invert_x / invert_y) 通过 4 方向采样自动
 *  推导，以 3 字节 blob 形式保存在 NVS 命名空间 "cal"。
 *  摇杆事件为边缘触发 (从回中到偏离时产生一次)，按键软件去抖。
 * ============================================================ */
#include "input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"

/* ---- 硬件引脚 ---- */
#define JOY_X       ADC1_CHANNEL_6  /* GPIO34 */
#define JOY_Y       ADC1_CHANNEL_7  /* GPIO35 */
#define BTN_GPIO    GPIO_NUM_32     /* 低电平有效 */
#define DEBOUNCE_MS 50              /* 按键去抖时间 */

/* 摇杆方向状态机：当前偏离方向，用于边缘检测 */
typedef enum { JS_IDLE, JS_LEFT, JS_RIGHT, JS_UP, JS_DOWN } joy_axis_t;

/* ---- 校准参数 ---- */
static uint8_t swap_xy  = 0;    /* X / Y 轴交换 */
static uint8_t invert_x = 0;    /* X 方向取反 */
static uint8_t invert_y = 0;    /* Y 方向取反 */

/* ---- 运行时状态 ---- */
static uint32_t last_btn_ms = 0;        /* 上次按键触发时间 */
static joy_axis_t js = JS_IDLE;         /* 当前偏差方向 (用于回中检测) */

void input_init(void)
{
    /* 12 位分辨率 (0~4095)，衰减 11dB 以支持 0~3.3V */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(JOY_X, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(JOY_Y, ADC_ATTEN_DB_11);

    /* 按键上拉输入 */
    gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_GPIO, GPIO_PULLUP_ONLY);
}

/* 读取原始 ADC 值 (0~4095) */
void input_get_raw(int16_t *ax, int16_t *ay)
{
    *ax = adc1_get_raw(JOY_X);
    *ay = adc1_get_raw(JOY_Y);
}

/* 读取校准后值 (±2048 范围)
 * 校准后：LR=0 为左极限，UD=0 为上极限 */
void input_get_calibrated(int16_t *cx, int16_t *cy)
{
    int ax = adc1_get_raw(JOY_X);
    int ay = adc1_get_raw(JOY_Y);

    int lr_raw = swap_xy ? ay : ax;
    int ud_raw = swap_xy ? ax : ay;

    *cx = invert_x ? (2048 - lr_raw) : (lr_raw - 2048);
    *cy = invert_y ? (2048 - ud_raw) : (ud_raw - 2048);
}

/* 根据 4 个方向的原始采样推导校准参数 */
void input_calibrate(const int16_t raw[4][2])
{
    int dx_ud = raw[0][0] - raw[1][0]; if (dx_ud < 0) dx_ud = -dx_ud;
    int dy_ud = raw[0][1] - raw[1][1]; if (dy_ud < 0) dy_ud = -dy_ud;
    int dx_lr = raw[2][0] - raw[3][0]; if (dx_lr < 0) dx_lr = -dx_lr;
    int dy_lr = raw[2][1] - raw[3][1]; if (dy_lr < 0) dy_lr = -dy_lr;

    /* 若上下方向在 X 轴上变化更大，左右在 Y 轴上变化更大，则交换 */
    swap_xy = (dx_ud > dy_ud && dx_lr < dy_lr);

    int lr_left  = swap_xy ? raw[2][1] : raw[2][0];
    int lr_right = swap_xy ? raw[3][1] : raw[3][0];
    invert_x = (lr_left > lr_right);    /* 左极限值 > 右极限 → 取反 */

    int ud_up   = swap_xy ? raw[0][0] : raw[0][1];
    int ud_down = swap_xy ? raw[1][0] : raw[1][1];
    invert_y = (ud_up > ud_down);       /* 上极限值 > 下极限 → 取反 */

    js = JS_IDLE;
}

/* 读取单次输入事件 (边缘触发) */
input_event_t input_read(void)
{
    input_event_t ev = EV_NONE;
    int ax = adc1_get_raw(JOY_X);
    int ay = adc1_get_raw(JOY_Y);
    int btn = gpio_get_level(BTN_GPIO);

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    /* 按键去抖 (下降沿) */
    if (!btn && (now - last_btn_ms > DEBOUNCE_MS)) {
        last_btn_ms = now;
        ev = EV_CONFIRM;
    }

    /* 校准映射 */
    int lr_raw = swap_xy ? ay : ax;
    int ud_raw = swap_xy ? ax : ay;

    /* 摇杆方向状态机 (边缘触发) */
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

/* 读取连续输入状态 (电平触发，用于 LED 指示和方向保持) */
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

/* ---- NVS 校准持久化 ---- */
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
