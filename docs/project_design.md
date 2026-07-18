# ESP32 OLED + LCD 项目设计文档

## 一、项目概述

基于 ESP32-WROOM-32，SSD1306 OLED (128x64) 与 ST7735 LCD (128x160) 双屏驱动（编译时切换），摇杆输入、菜单导航、校准功能及多种显示测试的嵌入式 UI 演示项目。

## 二、系统架构

```
┌──────────────────────────────────────────────────────────────┐
│                         main.c                               │
│   初始化 → 主循环 (固定16ms周期): input → ui → LED → FPS/CPU   │
├──────────────────────────────────────────────────────────────┤
│                         UI层 (ui/)                           │
│  ┌─────────┐ ┌──────────┐ ┌────────┐ ┌──────────┐           │
│  │ menu.c  │ │ ui.c     │ │ about  │ │common    │           │
│  │主菜单滚动│ │状态机调度│ │关于页面│ │箭头绘图   │           │
│  └─────────┘ └──────────┘ └────────┘ └──────────┘           │
├──────────────────────────────────────────────────────────────┤
│                       测试层 (tests/)                        │
│  ┌───────────┐ ┌──────────┐ ┌──────────────┐                │
│  │circle_test│ │test_ui   │ │display_test  │                │
│  │圆心映射   │ │按键指示  │ │6种动画演示   │                │
│  └───────────┘ └──────────┘ └──────────────┘                │
├──────────────────────────────────────────────────────────────┤
│                       输入层 (input/)                        │
│  ┌─────────┐ ┌───────────┐                                  │
│  │input.c  │ │calibrate  │                                  │
│  │ADC+按键 │ │校准向导   │                                  │
│  └─────────┘ └───────────┘                                  │
├──────────────────────────────────────────────────────────────┤
│                   统一显示抽象层 (drivers/)                   │
│  ┌───────────┐ ┌────────────────┐                           │
│  │display.h  │ │ display.c      │                           │
│  │统一 API   │ │ 编译时转发     │                           │
│  └───────────┘ └───────┬────────┘                           │
│                        │                                     │
│           ┌────────────┴────────────┐                        │
│           ▼                         ▼                        │
│  ┌──────────────┐         ┌──────────────┐                   │
│  │ ssd1306.c/h  │         │ lcd_st7735   │                   │
│  │ OLED I2C 驱动│         │ SPI LCD 驱动 │                   │
│  └──────────────┘         └──────────────┘                   │
│  ┌───────────┐                                               │
│  │ font_6x8  │  ← 字库被两个后端共用                         │
│  └───────────┘                                               │
└──────────────────────────────────────────────────────────────┘
```

## 三、目录结构与职责

| 目录 | 文件 | 职责 |
|------|------|------|
| 目录 | 文件 | 职责 |
|------|------|------|
| `main/` | `main.c` | 程序入口、主循环 |
| `main/` | `Kconfig.projbuild` | Display type 说明（实际由 `main.h` 定义 `CONFIG_DISPLAY_*` 宏切换） |
| `drivers/` | `display.h/c` | 统一显示抽象层 API，编译时转发至 OLED 或 LCD 后端 |
| `drivers/` | `display_layout.h` | OLED/LCD 布局宏（页面索引 + 像素坐标） |
| `drivers/` | `ssd1306.h/c` | SSD1306 OLED I2C 驱动，framebuffer 绘图 API |
| `drivers/` | `font_6x8.h/c` | ASCII 6x8 像素字体表 |
| `drivers/` | `lcd_st7735.h/c` | ST7735 SPI LCD 驱动 (128×160) |
| `input/` | `input.c/h` | 摇杆 ADC 采样、去抖、校准状态机 |
| `input/` | `calibrate.c/h` | 4 步校准向导 UI |
| `ui/` | `ui.c/h` | UI 状态机（6种模式切换） |
| `ui/` | `ui_common.c/h` | 共享常量 + 三角形箭头绘图 |
| `ui/` | `menu.c/h` | 主菜单滚动显示 + 选中指示 |
| `ui/` | `about.c/h` | 关于信息页，支持上下滚动 |
| `tests/` | `circle_test.c/h` | 摇杆圆边界测试 |
| `tests/` | `test_ui.c/h` | 方向键 + 中心按钮指示测试 |
| `tests/` | `display_test.c/h` | 6 种动画演示（正弦波/弹跳球/进度条/图形/字符集/跑马灯） |

## 四、核心数据流

```
摇杆硬件 (ADC CH6/CH7) + 按键 (GPIO32)
        │
        ▼
   input/input.c
   ├── input_read()        → input_event_t (单次事件)
   ├── input_get_state()   → joy_state_t (连续状态)
   └── input_get_calibrated() → 校准后 ±2048 范围
        │
        ▼
   ui/ui.c (状态机)
   ├── UI_MENU      → menu.c (主菜单)
   ├── UI_CIRCLE    → circle_test.c (画圆)
   ├── UI_TEST      → test_ui.c (按键测试)
   ├── UI_DISP_TEST → display_test.c (6动画)
   ├── UI_CAL       → calibrate.c (校准)
   └── UI_ABOUT     → about.c (关于)
        │
        ▼
    drivers/display.c (统一抽象层)
    ├── 每个 display_* 函数内含独立 #ifdef 守卫
    ├── CONFIG_DISPLAY_OLED → ssd1306_*() 调用
    └── CONFIG_DISPLAY_LCD  → lcd_st7735_*() 调用
        （双屏模式时两条路径同时编译）
         │
         ├── [OLED 路径] drivers/ssd1306.c
         │   ├── framebuf[8][128] (页面排列)
         │   └── I2C0 (SDA=14, SCL=13) @ 1MHz → SSD1306 128x64
         │
         └── [LCD 路径] drivers/lcd_st7735.c
             └── SPI (MOSI=23, SCK=18, CS=5, DC=2, RST=4, BL=22) @ 40MHz
                  └── ST7735 LCD 128x160
```

## 五、关键设计决策

### 5.1 状态机 UI

```
typedef enum { UI_MENU, UI_CIRCLE, UI_TEST, UI_DISP_TEST, UI_CAL, UI_ABOUT } ui_mode_t;
```

采用扁平枚举 + `switch-case` 实现模式切换，简单直接，适合功能有限的设备。每种模式对应一个独立绘制函数，通过 `ui_process(ev, js)` 统一调度。

### 5.2 固定帧周期主循环

```
循环时间线:
  t0 = esp_timer_get_time()
  ├─ input_read + ui_process + gpio
  t1 = esp_timer_get_time()
  ├─ work_sum += t1 - t0   (实际工作时间)
  ├─ 每50帧:
  │    ├─ display_update_fps(elapsed_ms)  // 计算 FPS 并格式化
  │    └─ printf("CPU:%d%%\n", cpu)       // CPU 占用率
  ├─ display_update_title()              // 局部刷新标题栏
  └─ 固定 16ms 周期:
        sleep_us = 16000 - (esp_timer_get_time() - t0)
        if sleep_us > 10ms → vTaskDelay(1)
        while (< 16000) { spin }   // 精确填充至 16ms
```

- 动画时自动保持 ~62.5 FPS，工作超 16ms 时才降帧
- 空闲时 vTaskDelay(1) 释放 CPU 给空闲任务
- FPS 统计实际 `display_update()` 调用次数（无刷新时显示 0）
- CPU% 串口输出，基于 `work_sum / 总时间`

### 5.3 惰性重绘优化

- **circle_test.c**: 圆点位置未变化时不刷新
- **test_ui.c**: 5-bit 掩码未变化时不刷新
- **menu.c**: `menu_is_dirty()` 检测焦点变化后才重绘

避免无谓的 I2C 传输，降低功耗、减少闪烁。

### 5.4 摇杆校准

```
校准步骤:
  1. 摇杆推 UP → 记录 ADC
  2. 摇杆推 DOWN → 记录 ADC
  3. 摇杆推 LEFT → 记录 ADC
  4. 摇杆推 RIGHT → 记录 ADC
  → 自动判断 swap_xy / invert_x / invert_y
  → 存入 NVS (key="cal", blob="map")
  → 启动时 input_load_cal() 恢复
```

校准结果存储到 NVS Flash，掉电不丢失。

**校准方向反转：** 当 `invert_x=1` 时，X 轴物理方向与电气方向相反：
- 低 ADC 值 → RIGHT（原来是 LEFT）
- 高 ADC 值 → LEFT（原来是 RIGHT）
`invert_y` 同理。阈值 `ADC_LO=1600 / ADC_HI=2400` 保持不变，仅交换方向映射。

**V0.1.1 bug 修复：** 旧代码在反转时互换 `lr_lo/lr_hi` 阈值但比较符号不变，导致 `lr_raw < 2400` 覆盖了中心区（死区消失），仅剩 1 个方向可检测。修复后保持阈值不变，仅交换 LEFT/RIGHT 映射。

### 5.5 长按校准

菜单模式下，按住中心按钮（SW, GPIO32）>500ms 直接进入校准模式。优先级高于方向键事件。

```
ui_process():
  if center_press_ms && (now - center) > LONG_PRESS_MS:
    center_press_ms = 0          // 防重复触发
    if mode == UI_MENU → 进入 UI_CAL
    else → 退出到主菜单 (旧: 仅非菜单模式退出)
```

**注意：** 长按退出只在非菜单模式生效；菜单模式长按进入校准。两个功能互斥。

### 5.6 输入状态机

```
JS_IDLE  →  检测到方向偏移  →  JS_LEFT/RIGHT/UP/DOWN
              └→ 返回单次事件 + 切换到对应保持状态
JS_LEFT  →  回到中位(>1600或<2400) → JS_IDLE
JS_RIGHT →  回到中位(<2400或>1600) → JS_IDLE
```

方向事件只在状态切换时产生一次，避免重复触发。
反转模式下释放条件随之转换：`invert_x` 时 LEFT 释放为 `lr_raw <= ADC_HI`，RIGHT 释放为 `lr_raw >= ADC_LO`。

### 5.7 长按退出

所有非菜单模式下，按住中心按钮 >500ms 自动回到主菜单，优先级最高（在主 switch 之前判断）。

### 5.8 分辨率适配（display_layout.h）

所有 UI/测试代码通过 `display_layout.h` 中的宏访问坐标，无需关心实际分辨率。

#### 5.8.1 显示屏参数对比

| 参数 | OLED (SSD1306) | LCD (ST7735) |
|------|---------------|--------------|
| 分辨率 | 128 × 64 | 128 × 160 |
| 总页面数 | 8 (0~7) | 20 (0~19) |
| 标题栏高度 | 16 px (pages 0-1) | 16 px (pages 0-1) |
| 内容区起始页 `CONTENT_Y` | 2 | 2 |
| 内容区页面数 `CONTENT_H` | 6 (48 px) | 18 (144 px) |
| 内容区文本行数 | 6 (rows 2~7) | 18 (rows 2~19) |
| 每行最大字符数 (6×8 font) | 21 | 21 |

#### 5.8.2 布局常量表

所有值以 `OLED_` / `LCD_` 前缀命名，`#define` 无条件暴露。UI 代码统一使用无前缀别名（单屏时指向对应前缀，双屏时指向 `OLED_*`）。

**菜单 (`menu.c`)**

| 宏 | OLED | LCD | 说明 |
|----|------|-----|------|
| `MENU_TITLE_Y` | page 2 | page 2 | 标题行 (当前未使用) |
| `MENU_VISIBLE` | 3 | 10 | 可见项数上限 |
| `MENU_ITEM_Y(i)` | page 3+2i | page 6+2i | 第 i 项的页面行，首项与标题栏空 1 行 |

OLED 菜单项排列在 pages 3,5,7；LCD 在 pages 6,8,10...（首项与标题栏空 1 行）。

**关于 (`about.c`)**

| 宏 | OLED | LCD | 说明 |
|----|------|-----|------|
| `ABOUT_TEXT_Y(i)` | page 2+i | page 2+i | 第 i 行文本的页面 |
| `ABOUT_VISIBLE` | 6 (`CONTENT_H`) | 6 (`CONTENT_H`) | 一次可见行数，受 OLED 内容区限制 |

两侧`ABOUT_VISIBLE`均硬编码为 8，超过时上下滚动。

**校准 (`calibrate.c`)**

| 宏 | OLED | LCD | 说明 |
|----|------|-----|------|
| `CAL_TITLE_Y` | page 2 | page 2 | 标题 |
| `CAL_STEP_Y` | page 3 | page 3 | 步骤指示 |
| `CAL_LABEL_Y` | page 6 | page 9 | 提示文字 |
| `CAL_ARROW_CX` | 64 px | 64 px | 箭头中心 X |
| `CAL_ARROW_CY` | 40 px | 88 px | 箭头中心 Y |

**测试标题**

| 宏 | OLED | LCD | 说明 |
|----|------|-----|------|
| `TEST_TITLE_Y` | page 2 | page 2 | 测试名称行 |

**像素坐标（画圆/动画）**

| 宏 | OLED | LCD | 说明 |
|----|------|-----|------|
| `CX` / `CY` | (64, 43) | (64, 100) | 内容区中心 |
| `GAP` | 16 px | 20 px | 间距 |
| `CIRCLE_R` | 5 px | 8 px | 圆点半径 |
| `CIRCLE_CX` / `CIRCLE_CY` | (64, 43) | (64, 100) | 圆周中心 |
| `CIRCLE_RAD` | 18 px | 50 px | 圆周半径 |
| `TEST_Y_MIN` / `TEST_Y_MAX` | 16 / 63 | 32 / 152 | 测试动画 Y 边界 |

#### 5.8.3 双屏模式

同时启用 `CONFIG_DISPLAY_OLED` + `CONFIG_DISPLAY_LCD` 时：

- 无前缀别名 → `OLED_*`（较紧约束）
- `LCD_*` 前缀常量可直接用于 LCD 独占绘制
- `display_*` API 自动向两块显存同步绘制
- 超出 OLED 边界的像素仅在 LCD 上可见

如需让 LCD 在双屏模式下跑满全分辨率，UI 函数需改为两遍绘制（一遍 `OLED_*` 定位一遍 `LCD_*` 定位）。

### 5.9 标题栏

两块显示屏共享标题栏设计（pages 0-1, 16px）：

```
┌─────────────────────────────────────────────┐
│ Main Menu                     FPS:xx       │  ← page 0 (8px)
│                                             │  ← page 1 (8px) — 留白
└─────────────────────────────────────────────┘
│ (content area starts at page 2)              │
```

| 属性 | OLED | LCD | 实现位置 |
|------|------|-----|---------|
| 标题位置 | 左对齐 x=0 | 左对齐 x=0 | `display.c` → `display_clear()` / `display_update_title()` |
| FPS 位置 | 右对齐 `"FPS:xx"` | 右对齐 `"FPS:xx"` | 同上 |
| 颜色 | 白字黑底 | 白字黑底 | `ssd1306_draw_string(白)` / `lcd_draw_string(白,黑)` |
| 更新策略 | 每帧局部刷新 pages 0-1 | 每帧局部刷新 (0,0)-(127,15) | `display_update_title()` |
| 标题切换时机 | 每个 mode 的 enter/exit | 同左 | `ui.c` 中 `display_set_title()` |

标题栏在 `display_clear()` 时同时绘制（全屏刷新）和 `display_update_title()` 时单独刷新（局部优化）。FPS 计算与刷新由 `display_update_fps(elapsed_ms)` 完成（`display.c:197`，在 `main.c` 主循环每 50 帧调用一次）。该函数内部读取 `ssd1306_get_frames()` / `lcd_get_frames()` 并格式化对应的 FPS 缓冲区。

### 5.10 菜单界面

```
> Circle          ← MENU_ITEM_Y(0), 焦点项前缀 '>'
                  ← 空行 (行距 2 pages)
  Button          ← MENU_ITEM_Y(1)
                  ← 空行
  Display         ← MENU_ITEM_Y(2)
```

| 属性 | 值 |
|------|-----|
| 菜单项 | Circle / Button / Display / About |
| 焦点指示器 | `>` 字符 + 行首空格 |
| 行距 | 1 空行（每项占 2 个 text rows = 16px） |
| 聚焦项颜色 | 无区分（单色屏仅 `>` 指示） |
| 滚动 | `scroll_y` 自动跟随焦点，焦点移出可见区时调整 |
| 重绘优化 | `menu_is_dirty()` 仅焦点变化时重绘 |
| 进入下一级 | 方向键 RIGHT 确认，由 `ui.c` 状态机调度 |
| 可见项 (OLED) | 3 项（`MENU_VISIBLE=3`） |
| 可见项 (LCD) | 10 项（`MENU_VISIBLE=10`，但共 4 项无需滚动） |

### 5.11 关于页面

```
== ESP32 LCD ==
ST7735S 128x160
SPI 26MHz
Dual display
Joystick + Menu
Calibration
Circle / Button
Display test
Firmware v0.0.0.2
Scroll: Up/Down
```

| 属性 | 值 |
|------|-----|
| 文本行数 | 8 行（OLED）/ 12 行（LCD/双屏） |
| 一次可见 | `CONTENT_H`（OLED 6 行，LCD 18 行，双屏时受 OLED 限制为 6） |
| 起始 X | 8px（不贴边） |
| 版本行 | `"Firmware " FW_VERSION_STR` → `v0.0.0.2` |
| 滚动 | 方向键 UP/DOWN |
| 重绘触发 | 仅 `scroll_y` 变化时调用 `about_draw()` |

### 5.12 校准向导

4 步校准，每步提示用户将摇杆推至极限方向：

| 步骤 | 标题 | 步骤指示 | 箭头标签 |
|------|------|---------|---------|
| 1 | `Cal UP` | `Step 1/4` | `[UP]` |
| 2 | `Cal DOWN` | `Step 2/4` | `[DOWN]` |
| 3 | `Cal LEFT` | `Step 3/4` | `[LEFT]` |
| 4 | `Cal RIGHT` | `Step 4/4` | `[RIGHT]` |
| 完成 | — | — | 回到主菜单 |

| 属性 | OLED | LCD |
|------|------|-----|
| 标题行 `CAL_TITLE_Y` | page 2 | page 2 |
| 步骤行 `CAL_STEP_Y` | page 3 | page 3 |
| 提示行 `CAL_LABEL_Y` | page 6 | page 9 |
| 方向箭头 `(CX, CY)` | (64, 43) | (64, 100) |
| 箭头颜色 | 实心 | 实心 |

箭头由 `ui_common.c:draw_arrow()` 绘制：3 条边构成三角形，扫描线填充算法实现实心。方向映射 `0=UP, 1=DOWN, 2=LEFT, 3=RIGHT`。

### 5.13 五方向测试

方向键和中心按钮的状态显示：

```
          ↑                          ← 箭头 UP (pos[0])
    ←     ●     →                  ← 箭头 LEFT(pos[2]), 中心圆, 箭头 RIGHT(pos[3])
          ↓                          ← 箭头 DOWN(pos[1])
```

| 属性 | 值 |
|------|-----|
| 方向箭头 | `draw_arrow(pos[i], fill)`，激活时实心，否则空心 |
| 中心按钮 | `display_draw_circle(CX, CY, CIRCLE_R, fill)`，按下实心 |
| 位置公式 | `pos[0]=(CX, CY-GAP)` / `pos[1]=(CX, CY+GAP)` / `pos[2]=(CX-GAP, CY)` / `pos[3]=(CX+GAP, CY)` / `pos[4]=(CX, CY)` |
| 重绘优化 | 5-bit 状态掩码无变化时跳过 |
| OLED 间距 GAP | 16px |
| LCD 间距 GAP | 20px |

### 5.14 Circle 测试

```
           . ← 摇杆控制的小圆点 (CIRCLE_R=3px)
      ┌───────┐
      │  O    │ ← 边界大圆 (CIRCLE_RAD=22/40px)
      │   ╬   │ ← 中心标记 (DOT_CENTER_R=2px)
      └───────┘
```

| 属性 | OLED | LCD |
|------|------|-----|
| 边界圆半径 `CIRCLE_RAD` | 18px | 50px |
| 边界圆颜色 | 空心 | 空心 |
| 中心点半径 `DOT_CENTER_R` | 2px | 2px |
| 中心点颜色 | 实心 | 实心 |
| 光标半径 `DOT_R` | 3px | 3px |
| 中心坐标 `(CIRCLE_CX, CIRCLE_CY)` | (64, 43) | (64, 100) |
| 坐标映射 | 经校准 ±2048 映射到 ±`CIRCLE_RAD` |
| 边界裁剪 | 超出时按比例缩小至圆周上 |
| 重绘优化 | 光标像素位置未变时跳过 |

### 5.15 Display 测试

6 种动画模式，左右键切换：

| 序号 | 名称 | 描述 |
|------|------|------|
| 0 | Sine Wave | 128px 宽正弦波，相位自旋 |
| 1 | Bounce Ball | 重力反弹小球（边界 +Y_MIN/Y_MAX） |
| 2 | Progress | 水平进度条 0→100%→0 往返，中央文字 |
| 3 | Shapes | 静态几何构图（矩形/圆/线） |
| 4 | Charset | ASCII 32~126 字符矩阵展示 |
| 5 | Marquee | 4 角爬行亮线（36px 尾迹） |

各模式调用 `display_clear()` 后全屏绘制，无增量优化（每帧全刷）。

### 5.16 色彩方案

| 元素 | OLED | LCD |
|------|------|-----|
| 背景色 | 黑 (0) | 白 (0xFFFF) |
| 前景色 | 白 (1) | 黑 (0x0000) |
| 标题栏背景 | 黑 (0) | 黑 (0x0000) |
| 标题栏文字 | 白 (1) | 白 (0xFFFF) |
| 菜单 `>` 选中 | 白 (1) | 黑 (0x0000) |
| 箭头填充色 | 白 (1) | 黑 (0x0000) |
| 箭头空心色 | 白 (1) | 黑 (0x0000) |
| 边界圆 | 空心 (1) | 空心 (0x0000) |

### 5.17 字体

- **字库文件**: `drivers/font_6x8.h` / `font_6x8.c`
- **规格**: 6×8 像素等宽，ASCII 32~126（可打印字符）
- **字符间距**: 无（相邻字符紧贴）
- **行间距**: 8px（一个字符高度）
- **存储格式**: `font6x8[95][6]`，每字符 6 bytes，每 byte 对应一列，bit 对应行

### 5.18 菜单滚动

- 4 个菜单项：Circle、Button、Display、About（Settings 已移除，校准仅通过长按进入）
- 最多显示 3 项（`MENU_VISIBLE = 3`）
- 焦点移出可见范围时自动调整 `scroll_y`
- 行距 2 行（`1 + i * 2`），留空行美观

## 六、硬件配置

| 外设 | 接口 | 引脚 |
|------|------|------|
| OLED SSD1306 | I2C0 | SCL=13, SDA=14 |
| LCD ST7735 | SPI2 | MOSI=23, SCK=18, CS=5, DC=2, RST=4, BL=22(PWM) |
| 摇杆 X 轴 | ADC1_CH6 | GPIO34 |
| 摇杆 Y 轴 | ADC1_CH7 | GPIO35 |
| 摇杆按键 | GPIO_IN | GPIO32 (上拉) |
| 指示灯 LED | GPIO_OUT | GPIO2 |

- I2C 频率: 1MHz
- ADC 精度: 12-bit (0~4095)
- ADC 衰减: 11dB (量程 ~3.6V)
- 死区阈值: ADC_LO=1600, ADC_HI=2400
- 中心值: 2048
