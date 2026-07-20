/* ============================================================
 *  ssd1306.h — SSD1306 OLED 驱动 (I2C, 128×64)
 *
 *  单色 1-bit 帧缓冲，1KB (8 pages × 128 bytes)。
 *  所有绘图操作先写入 framebuf，ssd1306_update() 统一提交。
 * ============================================================ */
#ifndef SSD1306_H
#define SSD1306_H

#include "stdint.h"

/* I2C 地址及分辨率 */
#define SSD1306_I2C_ADDR    0x3C
#define SSD1306_WIDTH        128
#define SSD1306_HEIGHT       64

/* ---- 生命周期 ---- */
void ssd1306_init(void);                    /* 硬件复位 + 初始化序列 */
void ssd1306_clear(void);                   /* 清空帧缓冲 */
void ssd1306_update(void);                  /* 全帧缓冲 → I2C 发送 */
void ssd1306_update_area(uint8_t p0, uint8_t p1);  /* 局部刷新（页面范围） */
void ssd1306_clear_pages(uint8_t p0, uint8_t p1);  /* 清除指定页 */

/* ---- 绘图 (color=1 白, 0 黑) ---- */
void ssd1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void ssd1306_draw_hline(uint8_t x, uint8_t y, uint8_t w);
void ssd1306_draw_vline(uint8_t x, uint8_t y, uint8_t h);
void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill);
void ssd1306_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill);
void ssd1306_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ssd1306_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data);

/* ---- 文字 ---- */
void ssd1306_draw_char(uint8_t x, uint8_t y, char c);
void ssd1306_draw_string(uint8_t x, uint8_t y, const char *str);

/* ---- 性能 ---- */
void ssd1306_set_fps(uint8_t fps);         /* 预留 */
uint32_t ssd1306_get_frames(void);         /* 获取帧计数（自上次调用起） */

#endif
