#include "display.h"
#include "main.h"

#if !defined(CONFIG_DISPLAY_OLED) && !defined(CONFIG_DISPLAY_LCD)
#error "At least one display type must be selected: run 'idf.py menuconfig' and enable OLED and/or LCD"
#endif

#ifdef CONFIG_DISPLAY_OLED
#include "ssd1306.h"
#endif
#ifdef CONFIG_DISPLAY_LCD
#include "lcd_st7735.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#define TITLE_BAR_PAGES 2
#define STATUS_BAR_H 16

static char title_buf[16] = "Main Menu";
#ifdef CONFIG_DISPLAY_OLED
static uint8_t fps_val = 0;
#endif
#ifdef CONFIG_DISPLAY_LCD
static char lcd_fps_buf[12] = "FPS:--";
#endif
static bool title_dirty = true;

void display_init(void)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_init();
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_init();
#endif
}

void display_clear(void)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_clear();
    char fps_str[8];
    snprintf(fps_str, sizeof(fps_str), "FPS:%u", fps_val > 99 ? 99 : fps_val);
    uint8_t fx = 128 - strlen(fps_str) * 6;
    ssd1306_draw_string(0, 0, title_buf);
    ssd1306_draw_string(fx, 0, fps_str);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_fill(COLOR_WHITE);
    lcd_fill_rect(0, 0, LCD_WIDTH, STATUS_BAR_H, COLOR_BLACK);
    lcd_draw_string(0, 4, title_buf, COLOR_WHITE, COLOR_BLACK);
    uint8_t lcd_fx = LCD_WIDTH - strlen(lcd_fps_buf) * 6;
    lcd_draw_string(lcd_fx, 4, lcd_fps_buf, COLOR_WHITE, COLOR_BLACK);
#endif
    title_dirty = true;
}

void display_update(void)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_update();
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_update();
#endif
}

void display_set_title(const char *title)
{
    strncpy(title_buf, title, sizeof(title_buf) - 1);
    title_buf[sizeof(title_buf) - 1] = '\0';
    title_dirty = true;
}

void display_update_title(void)
{
    if (!title_dirty) return;
    title_dirty = false;
#ifdef CONFIG_DISPLAY_OLED
    char fps_str[8];
    snprintf(fps_str, sizeof(fps_str), "FPS:%u", fps_val > 99 ? 99 : fps_val);
    uint8_t fx = 128 - strlen(fps_str) * 6;
    ssd1306_clear_pages(0, TITLE_BAR_PAGES - 1);
    ssd1306_draw_string(0, 0, title_buf);
    ssd1306_draw_string(fx, 0, fps_str);
    ssd1306_update_area(0, TITLE_BAR_PAGES - 1);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_fill_rect(0, 0, LCD_WIDTH, STATUS_BAR_H, COLOR_BLACK);
    lcd_draw_string(0, 4, title_buf, COLOR_WHITE, COLOR_BLACK);
    uint8_t lcd_fx = LCD_WIDTH - strlen(lcd_fps_buf) * 6;
    lcd_draw_string(lcd_fx, 4, lcd_fps_buf, COLOR_WHITE, COLOR_BLACK);
    lcd_update_area(0, 0, LCD_WIDTH - 1, STATUS_BAR_H - 1);
#endif
}

void display_draw_pixel(uint8_t x, uint8_t y)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_pixel(x, y, 1);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_pixel(x, y, COLOR_BLACK);
#endif
}

void display_draw_hline(uint8_t x, uint8_t y, uint8_t w)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_hline(x, y, w);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_hline(x, y, w, COLOR_BLACK);
#endif
}

void display_draw_vline(uint8_t x, uint8_t y, uint8_t h)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_vline(x, y, h);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_vline(x, y, h, COLOR_BLACK);
#endif
}

void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_rect(x, y, w, h, fill);
#endif
#ifdef CONFIG_DISPLAY_LCD
    if (fill)
        lcd_fill_rect(x, y, w, h, COLOR_BLACK);
    else
        lcd_draw_rect(x, y, w, h, COLOR_BLACK);
#endif
}

void display_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_circle(cx, cy, r, fill);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_circle(cx, cy, r, COLOR_BLACK, fill);
#endif
}

void display_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_line(x0, y0, x1, y1);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_line(x0, y0, x1, y1, COLOR_BLACK);
#endif
}

void display_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_bitmap(x, y, w, h, data);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_bitmap(x, y, w, h, data, COLOR_BLACK, COLOR_WHITE);
#endif
}

void display_draw_char(uint8_t x, uint8_t y, char c)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_char(x, y, c);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_char(x, y * 8, c, COLOR_BLACK, COLOR_WHITE);
#endif
}

void display_draw_string(uint8_t x, uint8_t y, const char *str)
{
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_string(x, y, str);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_string(x, y * 8, str, COLOR_BLACK, COLOR_WHITE);
#endif
}

void display_printf(uint8_t x, uint8_t y, const char *fmt, ...)
{
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
#ifdef CONFIG_DISPLAY_OLED
    ssd1306_draw_string(x, y, buf);
#endif
#ifdef CONFIG_DISPLAY_LCD
    lcd_draw_string(x, y * 8, buf, COLOR_BLACK, COLOR_WHITE);
#endif
}

static int int_sqrt(int n)
{
    int s = 0;
    while ((s + 1) * (s + 1) <= n) s++;
    return s;
}

void display_draw_round_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t fill)
{
    if (r > w / 2 || r > h / 2) r = (w < h ? w : h) / 2;
    if (fill) {
        display_draw_rect(x + r, y, w - 2 * r, h, 1);
        display_draw_rect(x, y + r, r, h - 2 * r, 1);
        display_draw_rect(x + w - r, y + r, r, h - 2 * r, 1);
        for (int dy = 0; dy < r; dy++) {
            int len = int_sqrt(r * r - (r - 1 - dy) * (r - 1 - dy));
            for (int c = 1; c <= len; c++) {
                display_draw_pixel(x + r - c, y + dy);
                display_draw_pixel(x + w - r - 1 + c, y + dy);
                display_draw_pixel(x + r - c, y + h - 1 - dy);
                display_draw_pixel(x + w - r - 1 + c, y + h - 1 - dy);
            }
        }
    } else {
        display_draw_hline(x + r, y, w - 2 * r);
        display_draw_hline(x + r, y + h - 1, w - 2 * r);
        display_draw_vline(x, y + r, h - 2 * r);
        display_draw_vline(x + w - 1, y + r, h - 2 * r);
        for (int dx = 0; dx <= r; dx++) {
            int dy = int_sqrt(r * r - dx * dx);
            display_draw_pixel(x + r - dx, y + r - dy);
            display_draw_pixel(x + w - r - 1 + dx, y + r - dy);
            display_draw_pixel(x + r - dx, y + h - r - 1 + dy);
            display_draw_pixel(x + w - r - 1 + dx, y + h - r - 1 + dy);
        }
    }
}

void display_set_fps(uint8_t fps)
{
#ifdef CONFIG_DISPLAY_OLED
    fps_val = fps;
#endif
#ifdef CONFIG_DISPLAY_LCD
    if (fps > 99) fps = 99;
    snprintf(lcd_fps_buf, sizeof(lcd_fps_buf), "FPS:%u", fps);
#endif
}

void display_update_fps(int elapsed_ms)
{
#ifdef CONFIG_DISPLAY_OLED
    {
        int f = ssd1306_get_frames() * 1000 / (elapsed_ms ? elapsed_ms : 1);
        if (f > 99) f = 99;
        if ((uint8_t)f != fps_val) {
            fps_val = (uint8_t)f;
            title_dirty = true;
        }
    }
#endif
#ifdef CONFIG_DISPLAY_LCD
    {
        int f = lcd_get_frames() * 1000 / (elapsed_ms ? elapsed_ms : 1);
        if (f > 99) f = 99;
        char tmp[12];
        snprintf(tmp, sizeof(tmp), "FPS:%u", (uint8_t)f);
        if (strcmp(tmp, lcd_fps_buf) != 0) {
            memcpy(lcd_fps_buf, tmp, sizeof(lcd_fps_buf));
            title_dirty = true;
        }
    }
#endif
}

uint32_t display_get_frames(void)
{
    uint32_t sum = 0;
#ifdef CONFIG_DISPLAY_OLED
    sum += ssd1306_get_frames();
#endif
#ifdef CONFIG_DISPLAY_LCD
    sum += lcd_get_frames();
#endif
    return sum;
}
