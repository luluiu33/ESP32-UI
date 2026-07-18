#include "lcd_ui.h"
#include "lcd_st7735.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MENU_ITEMS  5

static const char *menu[] = {
    "  Start Capture",
    "  Settings",
    "  About",
    "  Run LED Test",
    "  UI Demo",
};

static int focus = 0;

void lcd_ui_init(void)
{
    lcd_init();
}

static void draw_title(void)
{
    lcd_draw_string(20, 0, "=== MENU ===", COLOR_YELLOW, COLOR_BLACK);
}

static void draw_menu(void)
{
    char buf[24];
    for (int i = 0; i < MENU_ITEMS; i++) {
        int y = 2 + i;
        uint16_t fg, bg;
        if (i == focus) {
            buf[0] = '>';
            strcpy(buf + 1, menu[i] + 2);
            strcat(buf, " <");
            fg = COLOR_BLACK;
            bg = COLOR_CYAN;
        } else {
            strcpy(buf, menu[i]);
            fg = COLOR_WHITE;
            bg = COLOR_BLACK;
        }
        lcd_draw_string(0, y * 8, buf, fg, bg);
    }
}

void lcd_ui_draw(void)
{
    lcd_clear(COLOR_BLACK);
    draw_title();
    draw_menu();
}

void lcd_ui_navigate(int dir)
{
    focus += dir;
    if (focus < 0) focus = MENU_ITEMS - 1;
    if (focus >= MENU_ITEMS) focus = 0;
}

void lcd_ui_confirm(void)
{
    if (focus == 4) {
        lcd_ui_demo_run();
        return;
    }
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(10, 3 * 8, "Selected:", COLOR_GREEN, COLOR_BLACK);
    lcd_draw_string(10, 4 * 8, menu[focus] + 2, COLOR_WHITE, COLOR_BLACK);
    lcd_update();
    vTaskDelay(1500 / portTICK_PERIOD_MS);
}

/* ──────────────────────────────────────────────
 *  LCD 版综合 UI 演示（ST7735 160×128 彩色）
 * ────────────────────────────────────────────── */

static void demo_wave(void)
{
    static uint8_t phase = 0;
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(32, 0, "Wave Demo", COLOR_CYAN, COLOR_BLACK);

    for (uint8_t x = 0; x < LCD_WIDTH; x++) {
        int y = 60 + (int)(20.0f * sinf((x + phase) * 3.14159f * 2 / 16));
        if (y >= 0 && y < LCD_HEIGHT)
            lcd_draw_pixel(x, y, COLOR_RED);
    }

    for (uint8_t x = 0; x < LCD_WIDTH; x++) {
        int y = 100 + (int)(15.0f * sinf((x + phase) * 3.14159f * 2 / 24 + 1));
        if (y >= 0 && y < LCD_HEIGHT)
            lcd_draw_pixel(x, y, COLOR_GREEN);
    }

    for (uint8_t x = 0; x < LCD_WIDTH; x++) {
        int y = 30 + (int)(12.0f * sinf((x + phase) * 3.14159f * 2 / 20 + 2));
        if (y >= 0 && y < LCD_HEIGHT)
            lcd_draw_pixel(x, y, COLOR_BLUE);
    }

    phase += 2;
    lcd_update();
}

static void demo_bounce(void)
{
    static int bx = 30, by = 40, bdx = 3, bdy = 2;
    static uint16_t colors[] = { COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN };
    static int ci = 0;

    lcd_clear(COLOR_BLACK);
    lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, COLOR_WHITE);
    bx += bdx; by += bdy;
    if (bx <= 3 || bx >= LCD_WIDTH - 8)  { bdx = -bdx; ci = (ci + 1) % 6; }
    if (by <= 3 || by >= LCD_HEIGHT - 8) { bdy = -bdy; ci = (ci + 1) % 6; }
    lcd_draw_circle(bx, by, 5, colors[ci], 1);
    lcd_update();
}

static void demo_progress(void)
{
    for (int p = 0; p <= 100; p += 2) {
        lcd_clear(COLOR_BLACK);
        lcd_draw_string(8, 0, "Loading...", COLOR_WHITE, COLOR_BLACK);

        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", p);
        lcd_draw_string(102, 0, buf, COLOR_YELLOW, COLOR_BLACK);

        lcd_draw_rect(14, 20, 100, 14, COLOR_WHITE);
        if (p > 0)
            lcd_fill_rect(14, 20, p, 14, (p < 50) ? COLOR_RED : (p < 80) ? COLOR_YELLOW : COLOR_GREEN);
        lcd_update();
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

static void demo_shapes(void)
{
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(24, 0, "Shapes Test", COLOR_WHITE, COLOR_BLACK);
    lcd_draw_rect(4, 12, 30, 30, COLOR_RED);
    lcd_draw_circle(70, 27, 15, COLOR_GREEN, 0);
    lcd_draw_line(0, 50, 127, 10, COLOR_YELLOW);
    lcd_fill_rect(40, 100, 48, 30, COLOR_BLUE);
    lcd_draw_rect(40, 100, 48, 30, COLOR_WHITE);
    lcd_draw_string(4, 80, "RGB565 COLOR", COLOR_MAGENTA, COLOR_BLACK);
    lcd_update();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

static void demo_charset(void)
{
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(0, 0, " !\"#$%&'()*+,-./", COLOR_GREEN, COLOR_BLACK);
    lcd_draw_string(0, 1, "0123456789:;<=>?", COLOR_CYAN, COLOR_BLACK);
    lcd_draw_string(0, 2, "@ABCDEFGHIJKLMNO", COLOR_YELLOW, COLOR_BLACK);
    lcd_draw_string(0, 3, "PQRSTUVWXYZ[\\]^_", COLOR_MAGENTA, COLOR_BLACK);
    lcd_draw_string(0, 4, "`abcdefghijklmno", COLOR_RED, COLOR_BLACK);
    lcd_draw_string(0, 5, "pqrstuvwxyz{|}~ ", COLOR_ORANGE, COLOR_BLACK);
    lcd_draw_string(8, 6, "ST7735 128x160", COLOR_WHITE, COLOR_BLUE);
    lcd_draw_string(16, 7, "ESP32 + SPI", COLOR_WHITE, COLOR_BLACK);
    lcd_update();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

static void demo_marquee(void)
{
    for (int step = 0; step < 48; step++) {
        lcd_clear(COLOR_BLACK);
        lcd_draw_string(28, 40, "Marquee!", COLOR_YELLOW, COLOR_BLACK);

        uint16_t c = (step % 8 < 4) ? COLOR_RED : COLOR_GREEN;
        int s = step % 4;
        if (s == 0) lcd_draw_hline(0, 0, step * 3, c);
        if (s == 1) lcd_draw_vline(127, 0, step * 3, c);
        if (s == 2) lcd_draw_hline(127 - step * 3, 159, step * 3, c);
        if (s == 3) lcd_draw_vline(0, 159 - step * 3, step * 3, c);
        lcd_update();
        vTaskDelay(15 / portTICK_PERIOD_MS);
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
}

static void demo_color_bars(void)
{
    lcd_clear(COLOR_BLACK);
    uint16_t bars[] = { COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW,
                        COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE, COLOR_ORANGE };
    uint8_t bw = LCD_WIDTH / 8;
    for (int i = 0; i < 8; i++)
        lcd_fill_rect(i * bw, 10, bw, 30, bars[i]);

    lcd_draw_string(0, 50, "R  G  B  Y  M  C  W  O", COLOR_WHITE, COLOR_BLACK);
    lcd_draw_string(4, 70, "Color Bar Test 128x160", COLOR_CYAN, COLOR_BLACK);
    lcd_draw_string(4, 90, "SPI 26MHz ST7735", COLOR_YELLOW, COLOR_BLACK);
    lcd_update();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

static void demo_fill_cycle(void)
{
    uint16_t colors[] = { COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_BLACK };
    for (int i = 0; i < 4; i++) {
        lcd_clear(colors[i]);
        lcd_update();
        vTaskDelay(400 / portTICK_PERIOD_MS);
    }
}

void lcd_ui_demo_run(void)
{
    while (1) {
        demo_wave();
        vTaskDelay(30 / portTICK_PERIOD_MS);

        for (int i = 0; i < 80; i++)
            demo_bounce();
        vTaskDelay(100 / portTICK_PERIOD_MS);

        demo_progress();
        demo_shapes();
        demo_charset();
        demo_marquee();
        demo_color_bars();
        demo_fill_cycle();

        for (int i = 0; i < 10; i++) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            if (gpio_get_level(GPIO_NUM_32) == 0) return;
        }
    }
}
