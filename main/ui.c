#include "ui.h"
#include "ssd1306.h"
#include <string.h>

#define CX         64
#define CY         32
#define GAP        22
#define ARROW_SZ   7
#define CIRCLE_R   7

#define MAP_UP     2
#define MAP_DOWN   3
#define MAP_LEFT   1
#define MAP_RIGHT  0

static const uint8_t pos[5][2] = {
    { CX,       CY - GAP },
    { CX,       CY + GAP },
    { CX - GAP, CY },
    { CX + GAP, CY },
    { CX,       CY },
};

static uint8_t prev_mask = 0xFF;
static uint8_t test_mode = 0;
static int menu_focus = 0;
static int prev_focus = -1;

static const char *menu_items[3] = {
    "Cross Test",
    "Settings",
    "About"
};

static void draw_arrow(uint8_t cx, uint8_t cy, uint8_t dir, uint8_t fill)
{
    int s = ARROW_SZ, x[3], y[3];
    switch (dir) {
        case 0: x[0]=cx;   y[0]=cy-s; x[1]=cx-s; y[1]=cy+s; x[2]=cx+s; y[2]=cy+s; break;
        case 1: x[0]=cx;   y[0]=cy+s; x[1]=cx-s; y[1]=cy-s; x[2]=cx+s; y[2]=cy-s; break;
        case 2: x[0]=cx-s; y[0]=cy;   x[1]=cx+s; y[1]=cy-s; x[2]=cx+s; y[2]=cy+s; break;
        case 3: x[0]=cx+s; y[0]=cy;   x[1]=cx-s; y[1]=cy-s; x[2]=cx-s; y[2]=cy+s; break;
    }
    for (int i = 0; i < 3; i++)
        ssd1306_draw_line(x[i], y[i], x[(i+1)%3], y[(i+1)%3]);

    if (fill) {
        int mn = y[0], mx = y[0];
        for (int i = 1; i < 3; i++) {
            if (y[i] < mn) mn = y[i];
            if (y[i] > mx) mx = y[i];
        }
        int step = 2;
        for (int yy = mn + 1; yy < mx; yy += step) {
            int xl = 255, xr = 0;
            for (int i = 0; i < 3; i++) {
                int j = (i + 1) % 3;
                if ((y[i] <= yy && y[j] > yy) || (y[j] <= yy && y[i] > yy)) {
                    int xi = x[i] + (yy - y[i]) * (x[j] - x[i]) / (y[j] - y[i]);
                    if (xi < xl) xl = xi;
                    if (xi > xr) xr = xi;
                }
            }
            if (xl <= xr) ssd1306_draw_hline(xl, yy, xr - xl + 1);
        }
    }
}

void ui_init(void)
{
    ssd1306_init();
}

uint8_t ui_is_test(void)
{
    return test_mode;
}

static void draw_menu(void)
{
    ssd1306_clear();
    for (int i = 0; i < 3; i++) {
        char buf[20];
        buf[0] = (i == menu_focus) ? '>' : ' ';
        buf[1] = ' ';
        strcpy(buf + 2, menu_items[i]);
        ssd1306_draw_string(20, 2 + i * 2, buf);
    }
    ssd1306_update();
}

static void draw_test(joy_state_t *state)
{
    uint8_t mask = state->up | (state->down << 1) | (state->left << 2) |
                   (state->right << 3) | (state->center << 4);
    if (mask == prev_mask) return;
    prev_mask = mask;

    uint8_t arrows[4];
    arrows[MAP_UP]    = state->up;
    arrows[MAP_DOWN]  = state->down;
    arrows[MAP_LEFT]  = state->left;
    arrows[MAP_RIGHT] = state->right;

    ssd1306_clear();

    for (int i = 0; i < 4; i++)
        draw_arrow(pos[i][0], pos[i][1], i, arrows[i]);
    ssd1306_draw_circle(pos[4][0], pos[4][1], CIRCLE_R, state->center);

    ssd1306_update();
}

void ui_process(input_event_t ev, joy_state_t *js)
{
    if (!test_mode) {
        if (ev == EV_UP && menu_focus > 0)
            menu_focus--;
        else if (ev == EV_DOWN && menu_focus < 2)
            menu_focus++;
        else if (ev == EV_RIGHT && menu_focus == 0) {
            test_mode = 1;
            prev_mask = 0xFF;
            ssd1306_clear();
            return;
        }

        if (menu_focus != prev_focus) {
            prev_focus = menu_focus;
            draw_menu();
        }
    } else {
        draw_test(js);
    }
}
