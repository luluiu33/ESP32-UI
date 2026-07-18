#include "about.h"
#include "ssd1306.h"
#include "version.h"

#define ABOUT_LINES  8
#define ABOUT_VISIBLE 8

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
    ssd1306_clear();
    for (int i = 0; i < ABOUT_VISIBLE && (scroll_y + i) < ABOUT_LINES; i++) {
        ssd1306_draw_string(8, i, about_text[scroll_y + i]);
    }
    ssd1306_update();
}
