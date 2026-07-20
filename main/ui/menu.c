/* ============================================================
 *  menu.c — 滚动菜单实现
 *
 *  5 个菜单项，每次最多显示 MENU_VISIBLE 项。
 *  当焦点滚动到可见区域外时自动平移 scroll_y。
 *  惰性重绘：menu_is_dirty() 仅当焦点变化时返回 1。
 * ============================================================ */
#include "menu.h"
#include "display.h"
#include "display_layout.h"
#include <string.h>

#define MENU_COUNT   5

/* 菜单项文本 */
static const char *items[MENU_COUNT] = {
    "Circle",
    "Button",
    "Display",
    "Snake",
    "About"
};

static int focus = 0;       /* 当前焦点索引 */
static int prev  = -1;      /* 上一次焦点 (用于脏检查) */
static int scroll_y = 0;    /* 滚动偏移 */

/* 确保焦点在可见区域内，否则平移 scroll_y */
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

/* 脏检查：若焦点变化返回 1，并记录当前焦点 */
int menu_is_dirty(void)
{
    int d = (focus != prev);
    if (d) prev = focus;
    return d;
}

/* 全屏绘制菜单 */
void menu_draw(void)
{
    display_clear();
    for (int i = 0; i < MENU_VISIBLE && (scroll_y + i) < MENU_COUNT; i++) {
        int idx = scroll_y + i;
        char buf[20];
        buf[0] = (idx == focus) ? '>' : ' ';   /* 焦点指示 */
        buf[1] = ' ';
        strcpy(buf + 2, items[idx]);
        display_draw_string(20, MENU_ITEM_Y(i), buf);
    }
    display_update();
}
