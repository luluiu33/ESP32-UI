#include "circle_test.h"
#include "input.h"
#include "display.h"
#include "display_layout.h"

#if defined(CONFIG_DISPLAY_OLED) && defined(CONFIG_DISPLAY_LCD)
  #include "ssd1306.h"
  #include "lcd_st7735.h"
#endif

#define DOT_R          3
#define DOT_CENTER_R   2
#define LCD_DOT_R      6
#define LCD_CENTER_R   4

static int prev_px = 0, prev_py = 0;

void circle_test_reset(void)
{
    prev_px = 1;
    prev_py = 1;
}

static void calc_pos(int16_t *px, int16_t *py, int rad)
{
    int16_t cx, cy;
    input_get_calibrated(&cx, &cy);

    int ox = cx;
    int oy = cy;

    if (ox > -400 && ox < 400) ox = 0;
    if (oy > -400 && oy < 400) oy = 0;

    *px = ox * rad / 2048;
    *py = oy * rad / 2048;

    if (*px < -rad) *px = -rad;
    if (*px >  rad) *px =  rad;
    if (*py < -rad) *py = -rad;
    if (*py >  rad) *py =  rad;

    int d_sq = (*px) * (*px) + (*py) * (*py);
    int r_sq = rad * rad;
    if (d_sq > r_sq) {
        int d = 0;
        while (d * d < d_sq) d++;
        if (d > 0) {
            *px = *px * rad / d;
            *py = *py * rad / d;
        }
    }
}

void circle_test_draw(joy_state_t *js)
{
    (void)js;

    int16_t px, py;
    calc_pos(&px, &py, CIRCLE_RAD);

    if (px == prev_px && py == prev_py)
        return;
    prev_px = px;
    prev_py = py;

    display_clear();

#if defined(CONFIG_DISPLAY_OLED) && defined(CONFIG_DISPLAY_LCD)
    {
        int16_t lcd_px = px * LCD_CIRCLE_RAD / CIRCLE_RAD;
        int16_t lcd_py = py * LCD_CIRCLE_RAD / CIRCLE_RAD;

        ssd1306_draw_circle(OLED_CIRCLE_CX, OLED_CIRCLE_CY, OLED_CIRCLE_RAD, 0);
        ssd1306_draw_circle(OLED_CIRCLE_CX, OLED_CIRCLE_CY, DOT_CENTER_R, 1);
        ssd1306_draw_circle(OLED_CIRCLE_CX + px, OLED_CIRCLE_CY + py, DOT_R, 1);

        lcd_draw_circle(LCD_CIRCLE_CX, LCD_CIRCLE_CY, LCD_CIRCLE_RAD, COLOR_BLACK, 0);
        lcd_draw_circle(LCD_CIRCLE_CX, LCD_CIRCLE_CY, LCD_CENTER_R, COLOR_BLACK, 1);
        lcd_draw_circle(LCD_CIRCLE_CX + lcd_px, LCD_CIRCLE_CY + lcd_py, LCD_DOT_R, COLOR_BLACK, 1);
    }
#else
    display_draw_circle(CIRCLE_CX, CIRCLE_CY, CIRCLE_RAD, 0);
    display_draw_circle(CIRCLE_CX, CIRCLE_CY, DOT_CENTER_R, 1);
    display_draw_circle(CIRCLE_CX + px, CIRCLE_CY + py, DOT_R, 1);
#endif

    display_update();
}
