# ESP32 OLED + LCD 显示项目

基于 ESP32-WROOM-32，SSD1306 OLED（I2C）与 ST7735 LCD（SPI）双屏驱动，编译时切换。摇杆输入，菜单系统，动画演示。

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
| DC   | GPIO21    |
| RST  | GPIO22    |
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
| 阳极 | GPIO2 |
| 阴极 | GND |

## 项目结构

```
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                  # 入口：初始化，主循环轮询输入，FPS/CPU 统计
│   ├── main.h                  # 版本号 + 显示屏选择宏 (CONFIG_DISPLAY_OLED/LCD)
│   ├── drivers/
│   │   ├── display.h / .c      # 统一 display API 抽象层（编译时转发）
│   │   ├── display_layout.h    # OLED/LCD 布局宏（页面索引 + 像素坐标）
│   │   ├── ssd1306.h / .c      # SSD1306 I2C 驱动 + framebuffer 绘图 API
│   │   ├── lcd_st7735.h / .c   # ST7735 SPI LCD 驱动 (128×160)
│   │   └── font_6x8.h / .c     # 6×8 ASCII 字库 (0x20~0x7E)
│   ├── input/
│   │   ├── input.h / .c        # 输入处理：双轴ADC + 按键 + 边缘状态机 + 动态轴映射
│   │   └── calibrate.h / .c    # 4步校准 UI + NVS 持久化
│   ├── ui/
│   │   ├── ui.h / .c           # 模式状态机 (MENU/CIRCLE/TEST/DISP_TEST/CAL/ABOUT)
│   │   ├── menu.h / .c         # 滚动菜单 (5项, 3可见)
│   │   ├── about.h / .c        # 关于页 (可滚动文本)
│   │   └── ui_common.h / .c    # 共享绘图原语 + 箭头绘制
│   └── tests/
│       ├── test_ui.h / .c      # 十字箭头方向测试
│       ├── circle_test.h / .c  # 圆轨迹追踪 + 摇杆点
│       ├── display_test.h / .c # 动画演示集 (正弦波/弹球/进度条/图形/字库/跑马灯)
│       └── snake.h / .c        # LCD 贪吃蛇 (16×18 网格, 绿色蛇身, 摇杆控制)
├── docs/
│   ├── esp32-wroom-32-datasheet.md
│   ├── lcd_driver_info.md
│   ├── porting_oled_ui_core.md
│   └── project_design.md
```

## 构建与烧录

```bash
# 编辑 main/main.h，注释/取消注释 CONFIG_DISPLAY_OLED / CONFIG_DISPLAY_LCD 选择屏幕
idf.py build
idf.py -p COMx -b 1500000 flash
idf.py -p COMx monitor    # 串口输出 CPU 占用率
```

## 显示特性

### 统一抽象层
所有 UI/测试代码通过 `display_*()` API 显示，不直接操作屏幕硬件。
编译时由 `main.h` 中的宏定义（`CONFIG_DISPLAY_OLED` / `CONFIG_DISPLAY_LCD`）选择后端。
布局宏（`display_layout.h`）自动适配两种分辨率。

### OLED (SSD1306) — 128×64
- **接口**：硬件 I2C, 1MHz
- **字体**：6×8 像素，8 页面 (0~7)
- **布局**：标题 (page 0~1, 16px) + 内容 (page 2~7)

### LCD (ST7735) — 128×160
- **接口**：硬件 SPI
- **字体**：6×8 像素，20 页面 (0~19)
- **布局**：状态栏黑底白字 16px (page 0~1) + 内容区白底黑字 (page 2~19)
- **颜色**：`display_clear()` 清白屏 → 绘黑底标题栏

## 功能

### 菜单系统
- 5 个菜单项：Circle / Button / Display / Snake / About
- 滚动显示（3 项可见），摇杆上下切换，按下进入

### 测试模式
- **Circle**：圆轨迹追踪，摇柄控制亮点沿圆周移动
- **Button**：十字箭头方向测试，按下点亮中心圆
- **Display**：6 种动画演示（正弦波 / 弹球 / 进度条 / 图形 / 字库 / 跑马灯）

### 校准
- 菜单模式长按中心按钮 >500ms 进入校准
- 4 步校准 (UP / DOWN / LEFT / RIGHT)，每步按住 300ms
- 校准数据以 blob 形式写入 NVS (`cal/map`)
- 上电自动加载

### 关于 (About)
- 多行滚动文本，显示项目信息和版本

### 系统状态
- OLED：右上角实时帧率 `FPS:xx`；LCD：状态栏右侧实时帧率 `FPS:xx`
- 串口每 ~1s 输出 CPU 占用率与帧率：`CPU:12%  FPS:35`
- 输入时 LED 亮起指示

## 引脚占用汇总

| GPIO | 功能 |
|------|------|
| 2    | LED 输入指示 |
| 5    | LCD CS |
| 13   | I2C SCL |
| 14   | I2C SDA |
| 18   | LCD SCK |
| 21   | LCD DC |
| 22   | LCD RST |
| 23   | LCD MOSI |
| 32   | 摇杆按键 SW |
| 34   | 摇杆 X 轴 (ADC1_CH6) |
| 35   | 摇杆 Y 轴 (ADC1_CH7) |
