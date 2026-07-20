/* ============================================================
 *  input.h — 摇杆输入处理 (ADC + 按键)
 *
 *  双轴 ADC (12-bit, 0~4095)，含去抖、边缘检测、校准映射。
 *  校准数据以 blob 形式存储在 NVS (key="cal/map")。
 * ============================================================ */
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/* ADC 死区阈值：1600~2400 为回中区域 */
#define ADC_LO  1600
#define ADC_HI  2400

/* 单次事件类型 (边缘触发) */
typedef enum {
    EV_NONE,        /* 无事件 */
    EV_UP,          /* 摇杆上推 */
    EV_DOWN,        /* 摇杆下拉 */
    EV_LEFT,        /* 摇杆左推 */
    EV_RIGHT,       /* 摇杆右推 */
    EV_CONFIRM,     /* 中心按键按下 (去抖后) */
} input_event_t;

/* 摇杆连续状态 (电平触发，用于 LED 指示和方向保持) */
typedef struct {
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
    uint8_t center;
} joy_state_t;

void input_init(void);                              /* 初始化 ADC + GPIO */
input_event_t input_read(void);                     /* 读取单次事件 */
joy_state_t input_get_state(void);                  /* 读取连续状态 */
void input_get_raw(int16_t *ax, int16_t *ay);       /* 读取原始 ADC 值 */
void input_get_calibrated(int16_t *cx, int16_t *cy); /* 读取校准后值 (±2048) */
void input_calibrate(const int16_t raw[4][2]);       /* 根据 4 方向采样计算校准参数 */
void input_save_cal(void);                           /* 保存校准到 NVS */
void input_load_cal(void);                           /* 从 NVS 加载校准 */

#endif
