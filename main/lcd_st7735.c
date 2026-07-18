#include "lcd_st7735.h"
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
    lcd_write_cmd(0x01);              // SWRESET
    vTaskDelay(150 / portTICK_PERIOD_MS);

    lcd_write_cmd(0x11);              // SLPOUT
    vTaskDelay(50 / portTICK_PERIOD_MS);

    lcd_write_cmd(0x3A);              // COLMOD
    lcd_write_data_byte(0x05);        // 16-bit RGB565

    lcd_write_cmd(0x36);              // MADCTL
    lcd_write_data_byte(0xC0);        // RGB order, row/col exchange

    lcd_write_cmd(0xB1);              // FRMCTR1
    lcd_write_data_byte(0x01);
    lcd_write_data_byte(0x2C);
    lcd_write_data_byte(0x2D);

    lcd_write_cmd(0xB2);              // FRMCTR2
    lcd_write_data_byte(0x01);
    lcd_write_data_byte(0x2C);
    lcd_write_data_byte(0x2D);

    lcd_write_cmd(0xB3);              // FRMCTR3
    lcd_write_data_byte(0x01);
    lcd_write_data_byte(0x2C);
    lcd_write_data_byte(0x2D);
    lcd_write_data_byte(0x01);
    lcd_write_data_byte(0x2C);
    lcd_write_data_byte(0x2D);

    lcd_write_cmd(0xB4);              // INVCTR
    lcd_write_data_byte(0x07);

    lcd_write_cmd(0xC0);              // PWCTR1
    lcd_write_data_byte(0xA2);
    lcd_write_data_byte(0x02);
    lcd_write_data_byte(0x84);

    lcd_write_cmd(0xC1);              // PWCTR2
    lcd_write_data_byte(0xC5);

    lcd_write_cmd(0xC2);              // PWCTR3
    lcd_write_data_byte(0x0A);
    lcd_write_data_byte(0x00);

    lcd_write_cmd(0xC3);              // PWCTR4
    lcd_write_data_byte(0x8A);
    lcd_write_data_byte(0x2A);

    lcd_write_cmd(0xC4);              // PWCTR5
    lcd_write_data_byte(0x8A);
    lcd_write_data_byte(0xEE);

    lcd_write_cmd(0xC5);              // VCOM
    lcd_write_data_byte(0x0E);

    lcd_write_cmd(0xE0);              // GMCTRP1 (positive gamma)
    const uint8_t gamma_pos[] = { 0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20,
                                  0x22, 0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07,
                                  0x02, 0x10 };
    for (int i = 0; i < 16; i++) lcd_write_data_byte(gamma_pos[i]);

    lcd_write_cmd(0xE1);              // GMCTRN1 (negative gamma)
    const uint8_t gamma_neg[] = { 0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29,
                                  0x2E, 0x30, 0x30, 0x39, 0x3F, 0x00, 0x07,
                                  0x03, 0x10 };
    for (int i = 0; i < 16; i++) lcd_write_data_byte(gamma_neg[i]);

    lcd_write_cmd(0xFC);              // Undocumented lock
    lcd_write_data_byte(0x00);

    lcd_write_cmd(0x36);              // MADCTL
    lcd_write_data_byte(0xC0);

    lcd_write_cmd(0x21);              // INVON
    lcd_write_cmd(0x13);              // NORMAL

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
    for (int y = 0; y < LCD_HEIGHT; y++)
        for (int x = 0; x < LCD_WIDTH; x++)
            framebuf[y][x] = color;
    lcd_update();
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
        for (int i = -y; i <= y; i++) lcd_draw_pixel(cx + i, cy + x, color);
        if (x != 0) for (int i = -y; i <= y; i++) lcd_draw_pixel(cx + i, cy - x, color);
        for (int i = -x; i <= x; i++) lcd_draw_pixel(cx + i, cy + y, color);
        for (int i = -x; i <= x; i++) lcd_draw_pixel(cx + i, cy - y, color);
        if (!fill) {
            if (x == 0) { lcd_draw_pixel(cx, cy + y, COLOR_BLACK); lcd_draw_pixel(cx, cy - y, COLOR_BLACK); }
            lcd_draw_pixel(cx + y, cy + x, COLOR_BLACK); lcd_draw_pixel(cx - y, cy + x, COLOR_BLACK);
            lcd_draw_pixel(cx + y, cy - x, COLOR_BLACK); lcd_draw_pixel(cx - y, cy - x, COLOR_BLACK);
            if (x != 0) {
                lcd_draw_pixel(cx + x, cy + y, COLOR_BLACK); lcd_draw_pixel(cx - x, cy + y, COLOR_BLACK);
                lcd_draw_pixel(cx + x, cy - y, COLOR_BLACK); lcd_draw_pixel(cx - x, cy - y, COLOR_BLACK);
            }
        }
        x++;
        if (d < 0) d += 4 * x + 6; else { y--; d += 4 * (x - y) + 10; }
    }
}

/* ── 6×8 ASCII 字库（同 ssd1306.c） ── */
static const uint8_t font6x8[][6] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00 },
    { 0x00, 0x07, 0x00, 0x07, 0x00, 0x00 },
    { 0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00 },
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00 },
    { 0x23, 0x13, 0x08, 0x64, 0x62, 0x00 },
    { 0x36, 0x49, 0x55, 0x22, 0x50, 0x00 },
    { 0x00, 0x05, 0x03, 0x00, 0x00, 0x00 },
    { 0x00, 0x1C, 0x22, 0x41, 0x00, 0x00 },
    { 0x00, 0x41, 0x22, 0x1C, 0x00, 0x00 },
    { 0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x00 },
    { 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00 },
    { 0x00, 0x50, 0x30, 0x00, 0x00, 0x00 },
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x00 },
    { 0x00, 0x60, 0x60, 0x00, 0x00, 0x00 },
    { 0x20, 0x10, 0x08, 0x04, 0x02, 0x00 },
    { 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 },
    { 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00 },
    { 0x42, 0x61, 0x51, 0x49, 0x46, 0x00 },
    { 0x21, 0x41, 0x45, 0x4B, 0x31, 0x00 },
    { 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00 },
    { 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 },
    { 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00 },
    { 0x01, 0x71, 0x09, 0x05, 0x03, 0x00 },
    { 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 },
    { 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00 },
    { 0x00, 0x36, 0x36, 0x00, 0x00, 0x00 },
    { 0x00, 0x56, 0x36, 0x00, 0x00, 0x00 },
    { 0x00, 0x08, 0x14, 0x22, 0x41, 0x00 },
    { 0x14, 0x14, 0x14, 0x14, 0x14, 0x00 },
    { 0x41, 0x22, 0x14, 0x08, 0x00, 0x00 },
    { 0x02, 0x01, 0x51, 0x09, 0x06, 0x00 },
    { 0x32, 0x49, 0x79, 0x41, 0x3E, 0x00 },
    { 0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00 },
    { 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00 },
    { 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00 },
    { 0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00 },
    { 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00 },
    { 0x7F, 0x09, 0x09, 0x01, 0x01, 0x00 },
    { 0x3E, 0x41, 0x41, 0x51, 0x32, 0x00 },
    { 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00 },
    { 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00 },
    { 0x20, 0x40, 0x41, 0x3F, 0x01, 0x00 },
    { 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00 },
    { 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00 },
    { 0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00 },
    { 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00 },
    { 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00 },
    { 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00 },
    { 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00 },
    { 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00 },
    { 0x46, 0x49, 0x49, 0x49, 0x31, 0x00 },
    { 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00 },
    { 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00 },
    { 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00 },
    { 0x7F, 0x20, 0x18, 0x20, 0x7F, 0x00 },
    { 0x63, 0x14, 0x08, 0x14, 0x63, 0x00 },
    { 0x03, 0x04, 0x78, 0x04, 0x03, 0x00 },
    { 0x61, 0x51, 0x49, 0x45, 0x43, 0x00 },
    { 0x00, 0x00, 0x7F, 0x41, 0x41, 0x00 },
    { 0x02, 0x04, 0x08, 0x10, 0x20, 0x00 },
    { 0x41, 0x41, 0x7F, 0x00, 0x00, 0x00 },
    { 0x04, 0x02, 0x01, 0x02, 0x04, 0x00 },
    { 0x80, 0x80, 0x80, 0x80, 0x80, 0x00 },
    { 0x00, 0x01, 0x02, 0x04, 0x00, 0x00 },
    { 0x20, 0x54, 0x54, 0x54, 0x78, 0x00 },
    { 0x7F, 0x48, 0x44, 0x44, 0x38, 0x00 },
    { 0x38, 0x44, 0x44, 0x44, 0x20, 0x00 },
    { 0x38, 0x44, 0x44, 0x48, 0x7F, 0x00 },
    { 0x38, 0x54, 0x54, 0x54, 0x18, 0x00 },
    { 0x08, 0x7E, 0x09, 0x01, 0x02, 0x00 },
    { 0x0C, 0x52, 0x52, 0x52, 0x3E, 0x00 },
    { 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00 },
    { 0x00, 0x44, 0x7D, 0x40, 0x00, 0x00 },
    { 0x20, 0x40, 0x44, 0x3D, 0x00, 0x00 },
    { 0x00, 0x7F, 0x10, 0x28, 0x44, 0x00 },
    { 0x00, 0x41, 0x7F, 0x40, 0x00, 0x00 },
    { 0x7C, 0x04, 0x18, 0x04, 0x78, 0x00 },
    { 0x7C, 0x08, 0x04, 0x04, 0x78, 0x00 },
    { 0x38, 0x44, 0x44, 0x44, 0x38, 0x00 },
    { 0x7C, 0x14, 0x14, 0x14, 0x08, 0x00 },
    { 0x08, 0x14, 0x14, 0x18, 0x7C, 0x00 },
    { 0x7C, 0x08, 0x04, 0x04, 0x08, 0x00 },
    { 0x48, 0x54, 0x54, 0x54, 0x20, 0x00 },
    { 0x04, 0x3F, 0x44, 0x40, 0x20, 0x00 },
    { 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00 },
    { 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00 },
    { 0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00 },
    { 0x44, 0x28, 0x10, 0x28, 0x44, 0x00 },
    { 0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00 },
    { 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00 },
    { 0x00, 0x08, 0x36, 0x41, 0x00, 0x00 },
    { 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00 },
    { 0x00, 0x41, 0x36, 0x08, 0x00, 0x00 },
    { 0x08, 0x04, 0x08, 0x10, 0x08, 0x00 },
};

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
    lcd_update();
}
