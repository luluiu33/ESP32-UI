/* ============================================================
 *  display.h — 统一显示抽象层
 *
 *  所有 UI / 测试代码通过本 API 绘制，不直接操作屏幕硬件。
 *  编译时由 main.h 中的 CONFIG_DISPLAY_OLED / CONFIG_DISPLAY_LCD
 *  选择后端。布局宏见 display_layout.h。
 * ============================================================ */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/* ---- 生命周期 ---- */
void display_init(void);            /* 初始化后端显示屏 */
void display_clear(void);           /* 全屏清除 + 绘制标题栏 */
void display_update(void);          /* 提交帧缓冲到硬件 */

/* ---- 基本图元 (前景色：OLED 白/LCD 黑) ---- */
void display_draw_pixel(uint8_t x, uint8_t y);
void display_draw_hline(uint8_t x, uint8_t y, uint8_t w);
void display_draw_vline(uint8_t x, uint8_t y, uint8_t h);
void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill);
void display_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill);
void display_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void display_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data);

/* ---- 文字 ---- */
void display_draw_char(uint8_t x, uint8_t y, char c);
void display_draw_string(uint8_t x, uint8_t y, const char *str);
void display_printf(uint8_t x, uint8_t y, const char *fmt, ...);

/* ---- 圆角矩形 ---- */
void display_draw_round_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t fill);

/* ---- 标题栏 + FPS ---- */
void display_set_title(const char *title);          /* 设置标题文本 */
void display_update_title(void);                    /* 刷新标题栏（含 FPS） */
void display_set_fps(uint8_t fps);                  /* 手动设定 FPS */
void display_update_fps(int elapsed_ms);            /* 根据 elapsed_ms 自动计算 FPS */
uint32_t display_get_frames(void);                  /* 获取帧计数（自上次调用起） */

#endif
