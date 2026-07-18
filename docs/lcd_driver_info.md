# LCD 驱动信息 — ST7735S 1.8寸 128x160 SPI

## 模块信息

| 参数 | 值 |
|------|-----|
| 型号 | 1.8寸 TFT LCD |
| 驱动 IC | ST7735S |
| 分辨率 | 128 × 160 |
| 色彩 | 16-bit RGB565 (65K色) |
| 接口 | 4线 SPI + DC + RES |
| 工作电压 | 3.3V / 5V |
| 参考资料 | `Examples/1.8TFT/驱动IC数据手册.pdf` / `原理图.pdf` |

**ST7735S 和 ST7735R 的差异：** Gamma 表、电源序列、帧率和反转模式参数均不同，初始化序列不能混用。见下方初始化序列对比。

---

## 硬件连接

| LCD 引脚 | 功能 | ESP32 GPIO |
|----------|------|-----------|
| SCL | SPI 时钟 | GPIO18 |
| SDA | SPI MOSI | GPIO23 |
| RES | 复位 (低电平复位) | GPIO4 |
| DC | 数据/命令选择 (高=数据) | GPIO2 |
| CS | 片选 (低电平选中) | GPIO5 |
| BLK | 背光 | GPIO22 |
| VCC | 电源 | 3.3V / 5V |
| GND | 地 | GND |

总线: **SPI2_HOST**, 26MHz, Mode 0 (CPOL=0, CPHA=0)

---

## 驱动程序架构

```
lcd_st7735.h / lcd_st7735.c
  ├── 底层 SPI 操作 (cs_select, dc_cmd, dc_data, spi_write)
  ├── 初始化序列 (st7735_hw_reset, st7735_init_seq)
  ├── 帧缓冲 (framebuf[160][128], 40KB RGB565)
  ├── 绘图 API (clear, pixel, rect, circle, line, char, string, bitmap)
  ├── 性能计数器 (frame_counter, lcd_get_frames)
  └── 复用 font_6x8.h 字库 (非内嵌)
```

---

## 初始化序列

### 复位时序

```
RES=1 → delay(10ms) → RES=0 → delay(10ms) → RES=1 → delay(120ms)
```

### ST7735S 初始化命令 (当前已实现)

```
SLPOUT(0x11)          + delay 120ms
FRMCTR1(0xB1):        0x05, 0x3C, 0x3C
FRMCTR2(0xB2):        0x05, 0x3C, 0x3C
FRMCTR3(0xB3):        0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C
INVCTR(0xB4):         0x03              (Dot inversion)
PWCTR1(0xC0):         0x28, 0x08, 0x04
PWCTR2(0xC1):         0xC0
PWCTR3(0xC2):         0x0D, 0x00
PWCTR4(0xC3):         0x8D, 0x2A
PWCTR5(0xC4):         0x8D, 0xEE
VCOM(0xC5):           0x1A
MADCTL(0x36):         0xC0              (RGB order)
GMCTRP1(0xE0):        0x03,0x22,0x07,0x0A,0x2E,0x30,0x25,0x2A,0x28,0x26,0x2E,0x3A,0x00,0x01,0x03,0x13
GMCTRN1(0xE1):        0x04,0x16,0x06,0x0D,0x2D,0x26,0x23,0x27,0x27,0x25,0x2D,0x3B,0x00,0x01,0x04,0x13
COLMOD(0x3A):         0x05              (16-bit RGB565)
DISPON(0x29)          + delay 50ms
```

### ST7735R vs ST7735S 关键差异 (白屏原因)

| 寄存器 | ST7735R (旧/错误) | ST7735S (当前/正确) |
|--------|-------------------|-------------------|
| FRMCTR1-3 (0xB1-3) | `0x01, 0x2C, 0x2D` | `0x05, 0x3C, 0x3C` |
| INVCTR (0xB4) | `0x07` (Column inversion) | `0x03` (Dot inversion) |
| PWCTR1 (0xC0) | `0xA2, 0x02, 0x84` | `0x28, 0x08, 0x04` |
| PWCTR2 (0xC1) | `0xC5` | `0xC0` |
| PWCTR3 (0xC2) | `0x0A, 0x00` | `0x0D, 0x00` |
| PWCTR4-5 (0xC3-4) | `0x8A, 0x2A` / `0x8A, 0xEE` | `0x8D, 0x2A` / `0x8D, 0xEE` |
| VCOM (0xC5) | `0x0E` | `0x1A` |
| Gamma (0xE0/0xE1) | ST7735R 32字节 | ST7735S 32字节 |
| RAM power save (0xF6) | `0xF0→0x01, 0xF6→0x00` | **不需要** (ST7735S 无此命令) |

**白屏排查历史：** 原先误用了 ST7735R 的初始化序列（来自 STM32 例程 `Lcd_Driver.c`），导致电源和 Gamma 参数完全错误。已按 `Examples/1.8TFT/ST7735S_CTC180_INITIAL.txt` / UTFT `st7735s/initlcd.h` 修正为 ST7735S 专用序列。

---

## 实现功能

### 绘图 API

| 函数 | 说明 |
|------|------|
| `lcd_init()` | GPIO → SPI → 硬件复位 → 初始化序列 → 清屏 |
| `lcd_clear(color)` | 帧缓冲填色 + 刷新 |
| `lcd_fill(color)` | 帧缓冲填色（不刷新），供 `lcd_clear()` 内部调用 |
| `lcd_update()` | 全屏刷新生效（递增 `frame_counter`） |
| `lcd_update_area(x0,y0,x1,y1)` | 局部刷新生效（递增 `frame_counter`） |
| `lcd_set_window(x0,y0,x1,y1)` | CASET + RASET + RAMWR |
| `lcd_draw_pixel(x,y,color)` | 单像素写缓冲区 |
| `lcd_fill_rect(x,y,w,h,color)` | 填充矩形 |
| `lcd_draw_rect(x,y,w,h,color)` | 空心矩形 |
| `lcd_draw_hline/vline` | 水平/垂直线 |
| `lcd_draw_line(x0,y0,x1,y1,color)` | Bresenham 直线 |
| `lcd_draw_circle(cx,cy,r,color,fill)` | Bresenham 圆，支持填充/空心 |
| `lcd_draw_bitmap(x,y,w,h,data,fg,bg)` | 单色位图渲染 |
| `lcd_draw_char(x,y,c,fg,bg)` | 6×8 ASCII 字符 |
| `lcd_draw_string(x,y,str,fg,bg)` | 字符串 |
| `lcd_draw_stringf(x,y,fg,bg,fmt,...)` | printf 格式化字符串，支持 `\n` 换行 |
| `lcd_set_fps(fps)` | 预留 FPS 设置 |
| `lcd_get_frames()` | 获取帧计数 |

### 颜色定义

```c
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_ORANGE    0xFC00
#define COLOR_GRAY      0x8410

// RGB565 宏:
#define RGB565(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
```

---

## 与 SSD1306 的差异 (适配要点)

| 维度 | SSD1306 (OLED) | ST7735 (LCD) |
|------|---------------|--------------|
| 分辨率 | 128×64 | **128×160** |
| 颜色 | 单色 (1-bit) | **RGB565 (16-bit)** |
| 帧缓冲 | 1KB (framebuf[8][128]) | **40KB** (framebuf[160][128]) |
| 接口 | I2C 800kHz | SPI 26MHz |
| 字体 | 6×8 像素 | 6×8 像素 (复用 font_6x8.h) |
| 中文 | 不支持 | 可扩展 (GBK16/GBK24) |
| 背光控制 | 无 | GPIO PWM |

### 当前 LCD 适配策略

1. **不直接替换 OLED**，OLED 和 LCD 各维护独立驱动
2. 新增 `drivers/display.h/c` 抽象层，通过 Kconfig 选择后端
3. LCD 模式下布局独立适配 (状态栏 16px + 内容区 144px)
4. 所有 UI 代码通过 `display_*()` API 调用，不直接引用 `ssd1306.h` 或 `lcd_st7735.h`

---

## 1.8TFT 示例代码参考 (STM32F103C8)

在 `Examples/1.8TFT/` 中提供了基于 STM32F103C8 的参考实现：

| 文件 | 内容 |
|------|------|
| `Lcd_Driver.c/h` | ST7735 软件 SPI 驱动 (GPIO 位操作) |
| `LCD_Config.h` | 分辨率定义: 128×160 |
| `GUI.c/h` | 绘图层: 圆、线、矩形、按钮、中英文字体 |
| `Font.h` | GBK16/GBK24 中文字库 + ASCII 8×16 字库 + 32×32 数字字库 |
| `QDTFT_demo.c` | 示例: 主菜单 → 色彩测试 → 数字测试 → 字体测试 → 图片显示 |
| `Picture.h` | 40×40 QQ 图片数据 |
| `ST7735S_CTC180_INITIAL.txt` | CTC180 模组专用初始化序列 |
| `驱动IC数据手册.pdf` | ST7735S 数据手册 |
| `原理图.pdf` | 1.8寸模组原理图 |

---

## 已知问题 / TODO

- [ ] `lcd_draw_circle()` 在填充模式下外圈有像素残留（用 COLOR_BLACK 擦除不够彻底）
- [ ] SPI 传输使用全屏刷帧 (128×160×2 = 40KB)，可改为窗口增量更新提升性能
- [ ] 未对接 Kconfig 编译选择开关，目前 SSD1306 和 ST7735 同时编译
- [ ] lcd_ui.c/h 已删除，LCD 的 UI 需通过 display.h 抽象层统一接入
