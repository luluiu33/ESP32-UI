# OLED_UI_Core 移植指南 — ESP32 项目

## 概述

将 OLED_UI_Core（STM32F4 HAL 库）移植到当前 ESP32 (ESP-IDF) 项目。

## 移植架构

```
当前项目                         移植后
─────────                      ──────
main/CMakeLists.txt            main/CMakeLists.txt
main/main.c                    main/main.c (适配)
  ui/                            OLED_UI_Core/
  drivers/                         OLED_UI_Launcher.c/h
  input/                          OLED_UI/OLED_UI.c/h
  tests/                          OLED_UI/OLED_UI_MenuData.c/h
  ...                             Driver/Hardware_Driver/
                                    OLED_UI_Driver.c/h
                                    OLED_driver.c/h
                                  Driver/Software_Driver/
                                    OLED.c/h
                                    OLED_Fonts.c/h
                                    misc.c/h
```

## 移植步骤

### 步骤 1：复制代码

将 `Examples/OLED_UI_Core/HAL/OLED_UI_Core/` 复制到 `main/OLED_UI_Core/`

### 步骤 2：硬件驱动适配 (`OLED_driver.c`)

OLED_UI_Core 依赖底层 `OLED_driver.c` 提供以下 API，需要在 ESP32 上重新实现：

| 函数 | 作用 | 当前项目对应 |
|------|------|-------------|
| `OLED_Init()` | 初始化 OLED | `ssd1306_init()` |
| `OLED_Clear()` | 清屏 | `ssd1306_clear()` |
| `OLED_Update()` | 刷新显示 | `ssd1306_update()` |
| `OLED_DrawPixel(x,y,color)` | 画点 | `ssd1306_draw_pixel()` |
| `OLED_DrawRectangle(x,y,w,h,fill)` | 画矩形 | `ssd1306_draw_rect()` |
| `OLED_DrawLine(x0,y0,x1,y1)` | 画线 | `ssd1306_draw_line()` |
| `OLED_DrawCircle(cx,cy,r,fill)` | 画圆 | `ssd1306_draw_circle()` |
| `OLED_ShowChar(x,y,chr,size)` | 显示字符 | `ssd1306_draw_char()` |
| `OLED_ShowString(x,y,str,size)` | 显示字符串 | `ssd1306_draw_string()` |
| `OLED_ReverseArea(x,y,w,h)` | 区域反色 | 新增实现 |
| `OLED_ClearArea(x,y,w,h)` | 区域清空 | 新增实现 |
| `OLED_ShowImageArea(...)` | 显示图片 | `ssd1306_draw_bitmap()` |
| `OLED_Printf(x,y,font,fmt,...)` | 格式化输出 | `ssd1306_draw_string()` |
| `OLED_PrintfMixArea(...)` | 区域格式化输出 | 新增实现 |
| `OLED_ShowFloatNum(x,y,num,font)` | 显示浮点数 | `sprintf`+`draw_string` |
| `OLED_DrawRoundedRectangle(...)` | 圆角矩形 | 新增实现 |
| `OLED_SetColorMode(mode)` | 设置颜色模式 | framebuf 取反 |
| `OLED_Brightness(val)` | 设置亮度 | `ssd1306` command |

**移植要点：**

```c
// SSD1306 framebuf 格式: framebuf[page=0..7][x=0..127]
// 每个 page 8 像素行
// page = y / 8, bit = y % 8

// OLED_UI_Core 使用:
//   OLED_WIDTH=128, OLED_HEIGHT=64
//   颜色: 0=灭(黑色), 1=亮(白色)
//   DARKMODE: 0=黑底白字, 1=白底黑字

// 关键适配函数:
void OLED_ReverseArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t j = 0; j < h; j++)
        for (int16_t i = 0; i < w; i++)
            ssd1306_draw_pixel(x + i, y + j,
                !ssd1306_read_pixel(x + i, y + j));
                // 需要新增 ssd1306_read_pixel
}

void OLED_ClearArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t j = 0; j < h; j++)
        for (int16_t i = 0; i < w; i++)
            ssd1306_draw_pixel(x + i, y + j, 0);
}
```

### 步骤 3：硬件抽象适配 (`OLED_UI_Driver.c`)

OLED_UI_Core 需要以下硬件级功能：

| 需求 | STM32 HAL | ESP32 ESP-IDF |
|------|-----------|---------------|
| 定时器中断 20ms | `HAL_TIM_Base_Start_IT(&htim1)` | `esp_timer_create()` + `esp_timer_start_periodic()` |
| GPIO 按键读取 | `HAL_GPIO_ReadPin()` | `gpio_get_level()` |
| 编码器 | `TIM_Encoder_Init()` | 使用 ADC 摇杆替代 |
| 微秒级延迟 | `HAL_Delay()` | `vTaskDelay()` |

**确认/返回按键映射：**

```c
// OLED_UI_Core 期望 4 个按键: Enter, Back, Up, Down
// 当前项目只有 1 个按钮 (GPIO32) + 摇杆方向

// 建议映射方案:
//   Enter = 摇杆按键 (GPIO32) 短按
//   Back  = 摇杆按键 (GPIO32) 长按 (>1000ms)
//   Up    = 摇杆上推   (ADC CH6/7 < 1600)
//   Down  = 摇杆下推   (ADC CH6/7 > 2400)

// 在 OLED_UI_Driver.c 中实现:
uint8_t Key_GetEnterStatus(void) { return !gpio_get_level(GPIO_NUM_32); }
uint8_t Key_GetBackStatus(void)  { /* 长按检测 */ }
uint8_t Key_GetUpStatus(void)    { return adc_raw < ADC_LO; }
uint8_t Key_GetDownStatus(void)  { return adc_raw > ADC_HI; }
```

### 步骤 4：菜单数据适配 (`OLED_UI_MenuData.c`)

OLED_UI_Core 的菜单数据与当前项目功能对齐：

```c
// 当前项目菜单:
//   Circle  → circle_test
//   Button  → test_ui
//   Display → display_test (6 demos)
//   Settings → calibrate
//   About   → about

// 移植为 OLED_UI_Core 格式:
MenuItem MainMenuItems[] = {
    {.General_item_text = "Circle",
     .General_callback = EnterCircleTest,
     .General_SubMenuPage = NULL},
    {.General_item_text = "Button",
     .General_callback = EnterButtonTest,
     .General_SubMenuPage = NULL},
    {.General_item_text = "Display",
     .General_callback = NULL,
     .General_SubMenuPage = &DisplayMenuPage},
    {.General_item_text = "Settings",
     .General_callback = NULL,
     .General_SubMenuPage = &SettingsMenuPage},
    {.General_item_text = "About",
     .General_callback = NULL,
     .General_SubMenuPage = &AboutMenuPage},
    {.General_item_text = NULL},  // 结束标记
};
```

### 步骤 5：显示测试接入

将当前 `tests/display_test.c` 的 6 种动画注册为 OLED_UI_Core 的回调函数或子页面。

```c
// 方式 A: 作为回调函数 (推荐)
void ShowSineWave(void) {
    // 直接调用 draw_sine()
    // 需要 OLED_UI_Core 提供 "全屏绘制模式"
}

// 方式 B: 作为窗口
MenuWindow SineWaveWindow = {
    .General_Width = 128,
    .General_Height = 64,
    .General_WindowType = WINDOW_RECTANGLE,
    .General_ContinueTime = 60.0,  // 持续60帧
};
```

### 步骤 6：中断与主循环集成

```c
// OLED_UI_Core 要求:
//   1. OLED_UI_InterruptHandler() 每 20ms 调用一次
//   2. OLED_UI_MainLoop() 在主循环中调用

// 在 ESP32 上实现:

// 6a. 定时器 (替代 STM32 TIM1)
static void ui_timer_cb(void *arg) {
    OLED_UI_InterruptHandler();
}

void start_ui_timer(void) {
    const esp_timer_create_args_t timer_args = {
        .callback = ui_timer_cb,
        .name = "ui_timer"
    };
    esp_timer_handle_t timer;
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_periodic(timer, 20000);  // 20ms
}

// 6b. 主循环
void app_main(void) {
    nvs_flash_init();
    start_ui_timer();
    OLED_UI_Init(&MainMenuPage);

    while (1) {
        OLED_UI_MainLoop();
    }
}
```

### 步骤 7：CMakeLists.txt 更新

```cmake
idf_component_register(SRCS "main.c"
    "OLED_UI_Core/OLED_UI_Launcher.c"
    "OLED_UI_Core/OLED_UI/OLED_UI.c"
    "OLED_UI_Core/OLED_UI/OLED_UI_MenuData.c"
    "OLED_UI_Core/Driver/Hardware_Driver/OLED_UI_Driver.c"
    "OLED_UI_Core/Driver/Hardware_Driver/OLED_driver.c"   # 适配版
    "OLED_UI_Core/Driver/Software_Driver/OLED.c"           # 适配版
    "OLED_UI_Core/Driver/Software_Driver/OLED_Fonts.c"
    "OLED_UI_Core/Driver/Software_Driver/misc.c"
    INCLUDE_DIRS "." "OLED_UI_Core" "OLED_UI_Core/OLED_UI"
        "OLED_UI_Core/Driver/Hardware_Driver"
        "OLED_UI_Core/Driver/Software_Driver"
)
```

## 移植工作量评估

| 模块 | 难度 | 估算 |
|------|------|------|
| 复制代码结构 | ★ | 10 分钟 |
| `OLED_driver.c` 适配 | ★★★ | 2-4 小时 |
| `OLED_UI_Driver.c` 按键适配 | ★★ | 1 小时 |
| 定时器替换 (TIM→esp_timer) | ★ | 30 分钟 |
| 编码器→摇杆重映射 | ★★★ | 2 小时 |
| 菜单数据重构 | ★★★ | 2-3 小时 |
| 显示测试注册 | ★ | 30 分钟 |
| 总工作量 | | **8-11 小时** |

## 潜在风险

1. **内存**：OLED_UI_Core 使用 float/PID 运算，`ChangeFloatNum` 含 `fabs()`，Flash 占用约 15-20KB
2. **堆栈**：`PrintMenuElements` 等函数含深层循环，建议 `main` task 堆栈 4096+
3. **中文字体**：STM32 版含中文字库（12x12/16x16），Flash 占用较大（>50KB），可裁剪
4. **framebuf**：OLED_UI_Core 的 `OLED_DisplayBuf` 格式需要与 `ssd1306.c` 的 `framebuf` 对齐，或直接复用当前 framebuffer
5. **IDF 版本**：确保 `OLED_driver.c` 中的 I2C/SPI 操作使用 ESP-IDF API 而非 STM32 HAL

## 建议移植策略

```
阶段 1 (3h): OLED_driver 适配
  → 让 OLED_UI_Core 的绘图函数在 ESP32 上跑通
  → 验证: 调用 OLED_Clear → OLED_DrawCircle → OLED_Update 能正常显示

阶段 2 (2h): 按键/定时器适配
  → 实现 OLED_UI_InterruptHandler 和按键读取
  → 验证: 可以在菜单间导航

阶段 3 (3h): 菜单数据对接
  → 用 OLED_UI_Core 的格式重构当前菜单
  → 验证: 所有功能（画圆/按键/校准/关于）可正常进入

阶段 4 (2h): 整理与优化
  → 接入 Display Test 动画
  → 清理旧文件（ui.c/menu.c/about.c 等已不再需要）
  → 验证: 完整功能回归测试
```
