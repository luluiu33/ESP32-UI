/* ============================================================
 *  lcd_st7735.h — ST7735 LCD 驱动 (SPI, 128×160, RGB565)
 *
 *  40KB 帧缓冲 (160 × 128 × 2 bytes)，支持全屏/局部刷新。
 *  所有颜色使用 RGB565 宏或预定义 COLOR_* 常量。
 *  字库复用 font_6x8.h。
 * ============================================================ */
#ifndef LCD_ST7735_H
#define LCD_ST7735_H

#include <stdint.h>

/* ---- 分辨率 ---- */
#define LCD_WIDTH       128
#define LCD_HEIGHT      160

/* ---- SPI 引脚定义 ---- */
#define LCD_SPI_CLK     18
#define LCD_SPI_MOSI    23
#define LCD_RESET       22          /* 硬件复位，低电平有效 */
#define LCD_DC          21          /* 数据/命令选择 (高=数据) */
#define LCD_CS          5           /* 片选，低电平选中 */
#define LCD_SPI_HOST    SPI2_HOST   /* ESP32 SPI2 控制器 */

/* ---- RGB565 色彩工具 ---- */
#define RGB565(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

/* ---- 预定义颜色 ---- */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_ORANGE    0xFC00
#define COLOR_GRAY      0x8410

/* ---- 生命周期 ---- */
void lcd_init(void);                        /* GPIO + SPI + 硬件复位 + 初始化序列 */
void lcd_clear(uint16_t color);             /* 全屏清色 */
void lcd_fill(uint16_t color);             /* 帧缓冲填色（不刷新） */
void lcd_update(void);                      /* 全屏刷新 */
void lcd_update_area(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1); /* 局部刷新 */
void lcd_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);  /* 设置 CASET/RASET */

/* ---- 图元 (color = RGB565) ---- */
void lcd_draw_pixel(uint8_t x, uint8_t y, uint16_t color);
void lcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void lcd_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void lcd_draw_hline(uint8_t x, uint8_t y, uint8_t w, uint16_t color);
void lcd_draw_vline(uint8_t x, uint8_t y, uint8_t h, uint16_t color);
void lcd_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void lcd_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint16_t color, uint8_t fill);
void lcd_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data, uint16_t fg, uint16_t bg);

/* ---- 文字 (复用 font_6x8, fg=前景色, bg=背景色) ---- */
void lcd_draw_char(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg);
void lcd_draw_string(uint8_t x, uint8_t y, const char *str, uint16_t fg, uint16_t bg);
void lcd_draw_stringf(uint8_t x, uint8_t y, uint16_t fg, uint16_t bg, const char *fmt, ...);

/* ---- 性能 ---- */
void lcd_set_fps(uint8_t fps);              /* 预留 */
uint32_t lcd_get_frames(void);              /* 帧计数（自上次调用起） */

#endif
