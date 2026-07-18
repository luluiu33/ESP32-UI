# ESP32 OLED + LCD 显示项目

基于 ESP32-WROOM-32，SSD1306 OLED（I2C）与 ST7735 LCD（SPI）双屏驱动，摇杆输入，菜单系统，动画演示。

## 硬件连接

### OLED (SSD1306, I2C)

| 引脚 | ESP32 GPIO |
|------|-----------|
| SCL  | GPIO13    |
| SDA  | GPIO14    |
| VCC  | 3.3V      |
| GND  | GND       |

### LCD (ST7735, SPI)

| 引脚 | ESP32 GPIO |
|------|-----------|
| MOSI | GPIO23    |
| SCK  | GPIO18    |
| CS   | GPIO5     |
| DC   | GPIO2     |
| RST  | GPIO4     |
| BL   | GPIO22 (PWM) |
| VCC  | 3.3V      |
| GND  | GND       |

### 摇杆 (Joystick)

| 摇杆引脚 | 功能 | ESP32 GPIO | 说明 |
|----------|------|-----------|------|
| VRX      | X 轴 | GPIO34 (ADC1_CH6) | 仅输入，0~3.3V |
| VRY      | Y 轴 | GPIO35 (ADC1_CH7) | 仅输入，0~3.3V |
| SW       | 按键 | GPIO32 | 外部上拉，按下拉低 |
| VCC      | 电源 | 3.3V | |
| GND      | 地   | GND | |

> ADC 判据 (12-bit, 0~4095)：1600~2400 为死区（回中），< 1600 为左/上，> 2400 为右/下。

### LED

| LED | ESP32 GPIO |
|-----|-----------|
| 阳极 | GPIO2（与 LCD DC 共用） |
| 阴极 | GND |

## 项目结构

```
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                  # 入口：初始化，主循环轮询输入，FPS/CPU 统计
│   ├── drivers/
│   │   ├── ssd1306.h / .c      # SSD1306 I2C 驱动 + 6×8 字库 + 绘图API
│   │   └── font_6x8.h / .c     # 6×8 ASCII 字库 (0x20~0x7E)
│   ├── input/
│   │   ├── input.h / .c        # 输入处理：双轴ADC + 按键 + 边缘状态机 + 动态轴映射
│   │   └── calibrate.h / .c    # 4步校准 UI + NVS 持久化
│   ├── ui/
│   │   ├── ui.h / .c           # 模式状态机 (MENU/CIRCLE/TEST/DISP_TEST/CAL/ABOUT)
│   │   ├── menu.h / .c         # 滚动菜单 (5项, 3可见)
│   │   ├── about.h / .c        # 关于页 (可滚动文本)
│   │   └── ui_common.h / .c    # 共享绘图原语
│   ├── tests/
│   │   ├── test_ui.h / .c      # 十字箭头方向测试
│   │   ├── circle_test.h / .c  # 圆轨迹追踪 + 摇杆点
│   │   └── display_test.h / .c # 动画演示集 (正弦波/弹球/进度条/图形/字库/跑马灯)
│   └── lcd_st7735.h / .c       # ST7735 SPI LCD 驱动 (160×128)
│   └── lcd_ui.h / .c           # LCD UI 层
└── docs/
    └── esp32-wroom-32-datasheet.md
```

## 构建与烧录

```bash
idf.py build
idf.py -p COMx -b 1500000 flash
idf.py -p COMx monitor    # 串口输出 CPU 占用率
```

## 显示特性

### OLED (SSD1306)
- **分辨率**：128×64 像素
- **黄色区域**：前 16 像素行 (y=0~15)，标题文字
- **蓝色区域**：后 48 像素行 (y=16~63)，图形内容
- **字体**：6×8 像素
- **接口**：硬件 I2C, 400kHz

### LCD (ST7735)
- **分辨率**：160×128 像素 (RGB)
- **接口**：硬件 SPI

## 功能

### 菜单系统
- 5 个菜单项：Circle / Button / Display / Settings / About
- 滚动显示（3 项可见），摇杆上下切换，按下进入

### 测试模式
- **Circle**：圆轨迹追踪，摇柄控制亮点沿圆周移动
- **Button**：十字箭头方向测试，按下点亮中心圆
- **Display**：6 种动画演示（正弦波 / 弹球 / 进度条 / 图形 / 字库 / 跑马灯）

### 校准 (Settings)
- 4 步校准 (UP / DOWN / LEFT / RIGHT)，每步按住 300ms
- 校准数据以 blob 形式写入 NVS (`cal/map`)
- 上电自动加载

### 关于 (About)
- 多行滚动文本，显示项目信息和版本

### 系统状态
- OLED 右上角实时帧率 `F:xx`（仅统计实际屏幕刷新次数）
- 串口每 ~1s 输出 CPU 占用率与帧率：`CPU:12%  FPS:35`
- 输入时 LED 亮起指示

## 引脚占用汇总

| GPIO | 功能 |
|------|------|
| 2    | LCD DC / LED 输入指示 |
| 4    | LCD RST |
| 5    | LCD CS |
| 13   | I2C SCL |
| 14   | I2C SDA |
| 18   | LCD SCK |
| 22   | LCD BL (PWM) |
| 23   | LCD MOSI |
| 32   | 摇杆按键 SW |
| 34   | 摇杆 X 轴 (ADC1_CH6) |
| 35   | 摇杆 Y 轴 (ADC1_CH7) |
