#include "circle_test.h"
#include "input.h"
#include "display.h"
#include "display_layout.h"

#define DOT_R          3
#define DOT_CENTER_R   2

static int prev_px = 0, prev_py = 0;

void circle_test_reset(void)
{
    prev_px = 1;
    prev_py = 1;
}

void circle_test_draw(joy_state_t *js)
{
    (void)js;

    int16_t cx, cy;
    input_get_calibrated(&cx, &cy);

    int ox = cx;
    int oy = cy;

    if (ox > -400 && ox < 400) ox = 0;
    if (oy > -400 && oy < 400) oy = 0;

    int px = ox * CIRCLE_RAD / 2048;
    int py = oy * CIRCLE_RAD / 2048;

    if (px < -CIRCLE_RAD) px = -CIRCLE_RAD;
    if (px >  CIRCLE_RAD) px =  CIRCLE_RAD;
    if (py < -CIRCLE_RAD) py = -CIRCLE_RAD;
    if (py >  CIRCLE_RAD) py =  CIRCLE_RAD;

    int d_sq = px * px + py * py;
    int r_sq = CIRCLE_RAD * CIRCLE_RAD;
    if (d_sq > r_sq) {
        int d = 0;
        while (d * d < d_sq) d++;
        if (d > 0) {
            px = px * CIRCLE_RAD / d;
            py = py * CIRCLE_RAD / d;
        }
    }

    if (px == prev_px && py == prev_py)
        return;
    prev_px = px;
    prev_py = py;

    display_clear();
    display_draw_circle(CIRCLE_CX, CIRCLE_CY, CIRCLE_RAD, 0);
    display_draw_circle(CIRCLE_CX, CIRCLE_CY, DOT_CENTER_R, 1);
    display_draw_circle(CIRCLE_CX + px, CIRCLE_CY + py, DOT_R, 1);
    display_update();
}
