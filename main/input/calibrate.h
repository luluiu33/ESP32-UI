#ifndef CALIBRATE_H
#define CALIBRATE_H

#include <stdint.h>

void cal_init(void);
void cal_draw(void);
uint8_t cal_process(void);
void cal_cancel(void);

#endif
