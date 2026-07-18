# OLED_UI_Core 吸收分析与发展计划

## 当前项目状态（吸收前）

```
display.h/c            ← 抽象层已就位（OLED/LCD 编译切换）
ssd1306.c/h            ← OLED 驱动
lcd_st7735.c/h         ← LCD 驱动（含 draw_stringf）
font_6x8.c/h           ← 单一 6x8 字库
ui/ui.c                ← 6 状态扁平状态机
ui/menu.c/about.c      ← 菜单/关于
tests/display_test.c   ← 6 种动画
input/input.c          ← ADC 摇杆 + GPIO 按键
Kconfig.projbuild      ← OLED/LCD 选择
display_layout.h       ← 分辨率适配宏
```

---

## 吸收评估矩阵

逐项对比 OLED_UI_Core 组件，标记是否值得吸收及理由：

### A. 绘图函数 (Software_Driver/OLED.c)

| 功能 | 来源行 | 当前项目 | 吸收？ | 理由 |
|------|--------|---------|--------|------|
| `OLED_Printf()` | 631 | `snprintf`+`draw_string` | **★★★** | 单次调用替代手动拼接，遍布 9 个文件 |
| `OLED_PrintfMix()` | 685 | 无 | — | 中英文混排，暂不需要 |
| `OLED_ShowNum()` | 348 | 无 | **★★★** | 类型化数字输出，便利 |
| `OLED_ShowFloatNum()` | — | 无 | **★★★** | 浮点数直接显示 |
| `OLED_ClearArea()` | 151 | 无（只能全屏清） | **★★★** | 局部更新，减少闪烁 |
| `OLED_ReverseArea()` | — | 无 | **★★★** | 选中/高亮效果 |
| `OLED_Reverse()` | — | 无 | **★★** | 全屏反色 |
| `OLED_ShowStringArea()` | 785 | 无 | **★★** | 区域裁剪文本（滚动区域） |
| `OLED_PrintfArea()` | 887 | 无 | **★★** | 区域裁剪格式化输出 |
| `OLED_DrawRoundedRectangle()` | 1520 | 无 | **★★★** | UI 美化，低实现成本 |
| `OLED_DrawTriangle()` | 1185 | 无 | **★** | 基础图元，但当前无需 |
| `OLED_DrawEllipse()` | 1324 | 无 | **★** | 基础图元，但当前无需 |
| `OLED_DrawArc()` | 1437 | 无 | — | 圆弧，暂不需要 |
| `OLED_ShowImage()` | — | `draw_bitmap` | **★** | 已有等效功能 |

### B. 字库 (Software_Driver/OLED_Fonts.h)

| 内容 | 当前项目 | 吸收？ | 理由 |
|------|---------|--------|------|
| `F6x8` | 已有 `font_6x8.c` | — | 已共享 |
| `F8x16` | 无 | **★★★** | LCD 可读性提升 2 倍 |
| `F7x12` / `F10x20` | 无 | — | 非必需，暂缓 |
| 中文字库 8x8~20x20 | 无 | — | 无需求，50KB+ Flash |

### C. UI 引擎 (OLED_UI/OLED_UI.c)

| 功能 | 当前项目 | 吸收？ | 理由 |
|------|---------|--------|------|
| `MenuPage`+`MenuItem` 结构 | 硬编码 switch-case | **★★** | 吸收设计思想，重写轻量版 |
| `OLED_UI_CreateWindow()` | 无 | **★** | 弹窗暂时不需要 |
| PID 动画 | 无 | — | 当前帧驱动动画已够用 |
| 淡入淡出 | 无 | — | 非必需 |
| 长按/连击宏 | 简单 tick 检测 | **★★** | 吸收到输入层 |

### D. 输入层 (misc.h + OLED_UI_Driver.h)

| 功能 | 当前项目 | 吸收？ | 理由 |
|------|---------|--------|------|
| `BTN_stat_t` 状态机 | 简单 `gpio_get_level` | **★★★** | 去抖动+长按+press/release |
| `BtnTask()` | 无 | **★★** | 定时扫描模式 |

---

## 吸收优先级排序

### P0 — 立即吸收（高价值，低工作量）

```
① display_printf()           ← LCD 已有 draw_stringf，OLED 需补充
② display_show_num/float()   ← 新功能
③ display_clear_area()       ← 局部清除
④ display_reverse_area()     ← 选中效果
⑤ display_draw_round_rect()  ← UI 美化
⑥ font_8x16.c               ← 8×16 字库
```

### P1 — 逐步吸收（中等价值）

```
⑦ display_printf_area()      ← 区域裁剪
⑧ display_reverse()          ← 全屏反色
⑨ 轻量声明式菜单              ← 替换 switch-case
⑩ BtnTask 状态机改进 input   ← 去抖动加强
```

### P2 — 可暂缓

```
⑪ display_draw_triangle/ellipse()
⑫ 弹窗系统
⑬ 图标资源
```

---

## 分阶段实施计划

### Phase 1 — API 扩展（1 周）

目标：在 `display.h` 中增加 P0 函数，两个后端分别实现。

**`display.h` 新增声明：**

```c
// ——— 格式化输出 ———
void display_printf(uint8_t x, uint8_t y, const char *fmt, ...);

// ——— 类型数字 ———
void display_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len);
void display_show_float(uint8_t x, uint8_t y, double num, uint8_t dec);

// ——— 区域操作 ———
void display_clear_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void display_reverse_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// ——— 扩展图元 ———
void display_draw_round_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t fill);
```

**后端映射：**

| 新函数 | OLED 后端 (ssd1306) | LCD 后端 (lcd_st7735) |
|--------|---------------------|----------------------|
| `display_printf` | vsnprintf → draw_char 循环 | 直接转发 `lcd_draw_stringf` |
| `display_show_num` | sprintf → draw_string | sprintf → draw_string |
| `display_clear_area` | for + draw_pixel(0) | lcd_fill_rect(white) |
| `display_reverse_area` | for + read_pixel → write !pixel | lcd_fill_rect(white) + ... |
| `display_draw_round_rect` | Bresenham 弧 + 直线段 | 同算法 + RGB565 |

**关键实现 — `ReverseArea` 需要 `ssd1306_read_pixel()`：**

```c
// ssd1306.c 新增
uint8_t ssd1306_read_pixel(uint8_t x, uint8_t y) {
    if (x >= WIDTH || y >= HEIGHT) return 0;
    return (framebuf[y/8][x] >> (y%8)) & 1;
}
```

### Phase 2 — 字库扩展（0.5 周）

从 `OLED_Fonts.h` 移植 `F8x16` 字库：

| 文件 | 内容 |
|------|------|
| `drivers/font_8x16.c` | 96 个 ASCII 字符 × 16 字节 = 1536 字节 |
| `drivers/font_8x16.h` | `extern const uint8_t font8x16[][16]` |

`display_draw_char/string` 增加字号参数：

```c
#define FONT_6X8   0
#define FONT_8X16  1

void display_draw_char(uint8_t x, uint8_t y, char c, uint8_t size);
void display_draw_string(uint8_t x, uint8_t y, const char *str, uint8_t size);

// LCD 模式默认 FONT_8X16，OLED 模式默认 FONT_6X8
```

### Phase 3 — 代码迁移（1 周）

将既有 9 个文件中的 `snprintf+draw_string` 模式替换为 `display_printf`：

| 文件 | 替换示例 |
|------|---------|
| `tests/display_test.c` | `snprintf(pct,...) + draw_string` → `display_printf(px,6,"%d%%",v)` |
| `ui/about.c` | 逐行 draw_string → `display_printf(8,i,"%s",text)` |
| `input/calibrate.c` | `snprintf + draw_string` → `display_printf(24,2,"Step %d/4",step+1)` |
| `main.c` | FPS/CPU 格式化由 display_set_fps 处理 |

### Phase 4 — 输入增强（1 周）

从 `misc.h` 移植 `BTN_stat_t` 状态机改进 `input/input.c`：

```c
// 新结构替代简单 tick 检测
typedef struct {
    uint8_t is_pressing;
    uint8_t is_debounced;
    uint8_t is_long_pressing;
    uint8_t press_event;
    uint8_t long_press_event;
    uint32_t press_start_tick;
} btn_stat_t;
```

收益：
- 去除 `#define DEBOUNCE_MS 50` 硬编码
- 长按检测从 ui.c 下放到 input 层
- 支持 press/release 双边沿事件

---

## 不吸收的内容及理由

| 不吸收 | 理由 |
|--------|------|
| 完整 `OLED_UI.c` 框架 (1500+ lines) | 当前状态机 + 简单菜单更可维护 |
| PID 动画 (`ChangeDistance` 等) | float 运算，Flash 占用 ~2KB，当前无需平滑动画 |
| 窗口系统 (`OLED_UI_CreateWindow`) | 无弹窗需求，scene 模式已可覆盖 |
| 中文字库 (12x12~20x20) | 50KB+ Flash，无显示需求 |
| F7x12 / F10x20 字库 | 6x8 + 8x16 已覆盖 |
| 磁贴布局 (TILES) | 当前列表菜单够用 |
| 3D 立方体 (`DrawCube3D`) | 演示功能，无实际用途 |
| 编码器支持 | 使用 ADC 摇杆替代 |
| Icons (settings/wechat/alipay) | 好看但非必需，暂不引入 |

---

## 文件变更总览

| 阶段 | 新建 | 修改 | 说明 |
|------|------|------|------|
| 1 | 0 | 5 | `display.h/c` 扩 API + `ssd1306.c` 加 `read_pixel` + `lcd_st7735.c` 适配 |
| 2 | 2 | 3 | `font_8x16.c/h` 新建 + `display.h/c` 扩字号参数 + `display_layout.h` |
| 3 | 0 | 9 | 逐个文件替换 `snprintf+draw_string` → `display_printf` |
| 4 | 0 | 2 | `input.h/c` 改进按键状态机 |
| 合计 | 2 | 19 | |

**工作量估算：3-4 周（兼职），可并行 Phase 2 + Phase 3。**
