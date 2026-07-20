/* ============================================================
 *  ssd1306.c — SSD1306 OLED 驱动 (I2C, 128×64, 单色)
 *
 *  帧缓冲：8 pages × 128 bytes = 1KB，页面排列。
 *  绘图操作先写入 framebuf，ssd1306_update() 通过 I2C 批量提交。
 * ============================================================ */
#include "ssd1306.h"
#include "font_6x8.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

/* ---- I2C 硬件配置 ---- */
#define I2C_MASTER_SCL_IO     GPIO_NUM_13
#define I2C_MASTER_SDA_IO     GPIO_NUM_14
#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_FREQ_HZ    1000000        /* 1MHz 快速模式 */
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TIMEOUT_MS 1000

#define TAG "SSD1306"

/* ---- 帧缓冲 + 帧计数器 ---- */
static uint8_t framebuf[SSD1306_HEIGHT / 8][SSD1306_WIDTH];  /* [page][column] */
static uint32_t frame_counter = 0;

uint32_t ssd1306_get_frames(void)
{
    uint32_t cnt = frame_counter;
    frame_counter = 0;      /* 读后清零 */
    return cnt;
}

/* ==================== I2C 底层 ==================== */

/* 向 SSD1306 发送单字节命令 */
static void i2c_write_cmd(uint8_t cmd)
{
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x00, true);   /* Co=0, D/C#=0 (命令) */
    i2c_master_write_byte(h, cmd, true);
    i2c_master_stop(h);
    i2c_master_cmd_begin(I2C_MASTER_NUM, h, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(h);
}

/* 初始化 I2C 控制器 */
static void i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/* ==================== 初始化序列 ==================== */

void ssd1306_init(void)
{
    i2c_init();

    vTaskDelay(10 / portTICK_PERIOD_MS);

    i2c_write_cmd(0xAE);                     /* 关闭显示 */
    i2c_write_cmd(0xD5); i2c_write_cmd(0x80); /* 振荡频率 */
    i2c_write_cmd(0xA8); i2c_write_cmd(0x3F); /* 多路比 (64) */
    i2c_write_cmd(0xD3); i2c_write_cmd(0x00); /* 显示偏移 0 */
    i2c_write_cmd(0x40);                     /* 起始行 0 */
    i2c_write_cmd(0x8D); i2c_write_cmd(0x14); /* 电荷泵使能 */
    i2c_write_cmd(0x20); i2c_write_cmd(0x00); /* 水平寻址模式 */
    i2c_write_cmd(0xA1);                     /* 段重映射 (列 127→SEG0) */
    i2c_write_cmd(0xC8);                     /* COM 扫描方向 (从下到上) */
    i2c_write_cmd(0xDA); i2c_write_cmd(0x12); /* COM 引脚硬件配置 */
    i2c_write_cmd(0x81); i2c_write_cmd(0xCF); /* 对比度 */
    i2c_write_cmd(0xD9); i2c_write_cmd(0xF1); /* 预充电周期 */
    i2c_write_cmd(0xDB); i2c_write_cmd(0x40); /* VCOMH 电压 */
    i2c_write_cmd(0xA4);                     /* 全屏点亮关闭 */
    i2c_write_cmd(0xA6);                     /* 正常显示 (非反色) */
    i2c_write_cmd(0xAF);                     /* 打开显示 */

    ssd1306_clear();
}

/* ==================== 帧缓冲操作 ==================== */

void ssd1306_clear(void)
{
    for (int y = 0; y < SSD1306_HEIGHT / 8; y++)
        for (int x = 0; x < SSD1306_WIDTH; x++)
            framebuf[y][x] = 0x00;
}

/* 清除指定的页面范围 */
void ssd1306_clear_pages(uint8_t p0, uint8_t p1)
{
    if (p1 >= SSD1306_HEIGHT / 8) p1 = SSD1306_HEIGHT / 8 - 1;
    for (int y = p0; y <= p1; y++)
        for (int x = 0; x < SSD1306_WIDTH; x++)
            framebuf[y][x] = 0x00;
}

/* ==================== 提交帧缓冲到硬件 ==================== */

/* 全帧刷新 */
void ssd1306_update(void)
{
    frame_counter++;
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x00, true);
    i2c_master_write_byte(h, 0x21, true);    /* 设置列范围 */
    i2c_master_write_byte(h, 0x00, true);
    i2c_master_write_byte(h, 0x7F, true);
    i2c_master_write_byte(h, 0x22, true);    /* 设置页范围 */
    i2c_master_write_byte(h, 0x00, true);
    i2c_master_write_byte(h, 0x07, true);
    i2c_master_start(h);
    i2c_master_write_byte(h, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x40, true);    /* Co=0, D/C#=1 (数据) */
    for (int y = 0; y < SSD1306_HEIGHT / 8; y++)
        i2c_master_write(h, framebuf[y], SSD1306_WIDTH, true);
    i2c_master_stop(h);
    i2c_master_cmd_begin(I2C_MASTER_NUM, h, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(h);
}

/* 局部刷新区间 (仅刷新指定页范围) */
void ssd1306_update_area(uint8_t p0, uint8_t p1)
{
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x00, true);
    i2c_master_write_byte(h, 0x21, true);
    i2c_master_write_byte(h, 0, true);
    i2c_master_write_byte(h, 127, true);
    i2c_master_write_byte(h, 0x22, true);
    i2c_master_write_byte(h, p0, true);
    i2c_master_write_byte(h, p1, true);
    i2c_master_start(h);
    i2c_master_write_byte(h, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x40, true);
    for (uint8_t y = p0; y <= p1; y++)
        i2c_master_write(h, framebuf[y], SSD1306_WIDTH, true);
    i2c_master_stop(h);
    i2c_master_cmd_begin(I2C_MASTER_NUM, h, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(h);
}

/* ==================== 绘图 API ==================== */

/* 画点 (color=1 亮/白, 0 灭/黑) */
void ssd1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    if (color)
        framebuf[y / 8][x] |= (1 << (y % 8));
    else
        framebuf[y / 8][x] &= ~(1 << (y % 8));
}

void ssd1306_draw_hline(uint8_t x, uint8_t y, uint8_t w)
{
    for (uint8_t i = 0; i < w; i++)
        ssd1306_draw_pixel(x + i, y, 1);
}

void ssd1306_draw_vline(uint8_t x, uint8_t y, uint8_t h)
{
    for (uint8_t i = 0; i < h; i++)
        ssd1306_draw_pixel(x, y + i, 1);
}

void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill)
{
    if (fill) {
        for (uint8_t i = 0; i < h; i++)
            ssd1306_draw_hline(x, y + i, w);
    } else {
        ssd1306_draw_hline(x, y, w);
        ssd1306_draw_hline(x, y + h - 1, w);
        ssd1306_draw_vline(x, y, h);
        ssd1306_draw_vline(x + w - 1, y, h);
    }
}

/* Bresenham 圆 */
void ssd1306_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill)
{
    if (fill) {
        int x = 0, y = r, d = 3 - 2 * r;
        while (x <= y) {
            for (int i = -y; i <= y; i++) ssd1306_draw_pixel(cx + i, cy + x, 1);
            if (x != 0) for (int i = -y; i <= y; i++) ssd1306_draw_pixel(cx + i, cy - x, 1);
            for (int i = -x; i <= x; i++) ssd1306_draw_pixel(cx + i, cy + y, 1);
            for (int i = -x; i <= x; i++) ssd1306_draw_pixel(cx + i, cy - y, 1);
            x++;
            if (d < 0) d += 4 * x + 6; else { y--; d += 4 * (x - y) + 10; }
        }
    } else {
        int x = 0, y = r, d = 3 - 2 * r;
        while (x <= y) {
            ssd1306_draw_pixel(cx + x, cy + y, 1);
            ssd1306_draw_pixel(cx - x, cy + y, 1);
            ssd1306_draw_pixel(cx + x, cy - y, 1);
            ssd1306_draw_pixel(cx - x, cy - y, 1);
            ssd1306_draw_pixel(cx + y, cy + x, 1);
            ssd1306_draw_pixel(cx - y, cy + x, 1);
            ssd1306_draw_pixel(cx + y, cy - x, 1);
            ssd1306_draw_pixel(cx - y, cy - x, 1);
            x++;
            if (d < 0) d += 4 * x + 6; else { y--; d += 4 * (x - y) + 10; }
        }
    }
}

/* Bresenham 直线 */
void ssd1306_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (1) {
        ssd1306_draw_pixel(x0, y0, 1);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= -dy) { err -= dy; x0 += sx; }
        if (e2 <= dx)  { err += dx; y0 += sy; }
    }
}

/* 单色位图 (data 按列排列, MSB 优先) */
void ssd1306_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data)
{
    for (uint8_t j = 0; j < h; j++)
        for (uint8_t i = 0; i < w; i++)
            if (data[j * ((w + 7) / 8) + i / 8] & (0x80 >> (i % 8)))
                ssd1306_draw_pixel(x + i, y + j, 1);
}

/* ==================== 文字 ==================== */

/* 6×8 字符直接写入 framebuf (优化：不通过 draw_pixel) */
void ssd1306_draw_char(uint8_t x, uint8_t y, char c)
{
    if (c < 32 || c > 127) c = 32;
    c -= 32;
    for (int i = 0; i < 6; i++) {
        uint8_t line = font6x8[(uint8_t)c][i];
        for (int j = 0; j < 8; j++) {
            if (line & (1 << j))
                framebuf[y][x + i] |= (1 << j);
        }
    }
}

void ssd1306_draw_string(uint8_t x, uint8_t y, const char *str)
{
    while (*str) {
        ssd1306_draw_char(x, y, *str);
        x += 6;
        if (x + 6 > SSD1306_WIDTH) { x = 0; y += 8; }  /* 自动换行 */
        str++;
    }
}
