#pragma once

/* ============================================================
 *  项目全局配置：版本号 + 显示屏选择
 * ============================================================ */

/* ===== 版本号 (主版本.次版本.补丁.构建) ===== */
#define FW_MAJOR 0
#define FW_MINOR 3
#define FW_PATCH 1
#define FW_BUILD 11

/* 字符串化辅助宏 */
#define STR(x)  _STR(x)
#define _STR(x) #x

/* 版本号字符串，如 "v0.2.3.3" */
#define FW_VERSION_STR  "v" STR(FW_MAJOR) "." STR(FW_MINOR) "." STR(FW_PATCH) "." STR(FW_BUILD)

/* ===== 显示屏选择 =====
 * 通过注释/取消注释切换：
 *   #define CONFIG_DISPLAY_OLED  启用 SSD1306 OLED (128×64)
 *   #define CONFIG_DISPLAY_LCD   启用 ST7735 LCD  (128×160)
 * 两者同时启用 = 双屏模式
 */
// #define CONFIG_DISPLAY_OLED
#define CONFIG_DISPLAY_LCD
