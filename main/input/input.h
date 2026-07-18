#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#define ADC_LO  1600
#define ADC_HI  2400

typedef enum {
    EV_NONE,
    EV_UP,
    EV_DOWN,
    EV_LEFT,
    EV_RIGHT,
    EV_CONFIRM,
} input_event_t;

typedef struct {
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
    uint8_t center;
} joy_state_t;

void input_init(void);
input_event_t input_read(void);
joy_state_t input_get_state(void);
void input_get_raw(int16_t *ax, int16_t *ay);
void input_get_calibrated(int16_t *cx, int16_t *cy);
void input_calibrate(const int16_t raw[4][2]);
void input_save_cal(void);
void input_load_cal(void);

#endif
