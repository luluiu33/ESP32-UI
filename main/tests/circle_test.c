#include "circle_test.h"
#include "input.h"
#include "ssd1306.h"

#define CIRCLE_R      22
#define CENTER_X      64
#define CENTER_Y      40
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

    int px = ox * CIRCLE_R / 2048;
    int py = oy * CIRCLE_R / 2048;

    if (px < -CIRCLE_R) px = -CIRCLE_R;
    if (px >  CIRCLE_R) px =  CIRCLE_R;
    if (py < -CIRCLE_R) py = -CIRCLE_R;
    if (py >  CIRCLE_R) py =  CIRCLE_R;

    int d_sq = px * px + py * py;
    int r_sq = CIRCLE_R * CIRCLE_R;
    if (d_sq > r_sq) {
        int d = 0;
        while (d * d < d_sq) d++;
        if (d > 0) {
            px = px * CIRCLE_R / d;
            py = py * CIRCLE_R / d;
        }
    }

    if (px == prev_px && py == prev_py)
        return;
    prev_px = px;
    prev_py = py;

    ssd1306_clear();
    ssd1306_draw_string(34, 0, "Circle");
    ssd1306_draw_circle(CENTER_X, CENTER_Y, CIRCLE_R, 0);
    ssd1306_draw_circle(CENTER_X, CENTER_Y, DOT_CENTER_R, 1);
    ssd1306_draw_circle(CENTER_X + px, CENTER_Y + py, DOT_R, 1);
    ssd1306_update();
}
