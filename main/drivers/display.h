#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_clear(void);
void display_update(void);
void display_draw_pixel(uint8_t x, uint8_t y);
void display_draw_hline(uint8_t x, uint8_t y, uint8_t w);
void display_draw_vline(uint8_t x, uint8_t y, uint8_t h);
void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill);
void display_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill);
void display_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void display_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data);
void display_draw_char(uint8_t x, uint8_t y, char c);
void display_draw_string(uint8_t x, uint8_t y, const char *str);
void display_set_title(const char *title);
void display_update_title(void);
void display_set_fps(uint8_t fps);
void display_update_fps(int elapsed_ms);
uint32_t display_get_frames(void);

#endif
