#include "about.h"
#include "display.h"
#include "display_layout.h"
#include "main.h"

/* ── 文字内容按显示模式区分 ── */
#if defined(CONFIG_DISPLAY_OLED) && !defined(CONFIG_DISPLAY_LCD)
/* OLED only */
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
/* LCD only */
#define ABOUT_LINES    12
static const char *about_text[] = {
    "== ESP32 LCD ==",
    "ST7735S 128x160",
    "SPI 26MHz",
    "Dual display",
    "Joystick + Menu",
    "Calibration",
    "Circle / Button",
    "Display test",
    "Firmware " FW_VERSION_STR,
    "MCU: ESP32-S3",
    "Flash: 16MB",
    "Scroll: Up/Down",
};

#else
/* Dual display — use LCD content but limit rows to OLED content area */
#define ABOUT_LINES    12
static const char *about_text[] = {
    "== ESP32 LCD ==",
    "ST7735S 128x160",
    "SPI 26MHz",
    "Dual display",
    "Joystick + Menu",
    "Calibration",
    "Circle / Button",
    "Display test",
    "Firmware " FW_VERSION_STR,
    "MCU: ESP32-S3",
    "Flash: 16MB",
    "Scroll: Up/Down",
};
#endif

/* ── 可见行数 = 受 OLED 内容区页面数限制 ── */
#define ABOUT_VISIBLE   (CONTENT_H)             /* OLED:6, LCD:18 */

static int scroll_y = 0;

void about_enter(void)
{
    scroll_y = 0;
}

void about_scroll(int8_t dir)
{
    int new_y = scroll_y + dir;
    if (new_y < 0) new_y = 0;
    if (new_y > ABOUT_LINES - ABOUT_VISIBLE)
        new_y = ABOUT_LINES - ABOUT_VISIBLE;
    if (new_y != scroll_y) {
        scroll_y = new_y;
        about_draw();
    }
}

void about_draw(void)
{
    display_clear();
    uint8_t max_page = CONTENT_Y + CONTENT_H;   /* OLED:8, LCD:20 */
    for (int i = 0; i < ABOUT_VISIBLE && (scroll_y + i) < ABOUT_LINES; i++) {
        uint8_t py = ABOUT_TEXT_Y(i);
        if (py < max_page) {
            display_draw_string(8, py, about_text[scroll_y + i]);
        }
    }
    display_update();
}
