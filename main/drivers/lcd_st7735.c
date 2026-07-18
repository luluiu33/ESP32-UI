#include "lcd_st7735.h"
#include "font_6x8.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define TAG "ST7735"

/* ── 帧缓冲 128 × 160 × 2 bytes = 40KB ── */
static uint16_t framebuf[LCD_HEIGHT][LCD_WIDTH];
static spi_device_handle_t spi_dev;
static uint32_t frame_counter = 0;

/* ── 底层 SPI 操作 ── */
static void cs_select(void)
{
    gpio_set_level(LCD_CS, 0);
}

static void cs_deselect(void)
{
    gpio_set_level(LCD_CS, 1);
}

static void dc_cmd(void)
{
    gpio_set_level(LCD_DC, 0);
}

static void dc_data(void)
{
    gpio_set_level(LCD_DC, 1);
}

static void spi_write(const uint8_t *data, size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(spi_dev, &t);
}

static void lcd_write_cmd(uint8_t cmd)
{
    cs_select();
    dc_cmd();
    spi_write(&cmd, 1);
    cs_deselect();
}

static void lcd_write_data(const uint8_t *data, size_t len)
{
    cs_select();
    dc_data();
    spi_write(data, len);
    cs_deselect();
}

static void lcd_write_data_byte(uint8_t data)
{
    lcd_write_data(&data, 1);
}

/* ── ST7735 初始化序列 ── */
static void st7735_hw_reset(void)
{
    gpio_set_level(LCD_RESET, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(LCD_RESET, 1);
    vTaskDelay(120 / portTICK_PERIOD_MS);
}

static void st7735_init_seq(void)
{
    lcd_write_cmd(0x11);              // SLPOUT
    vTaskDelay(120 / portTICK_PERIOD_MS);

    lcd_write_cmd(0xB1);              // FRMCTR1
    lcd_write_data_byte(0x05);
    lcd_write_data_byte(0x3C);
    lcd_write_data_byte(0x3C);

    lcd_write_cmd(0xB2);              // FRMCTR2
    lcd_write_data_byte(0x05);
    lcd_write_data_byte(0x3C);
    lcd_write_data_byte(0x3C);

    lcd_write_cmd(0xB3);              // FRMCTR3
    lcd_write_data_byte(0x05);
    lcd_write_data_byte(0x3C);
    lcd_write_data_byte(0x3C);
    lcd_write_data_byte(0x05);
    lcd_write_data_byte(0x3C);
    lcd_write_data_byte(0x3C);

    lcd_write_cmd(0xB4);              // INVCTR (Dot inversion)
    lcd_write_data_byte(0x03);

    lcd_write_cmd(0xC0);              // PWCTR1
    lcd_write_data_byte(0x28);
    lcd_write_data_byte(0x08);
    lcd_write_data_byte(0x04);

    lcd_write_cmd(0xC1);              // PWCTR2
    lcd_write_data_byte(0xC0);

    lcd_write_cmd(0xC2);              // PWCTR3
    lcd_write_data_byte(0x0D);
    lcd_write_data_byte(0x00);

    lcd_write_cmd(0xC3);              // PWCTR4
    lcd_write_data_byte(0x8D);
    lcd_write_data_byte(0x2A);

    lcd_write_cmd(0xC4);              // PWCTR5
    lcd_write_data_byte(0x8D);
    lcd_write_data_byte(0xEE);

    lcd_write_cmd(0xC5);              // VCOM
    lcd_write_data_byte(0x1A);

    lcd_write_cmd(0x36);              // MADCTL
    lcd_write_data_byte(0xC0);

    lcd_write_cmd(0xE0);              // GMCTRP1 (positive gamma)
    const uint8_t gamma_pos[] = { 0x04, 0x22, 0x07, 0x0A, 0x2E, 0x30, 0x25,
                                   0x2A, 0x28, 0x26, 0x2E, 0x3A, 0x00, 0x01,
                                   0x03, 0x13 };
    for (int i = 0; i < 16; i++) lcd_write_data_byte(gamma_pos[i]);

    lcd_write_cmd(0xE1);              // GMCTRN1 (negative gamma)
    const uint8_t gamma_neg[] = { 0x04, 0x16, 0x06, 0x0D, 0x2D, 0x26, 0x23,
                                   0x27, 0x27, 0x25, 0x2D, 0x3B, 0x00, 0x01,
                                   0x04, 0x13 };
    for (int i = 0; i < 16; i++) lcd_write_data_byte(gamma_neg[i]);

    lcd_write_cmd(0x3A);              // COLMOD
    lcd_write_data_byte(0x05);        // 16-bit RGB565

    lcd_write_cmd(0x29);              // DISPON
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

/* ── SPI 总线初始化 ── */
static void spi_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = LCD_SPI_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2 + 8,
    };
    spi_device_interface_config_t dev = {
        .clock_speed_hz = 26 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
        .flags = SPI_DEVICE_NO_DUMMY,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_SPI_HOST, &dev, &spi_dev));
}

/* ── GPIO 初始化 ── */
static void gpio_init(void)
{
    const int pins[] = { LCD_RESET, LCD_DC, LCD_CS };
    for (int i = 0; i < 3; i++) {
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins[i], 1);
    }
}

/* ── 公开 API ── */

void lcd_init(void)
{
    gpio_init();
    spi_init();
    st7735_hw_reset();
    st7735_init_seq();
    lcd_clear(COLOR_BLACK);
}

void lcd_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t col[] = { (uint8_t)(x0 >> 8), x0, (uint8_t)(x1 >> 8), x1 };
    uint8_t row[] = { (uint8_t)(y0 >> 8), y0, (uint8_t)(y1 >> 8), y1 };
    lcd_write_cmd(0x2A);
    lcd_write_data(col, 4);
    lcd_write_cmd(0x2B);
    lcd_write_data(row, 4);
    lcd_write_cmd(0x2C);
}

void lcd_clear(uint16_t color)
{
    lcd_fill(color);
    lcd_update();
}

void lcd_fill(uint16_t color)
{
    for (int y = 0; y < LCD_HEIGHT; y++)
        for (int x = 0; x < LCD_WIDTH; x++)
            framebuf[y][x] = color;
}

void lcd_update(void)
{
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    cs_select();
    dc_data();
    spi_transaction_t t = {
        .length = LCD_WIDTH * LCD_HEIGHT * 2 * 8,
        .tx_buffer = framebuf,
    };
    spi_device_transmit(spi_dev, &t);
    cs_deselect();
    frame_counter++;
}

void lcd_draw_pixel(uint8_t x, uint8_t y, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    framebuf[y][x] = color;
}

void lcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH)  w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    for (uint8_t row = 0; row < h; row++)
        for (uint8_t col = 0; col < w; col++)
            framebuf[y + row][x + col] = color;
}

void lcd_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
    lcd_draw_hline(x, y, w, color);
    lcd_draw_hline(x, y + h - 1, w, color);
    lcd_draw_vline(x, y, h, color);
    lcd_draw_vline(x + w - 1, y, h, color);
}

void lcd_draw_hline(uint8_t x, uint8_t y, uint8_t w, uint16_t color)
{
    for (uint8_t i = 0; i < w; i++)
        lcd_draw_pixel(x + i, y, color);
}

void lcd_draw_vline(uint8_t x, uint8_t y, uint8_t h, uint16_t color)
{
    for (uint8_t i = 0; i < h; i++)
        lcd_draw_pixel(x, y + i, color);
}

void lcd_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (1) {
        lcd_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void lcd_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint16_t color, uint8_t fill)
{
    int x = 0, y = r, d = 3 - 2 * r;
    while (x <= y) {
        if (fill) {
            for (int i = -y; i <= y; i++) lcd_draw_pixel(cx + i, cy + x, color);
            if (x != 0) for (int i = -y; i <= y; i++) lcd_draw_pixel(cx + i, cy - x, color);
            for (int i = -x; i <= x; i++) lcd_draw_pixel(cx + i, cy + y, color);
            for (int i = -x; i <= x; i++) lcd_draw_pixel(cx + i, cy - y, color);
        } else {
            lcd_draw_pixel(cx + x, cy + y, color);
            lcd_draw_pixel(cx - x, cy + y, color);
            lcd_draw_pixel(cx + x, cy - y, color);
            lcd_draw_pixel(cx - x, cy - y, color);
            lcd_draw_pixel(cx + y, cy + x, color);
            lcd_draw_pixel(cx - y, cy + x, color);
            lcd_draw_pixel(cx + y, cy - x, color);
            lcd_draw_pixel(cx - y, cy - x, color);
        }
        x++;
        if (d < 0) d += 4 * x + 6; else { y--; d += 4 * (x - y) + 10; }
    }
}

void lcd_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data, uint16_t fg, uint16_t bg)
{
    for (uint8_t row = 0; row < h; row++) {
        for (uint8_t col = 0; col < w; col++) {
            uint8_t byte = data[row * ((w + 7) / 8) + col / 8];
            uint16_t color = (byte & (0x80 >> (col & 7))) ? fg : bg;
            lcd_draw_pixel(x + col, y + row, color);
        }
    }
}

void lcd_draw_char(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg)
{
    if (c < 32 || c > 127) c = 32;
    c -= 32;
    for (int i = 0; i < 6; i++) {
        uint8_t line = font6x8[(uint8_t)c][i];
        for (int j = 0; j < 8; j++) {
            lcd_draw_pixel(x + i, y + j, (line & (1 << j)) ? fg : bg);
        }
    }
}

void lcd_draw_string(uint8_t x, uint8_t y, const char *str, uint16_t fg, uint16_t bg)
{
    lcd_draw_stringf(x, y, fg, bg, "%s", str);
}

void lcd_draw_stringf(uint8_t x, uint8_t y, uint16_t fg, uint16_t bg, const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    uint8_t ox = x;
    for (char *p = buf; *p; p++) {
        if (*p == '\n') { x = ox; y += 8; continue; }
        lcd_draw_char(x, y, *p, fg, bg);
        x += 6;
        if (x + 6 > LCD_WIDTH) { x = ox; y += 8; }
    }
}

void lcd_update_area(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    lcd_set_window(x0, y0, x1, y1);
    cs_select();
    dc_data();
    for (int y = y0; y <= y1; y++) {
        spi_transaction_t t = {
            .length = (x1 - x0 + 1) * 2 * 8,
            .tx_buffer = &framebuf[y][x0],
        };
        spi_device_transmit(spi_dev, &t);
    }
    cs_deselect();
    frame_counter++;
}

void lcd_set_fps(uint8_t fps)
{
    (void)fps;
}

uint32_t lcd_get_frames(void)
{
    uint32_t cnt = frame_counter;
    frame_counter = 0;
    return cnt;
}
