/* ============================================================
 *  about.c — 关于页面实现
 *
 *  根据显示屏配置 (OLED / LCD / 双屏) 编译不同的文字内容。
 *  支持 Up/Down 滚动，仅 scroll_y 变化时重绘。
 * ============================================================ */
#include "about.h"
#include "display.h"
#include "display_layout.h"
#include "main.h"

#if defined(CONFIG_DISPLAY_OLED) && defined(CONFIG_DISPLAY_LCD)
  #include "ssd1306.h"
  #include "lcd_st7735.h"
#endif

/* ---- 三套文字内容（取决于显示屏配置） ---- */
#if defined(CONFIG_DISPLAY_OLED) && !defined(CONFIG_DISPLAY_LCD)
/* OLED 单屏 — 8 行 */
#define ABOUT_LINES    8
static const char *about_text[] = {
    "== ESP32 OLED ==",
    "SSD1306 128x64",
    "Joystick + Menu",
    "Calibration",
    "Direction Test",
    "Button indicator",
    "Firmware " FW_VERSION_STR,
    "Scroll: Up/Down",
};

#elif defined(CONFIG_DISPLAY_LCD) && !defined(CONFIG_DISPLAY_OLED)
/* LCD 单屏 — 10 行 */
#define ABOUT_LINES    10
static const char *about_text[] = {
    "== ESP32 LCD ==",
    "ST7735 128x160",
    "SPI 26MHz",
    "Joystick + Menu",
    "Calibration",
    "Circle / Button",
    "Display test",
    "Firmware " FW_VERSION_STR,
    "Scroll: Up/Down",
};

#else
/* 双屏 — 每块屏单独显示内容 */
#define ABOUT_LINES_OLED  8
static const char *about_text_oled[] = {
    "== ESP32 OLED ==",
    "SSD1306 128x64",
    "I2C 1MHz",
    "Joystick + Menu",
    "Calibration",
    "Direction Test",
    "Firmware " FW_VERSION_STR,
    "Scroll: Up/Down",
};

#define ABOUT_LINES_LCD   9
static const char *about_text_lcd[] = {
    "== ESP32 LCD ==",
    "ST7735 128x160",
    "SPI 26MHz",
    "Dual display",
    "Joystick + Menu",
    "Calibration",
    "Circle / Button",
    "Display test",
    "Firmware " FW_VERSION_STR,
};

#define ABOUT_LINES ABOUT_LINES_OLED
#endif

/* 可见行数 = 受 OLED 内容区页面数限制 */
#define ABOUT_VISIBLE   (CONTENT_H)

static int scroll_y = 0;        /* 当前滚动偏移 (行) */

void about_enter(void)
{
    scroll_y = 0;               /* 进入时重置滚动 */
}

void about_scroll(int8_t dir)
{
    int new_y = scroll_y + dir;
    if (new_y < 0) new_y = 0;
    if (new_y > ABOUT_LINES - ABOUT_VISIBLE)
        new_y = ABOUT_LINES - ABOUT_VISIBLE;
    if (new_y != scroll_y) {
        scroll_y = new_y;
        about_draw();           /* 仅位置变化时刷新 */
    }
}

void about_draw(void)
{
    display_clear();

#if defined(CONFIG_DISPLAY_OLED) && defined(CONFIG_DISPLAY_LCD)
    /* 双屏：各显对应内容 */
    for (int i = 0; i < OLED_CONTENT_H && (scroll_y + i) < ABOUT_LINES_OLED; i++)
        ssd1306_draw_string(8, OLED_CONTENT_Y + i, about_text_oled[scroll_y + i]);
    for (int i = 0; i < LCD_CONTENT_H && i < ABOUT_LINES_LCD; i++)
        lcd_draw_string(8, (LCD_CONTENT_Y + i) * 8, about_text_lcd[i], COLOR_BLACK, COLOR_WHITE);
#elif defined(CONFIG_DISPLAY_OLED)
    uint8_t max_page = CONTENT_Y + CONTENT_H;
    for (int i = 0; i < ABOUT_VISIBLE && (scroll_y + i) < ABOUT_LINES; i++) {
        uint8_t py = ABOUT_TEXT_Y(i);
        if (py < max_page)
            display_draw_string(8, py, about_text[scroll_y + i]);
    }
#else
    /* LCD 单屏 */
    for (int i = 0; i < ABOUT_VISIBLE && i < ABOUT_LINES; i++)
        display_draw_string(8, ABOUT_TEXT_Y(i), about_text[i]);
#endif

    display_update();
}
