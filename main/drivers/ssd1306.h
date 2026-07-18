#ifndef SSD1306_H
#define SSD1306_H

#include "stdint.h"

#define SSD1306_I2C_ADDR    0x3C
#define SSD1306_WIDTH        128
#define SSD1306_HEIGHT       64

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_update(void);
void ssd1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void ssd1306_draw_hline(uint8_t x, uint8_t y, uint8_t w);
void ssd1306_draw_vline(uint8_t x, uint8_t y, uint8_t h);
void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill);
void ssd1306_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill);
void ssd1306_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ssd1306_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data);
void ssd1306_draw_char(uint8_t x, uint8_t y, char c);
void ssd1306_draw_string(uint8_t x, uint8_t y, const char *str);
void ssd1306_set_fps(uint8_t fps);
uint32_t ssd1306_get_frames(void);
void ssd1306_update_area(uint8_t p0, uint8_t p1);
void ssd1306_clear_pages(uint8_t p0, uint8_t p1);

#endif
