#ifndef DISPLAY_TEST_H
#define DISPLAY_TEST_H

#include "input.h"

#define DISP_TEST_COUNT 6

void disp_test_enter(int index);
void disp_test_draw(joy_state_t *js);
void disp_test_next(void);
void disp_test_prev(void);

#endif
