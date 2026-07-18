#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "input.h"

void ui_init(void);
uint8_t ui_is_test(void);
void ui_process(input_event_t ev, joy_state_t *js);
void ui_exit_test(void);

#endif
