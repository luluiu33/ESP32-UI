# ESP32 OLED 项目设计文档

## 一、项目概述

基于 ESP32-WROOM-32 + SSD1306 0.96 寸 OLED (128x64) 的嵌入式 UI 演示项目，支持摇杆输入、菜单导航、校准功能及多种显示测试。

## 二、系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                         main.c                              │
│   初始化 → 主循环 (固定16ms周期): input → ui → LED → FPS/CPU  │
├─────────────────────────────────────────────────────────────┤
│                         UI层 (ui/)                          │
│  ┌─────────┐ ┌──────────┐ ┌────────┐ ┌────────┐ ┌───────┐  │
│  │ menu.c  │ │ ui.c     │ │ about  │ │common  │ │lcd_ui │  │
│  │主菜单滚动│ │状态机调度│ │关于页面│ │箭头绘图│ │LCD UI │  │
│  └─────────┘ └──────────┘ └────────┘ └────────┘ └───────┘  │
├─────────────────────────────────────────────────────────────┤
│                       测试层 (tests/)                       │
│  ┌───────────┐ ┌──────────┐ ┌──────────────┐               │
│  │circle_test│ │test_ui   │ │display_test  │               │
│  │圆心映射   │ │按键指示  │ │6种动画演示   │               │
│  └───────────┘ └──────────┘ └──────────────┘               │
├─────────────────────────────────────────────────────────────┤
│                       输入层 (input/)                       │
│  ┌─────────┐ ┌───────────┐                                 │
│  │input.c  │ │calibrate  │                                 │
│  │ADC+按键 │ │校准向导   │                                 │
│  └─────────┘ └───────────┘                                 │
├─────────────────────────────────────────────────────────────┤
│                     驱动层 (drivers/)                       │
│  ┌──────────┐ ┌───────────┐ ┌───────────┐                  │
│  │ssd1306.c │ │font_6x8   │ │lcd_st7735 │                  │
│  │I2C驱动   │ │6x8字库    │ │SPI LCD驱动│                  │
│  └──────────┘ └───────────┘ └───────────┘                  │
└─────────────────────────────────────────────────────────────┘
```

## 三、目录结构与职责

| 目录 | 文件 | 职责 |
|------|------|------|
| `main/` | `main.c` | 程序入口、主循环 |
| `drivers/` | `ssd1306.c/h` | SSD1306 OLED I2C 驱动，framebuffer 绘图 API |
| `drivers/` | `font_6x8.c/h` | ASCII 6x8 像素字体表 |
| `input/` | `input.c/h` | 摇杆 ADC 采样、去抖、校准状态机 |
| `input/` | `calibrate.c/h` | 4 步校准向导 UI |
| `ui/` | `ui.c/h` | UI 状态机（6种模式切换） |
| `ui/` | `ui_common.c/h` | 共享常量 + 三角形箭头绘图 |
| `ui/` | `menu.c/h` | 主菜单滚动显示 + 选中指示 |
| `ui/` | `about.c/h` | 关于信息页，支持上下滚动 |
| `tests/` | `circle_test.c/h` | 摇杆圆边界测试 |
| `tests/` | `test_ui.c/h` | 方向键 + 中心按钮指示测试 |
| `tests/` | `display_test.c/h` | 6 种动画演示（正弦波/弹跳球/进度条/图形/字符集/跑马灯） |
| `main/` | `lcd_st7735.c/h` | ST7735 SPI LCD 驱动 (160×128 硬件 SPI) |
| `main/` | `lcd_ui.c/h` | LCD UI 调度层 |

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
    drivers/ssd1306.c
    ├── framebuf[8][128] (页面排列)
    └── I2C0 (SDA=14, SCL=13) @ 1MHz
    │    │
    │    ▼
    │   SSD1306 OLED 128x64
    │
    lcd_st7735.c
    └── SPI (MOSI=23, SCK=18, CS=5, DC=2, RST=4) @ 40MHz
         │
         ▼
    ST7735 LCD 160x128
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
  ├─ 每50帧输出 FPS / CPU 到串口
  └─ 固定 16ms 周期:
       sleep_us = 16000 - (esp_timer_get_time() - t0)
       if sleep_us > 10ms → vTaskDelay(1)
       while (< 16000) { spin }   // 精确填充至 16ms
```

- 动画时自动保持 ~62.5 FPS，工作超 16ms 时才降帧
- 空闲时 vTaskDelay(1) 释放 CPU 给空闲任务
- FPS 统计实际 `ssd1306_update()` 调用次数（无刷新时显示 0）
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

### 5.5 输入状态机

```
JS_IDLE  →  检测到方向偏移  →  JS_LEFT/RIGHT/UP/DOWN
             └→ 返回单次事件 + 切换到对应保持状态
JS_LEFT  →  回到中位(>1600)  →  JS_IDLE
JS_RIGHT →  回到中位(<2400)  →  JS_IDLE
```

方向事件只在状态切换时产生一次，避免重复触发。

### 5.6 长按退出

所有非菜单模式下，按住中心按钮 >500ms 自动回到主菜单，优先级最高（在主 switch 之前判断）。

### 5.7 OLED 双色区域利用

| 区域 | 像素行 | 用途 |
|------|--------|------|
| 黄色 | y=0~15 | 标题文字显示 |
| 蓝色 | y=16~63 | 图形和内容 |

### 5.8 菜单滚动

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
