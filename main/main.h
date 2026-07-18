#pragma once

/* ===== 版本号 ===== */
#define FW_MAJOR 0
#define FW_MINOR 2
#define FW_PATCH 0
#define FW_BUILD 9

#define STR(x)  _STR(x)
#define _STR(x) #x

#define FW_VERSION_STR  "v" STR(FW_MAJOR) "." STR(FW_MINOR) "." STR(FW_PATCH) "." STR(FW_BUILD)

/* ===== 显示屏选择 =====
 * 通过注释/取消注释切换
 *
 *   仅 OLED:  OLED=y  LCD=n
 *   仅 LCD:   OLED=n  LCD=y
 *   双屏:     OLED=y  LCD=y
 */

#define CONFIG_DISPLAY_OLED
#define CONFIG_DISPLAY_LCD
