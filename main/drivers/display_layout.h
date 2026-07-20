/* ============================================================
 *  display_layout.h — OLED/LCD 布局宏
 *
 *  所有 UI/测试代码通过此头文件中的宏访问页面索引和像素坐标，
 *  无需关心实际分辨率。编译时根据 CONFIG_DISPLAY_OLED/LCD
 *  自动选择对应的 OLED_* / LCD_* 常量。
 *
 *  双屏模式下，无前缀别名指向 OLED（较紧约束），
 *  LCD 独占代码可直接使用 LCD_* 前缀常量。
 * ============================================================ */
#ifndef DISPLAY_LAYOUT_H
#define DISPLAY_LAYOUT_H

#include "main.h"

/* ═══════════════════════════════════════════════════════════════
 * OLED (SSD1306 128×64)   — 8 pages × 8px = 64px
 *   pages 0-1: 标题栏 (16px)
 *   pages 2-7: 内容区   (48px, 6 行文本)
 * ═══════════════════════════════════════════════════════════════ */
#ifdef CONFIG_DISPLAY_OLED

#define OLED_CONTENT_Y      2               /* 内容起始页 (紧接标题栏) */
#define OLED_CONTENT_H      6               /* 内容区页面数 (48px) */

/* ---- 文本行坐标 (page-based) ---- */
#define OLED_MENU_TITLE_Y   (OLED_CONTENT_Y + 0)
#define OLED_MENU_VISIBLE   3               /* 菜单可见项数 */
#define OLED_MENU_ITEM_Y(i) (OLED_CONTENT_Y + 1 + (i) * 2)   /* 行 3,5,7 */

#define OLED_ABOUT_TEXT_Y(i)(OLED_CONTENT_Y + (i))            /* 行 2..7 */

#define OLED_CAL_TITLE_Y    (OLED_CONTENT_Y + 0)              /* 行 2 */
#define OLED_CAL_STEP_Y     (OLED_CONTENT_Y + 1)              /* 行 3 */
#define OLED_CAL_LABEL_Y    (OLED_CONTENT_Y + 4)              /* 行 6 */

#define OLED_TEST_TITLE_Y   (OLED_CONTENT_Y + 0)              /* 行 2 */

/* ---- 像素坐标 ---- */
#define OLED_CX             64
#define OLED_CY             (OLED_CONTENT_Y * 8 + 27)         /* = 43 */
#define OLED_GAP            16
#define OLED_CIRCLE_R       5

#define OLED_CIRCLE_CX      64
#define OLED_CIRCLE_CY      (OLED_CONTENT_Y * 8 + 27)         /* = 43 */
#define OLED_CIRCLE_RAD     18

#define OLED_TEST_Y_MIN     (OLED_CONTENT_Y * 8 + 8)          /* 标题栏下 8px */
#define OLED_TEST_Y_MAX     (OLED_CONTENT_Y * 8 + OLED_CONTENT_H * 8 - 1) /* 63 */

#define OLED_CAL_ARROW_CX   64
#define OLED_CAL_ARROW_CY   (OLED_CONTENT_Y * 8 + 27)         /* = 43 */

#endif /* CONFIG_DISPLAY_OLED */

/* ═══════════════════════════════════════════════════════════════
 * LCD (ST7735 128×160)    — 20 pages × 8px = 160px
 *   pages 0-1:  标题栏 (16px)
 *   pages 2-19: 内容区  (144px, 18 行文本)
 * ═══════════════════════════════════════════════════════════════ */
#if defined(CONFIG_DISPLAY_LCD)

#define LCD_CONTENT_Y       2               /* 内容起始页 */
#define LCD_CONTENT_H       18              /* 内容区页面数 (144px) */

#define LCD_MENU_TITLE_Y    (LCD_CONTENT_Y + 0)
#define LCD_MENU_VISIBLE    10
#define LCD_MENU_ITEM_Y(i)  (LCD_CONTENT_Y + 4 + (i) * 2)   /* 行 6,8,10... */

#define LCD_ABOUT_TEXT_Y(i) (LCD_CONTENT_Y + (i))            /* 行 2..19 */

#define LCD_CAL_TITLE_Y     (LCD_CONTENT_Y + 0)
#define LCD_CAL_STEP_Y      (LCD_CONTENT_Y + 1)
#define LCD_CAL_LABEL_Y     (LCD_CONTENT_Y + 7)

#define LCD_TEST_TITLE_Y    (LCD_CONTENT_Y + 0)

/* ---- 像素坐标 ---- */
#define LCD_CX              64
#define LCD_CY              (LCD_CONTENT_Y * 8 + 84)         /* = 100 */
#define LCD_GAP             20
#define LCD_CIRCLE_R        8

#define LCD_CIRCLE_CX       64
#define LCD_CIRCLE_CY       (LCD_CONTENT_Y * 8 + 84)         /* = 100 */
#define LCD_CIRCLE_RAD      50

#define LCD_TEST_Y_MIN      (LCD_CONTENT_Y * 8 + 16)         /* 标题栏下 16px */
#define LCD_TEST_Y_MAX      (LCD_CONTENT_Y * 8 + 136)        /* = 152 */

#define LCD_CAL_ARROW_CX    64
#define LCD_CAL_ARROW_CY    (LCD_CONTENT_Y * 8 + 84)

#endif /* CONFIG_DISPLAY_LCD */

/* ═══════════════════════════════════════════════════════════════
 *  无前缀别名
 *
 *  单屏模式 → 直接映射到对应显示器的常量
 *  双屏模式 → 映射到 OLED（较紧约束），LCD 独占代码用 LCD_* 前缀
 * ═══════════════════════════════════════════════════════════════ */
#if !defined(CONFIG_DISPLAY_OLED) && !defined(CONFIG_DISPLAY_LCD)
  #error "请至少启用一种显示器：在 main.h 中定义 CONFIG_DISPLAY_OLED 或 CONFIG_DISPLAY_LCD"

#elif defined(CONFIG_DISPLAY_OLED) && defined(CONFIG_DISPLAY_LCD)
  /* 双屏模式 — 无前缀别名指向 OLED (较紧约束) */
  #define CONTENT_Y       OLED_CONTENT_Y
  #define CONTENT_H       OLED_CONTENT_H
  #define MENU_TITLE_Y    OLED_MENU_TITLE_Y
  #define MENU_VISIBLE    OLED_MENU_VISIBLE
  #define MENU_ITEM_Y(i)  OLED_MENU_ITEM_Y(i)
  #define ABOUT_TEXT_Y(i) OLED_ABOUT_TEXT_Y(i)
  #define CAL_TITLE_Y     OLED_CAL_TITLE_Y
  #define CAL_STEP_Y      OLED_CAL_STEP_Y
  #define CAL_LABEL_Y     OLED_CAL_LABEL_Y
  #define TEST_TITLE_Y    OLED_TEST_TITLE_Y
  #define CX              OLED_CX
  #define CY              OLED_CY
  #define GAP             OLED_GAP
  #define CIRCLE_R        OLED_CIRCLE_R
  #define CIRCLE_CX       OLED_CIRCLE_CX
  #define CIRCLE_CY       OLED_CIRCLE_CY
  #define CIRCLE_RAD      OLED_CIRCLE_RAD
  #define TEST_Y_MIN      OLED_TEST_Y_MIN
  #define TEST_Y_MAX      OLED_TEST_Y_MAX
  #define CAL_ARROW_CX    OLED_CAL_ARROW_CX
  #define CAL_ARROW_CY    OLED_CAL_ARROW_CY

#elif defined(CONFIG_DISPLAY_OLED)
  #define CONTENT_Y       OLED_CONTENT_Y
  #define CONTENT_H       OLED_CONTENT_H
  #define MENU_TITLE_Y    OLED_MENU_TITLE_Y
  #define MENU_VISIBLE    OLED_MENU_VISIBLE
  #define MENU_ITEM_Y(i)  OLED_MENU_ITEM_Y(i)
  #define ABOUT_TEXT_Y(i) OLED_ABOUT_TEXT_Y(i)
  #define CAL_TITLE_Y     OLED_CAL_TITLE_Y
  #define CAL_STEP_Y      OLED_CAL_STEP_Y
  #define CAL_LABEL_Y     OLED_CAL_LABEL_Y
  #define TEST_TITLE_Y    OLED_TEST_TITLE_Y
  #define CX              OLED_CX
  #define CY              OLED_CY
  #define GAP             OLED_GAP
  #define CIRCLE_R        OLED_CIRCLE_R
  #define CIRCLE_CX       OLED_CIRCLE_CX
  #define CIRCLE_CY       OLED_CIRCLE_CY
  #define CIRCLE_RAD      OLED_CIRCLE_RAD
  #define TEST_Y_MIN      OLED_TEST_Y_MIN
  #define TEST_Y_MAX      OLED_TEST_Y_MAX
  #define CAL_ARROW_CX    OLED_CAL_ARROW_CX
  #define CAL_ARROW_CY    OLED_CAL_ARROW_CY

#elif defined(CONFIG_DISPLAY_LCD)
  #define CONTENT_Y       LCD_CONTENT_Y
  #define CONTENT_H       LCD_CONTENT_H
  #define MENU_TITLE_Y    LCD_MENU_TITLE_Y
  #define MENU_VISIBLE    LCD_MENU_VISIBLE
  #define MENU_ITEM_Y(i)  LCD_MENU_ITEM_Y(i)
  #define ABOUT_TEXT_Y(i) LCD_ABOUT_TEXT_Y(i)
  #define CAL_TITLE_Y     LCD_CAL_TITLE_Y
  #define CAL_STEP_Y      LCD_CAL_STEP_Y
  #define CAL_LABEL_Y     LCD_CAL_LABEL_Y
  #define TEST_TITLE_Y    LCD_TEST_TITLE_Y
  #define CX              LCD_CX
  #define CY              LCD_CY
  #define GAP             LCD_GAP
  #define CIRCLE_R        LCD_CIRCLE_R
  #define CIRCLE_CX       LCD_CIRCLE_CX
  #define CIRCLE_CY       LCD_CIRCLE_CY
  #define CIRCLE_RAD      LCD_CIRCLE_RAD
  #define TEST_Y_MIN      LCD_TEST_Y_MIN
  #define TEST_Y_MAX      LCD_TEST_Y_MAX
  #define CAL_ARROW_CX    LCD_CAL_ARROW_CX
  #define CAL_ARROW_CY    LCD_CAL_ARROW_CY
#endif

#endif /* DISPLAY_LAYOUT_H */
