#include "menu.h"
#include "ssd1306.h"
#include <string.h>

#define MENU_COUNT   5
#define MENU_VISIBLE 3

static const char *items[MENU_COUNT] = {
    "Circle",
    "Button",
    "Display",
    "Settings",
    "About"
};

static int focus = 0;
static int prev  = -1;
static int scroll_y = 0;

static void menu_update_scroll(void)
{
    if (focus < scroll_y)
        scroll_y = focus;
    else if (focus >= scroll_y + MENU_VISIBLE)
        scroll_y = focus - MENU_VISIBLE + 1;
}

void menu_reset(void)
{
    focus = 0;
    prev  = -1;
    scroll_y = 0;
}

int menu_get_focus(void)
{
    return focus;
}

void menu_focus_up(void)
{
    if (focus > 0) {
        focus--;
        menu_update_scroll();
    }
}

void menu_focus_down(void)
{
    if (focus < MENU_COUNT - 1) {
        focus++;
        menu_update_scroll();
    }
}

int menu_is_dirty(void)
{
    int d = (focus != prev);
    if (d) prev = focus;
    return d;
}

void menu_draw(void)
{
    ssd1306_clear();
    ssd1306_draw_string(28, 0, "Main Menu");
    for (int i = 0; i < MENU_VISIBLE && (scroll_y + i) < MENU_COUNT; i++) {
        int idx = scroll_y + i;
        char buf[20];
        buf[0] = (idx == focus) ? '>' : ' ';
        buf[1] = ' ';
        strcpy(buf + 2, items[idx]);
        ssd1306_draw_string(20, 3 + i * 2, buf);
    }
    ssd1306_update();
}
