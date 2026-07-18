# LCD 集成计划 — ST7735 128x160 SPI

## 硬件连接

| LCD 引脚 | ESP32 GPIO |
|----------|-----------|
| SCL      | GPIO18 (SPI_CLK) |
| SDA(MOSI)| GPIO23 (SPI_MOSI) |
| RES      | GPIO22 |
| DC       | GPIO21 |
| CS       | GPIO5  |

总线: SPI2_HOST (VSPI), 26MHz, Mode 0

---

## 1. 当前依赖结构

`ssd1306_*` API 被 9 个文件直接依赖：

| 文件 | 使用的 ssd1306 API |
|------|-------------------|
| `main.c` | `get_frames / set_fps / set_cpu` |
| `ui/ui.c` | `init / clear` |
| `ui/menu.c` | `clear / draw_string / update` |
| `ui/about.c` | `clear / draw_string / update` |
| `ui/ui_common.c` | `draw_line / draw_hline` |
| `tests/test_ui.c` | `clear / draw_string / draw_circle / update` |
| `tests/circle_test.c` | `clear / draw_string / draw_circle / update` |
| `tests/display_test.c` | `clear / draw_string / draw_line / draw_rect / draw_circle / draw_pixel / draw_char / update` |
| `input/calibrate.c` | `clear / draw_string / update` (通过 ui_common 间接使用 draw_line/draw_hline) |

---

## 2. 抽象层方案

```
main.c  ui/*  tests/*  input/*
  只调用 display_*()，不直接 #include "ssd1306.h"
          │
   ┌──────┴──────┐
   │  display.h  │  ← 统一 API 声明
   │  display.c  │  ← 编译时根据 CONFIG 转发
   └──────┬──────┘
          │
    ┌─────┴─────┐
    │           │
display_oled    display_lcd
 (ssd1306)      (lcd_st7735)
```

### display.h 统一 API

```c
void display_init(void);
void display_clear(void);
void display_update(void);
void display_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void display_draw_hline(uint8_t x, uint8_t y, uint8_t w);
void display_draw_vline(uint8_t x, uint8_t y, uint8_t h);
void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill);
void display_draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill);
void display_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void display_draw_char(uint8_t x, uint8_t y, char c);
void display_draw_string(uint8_t x, uint8_t y, const char *str);
void display_set_fps(uint8_t fps);
void display_set_cpu(uint8_t cpu);
uint32_t display_get_frames(void);
```

---

## 3. 实施步骤

### Phase 1 — 新建抽象层（不改既有文件）

| 文件 | 操作 | 说明 |
|------|------|------|
| `main/drivers/display.h` | 新建 | 声明以上统一 API |
| `main/drivers/display.c` | 新建 | `#ifdef CONFIG_DISPLAY_OLED` → `ssd1306_*`; `#elif CONFIG_DISPLAY_LCD` → `lcd_st7735_*` |
| `main/Kconfig.projbuild` | 新建 | `choice DISPLAY_TYPE`: `CONFIG_DISPLAY_OLED` / `CONFIG_DISPLAY_LCD` |

### Phase 2 — 增强 LCD 驱动

| 文件 | 操作 | 说明 |
|------|------|------|
| `main/lcd_st7735.h` | 修改 | 补充 `lcd_set_fps()`, `lcd_set_cpu()`, `lcd_get_frames()` |
| `main/lcd_st7735.c` | 修改 | 加入 FPS/CPU 叠加层、帧计数器、仿 `ssd1306_*` 同名转发宏或包装函数 |
| `main/lcd_st7735.c` | 修改 | `lcd_clear()` 改为清黑；`lcd_draw_char/string` 改为白字黑底（匹配 OLED 单色外观） |
| `main/lcd_st7735.c` | 修改 | 注销或移除内嵌字库，改为 `#include "font_6x8.h"` 复用既有字库 |

### Phase 3 — 逐一迁移 9 个文件

按依赖顺序，每个文件只改两处：
- `#include "ssd1306.h"` → `#include "display.h"`
- `ssd1306_*()` → `display_*()`

| 顺序 | 文件 | 改动量 |
|------|------|--------|
| 1 | `main/ui/ui.c` | 2 处调用 |
| 2 | `main/ui/menu.c` | 3 处调用 |
| 3 | `main/ui/about.c` | 3 处调用 |
| 4 | `main/ui/ui_common.c` | 2 处调用 |
| 5 | `main/tests/test_ui.c` | 5 处调用 |
| 6 | `main/tests/circle_test.c` | 5 处调用 |
| 7 | `main/tests/display_test.c` | 30+ 处调用 |
| 8 | `main/input/calibrate.c` | 4 处调用 |
| 9 | `main/main.c` | 3 处调用 + 删 `"ssd1306.h"` |

### Phase 4 — 分辨率适配

OLED 128x64 vs LCD 128x160：

```
┌─────────────┐  y=0
│  菜单 UI     │  8 行 × 8px = 64px  ← 内容完全一致
├─────────────┤  y=64
│  附加信息    │  额外空间
│  版本/状态   │
└─────────────┘  y=159
```

- UI 内容顶部对齐，行为与 OLED 一致
- LCD 模式在 y=64+ 区域绘制固件版本 / FPS / 当前模式

### Phase 5 — CMakeLists

```cmake
idf_component_register(
    SRCS "main.c"
         "drivers/ssd1306.c" "drivers/font_6x8.c" "drivers/display.c"
         "ui/ui.c" "ui/ui_common.c" "ui/menu.c" "ui/about.c"
         "input/input.c" "input/calibrate.c"
         "tests/test_ui.c" "tests/circle_test.c" "tests/display_test.c"
         "lcd_st7735.c"
    INCLUDE_DIRS "." "drivers" "ui" "input" "tests"
)
```

`lcd_ui.c` 不参与主线编译，保留为独立彩色演示入口（通过 `#ifdef CONFIG_DISPLAY_LCD` 可选启用）。

---

## 4. 验证

```bash
idf.py menuconfig          # Display Configuration → 选择 OLED 或 LCD
idf.py build                # 编译通过无警告
idf.py -p COMx flash monitor  # 对应屏幕正常显示菜单
```
