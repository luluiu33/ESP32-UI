/* ============================================================
 *  about.h — 关于页面 (可滚动多行文本)
 *  不同显示模式 (OLED-only / LCD-only / 双屏) 显示不同内容。
 *  仅当 scroll_y 变化时重绘。
 * ============================================================ */
#ifndef ABOUT_H
#define ABOUT_H

#include <stdint.h>

void about_enter(void);             /* 进入关于页 (重置滚动位置) */
void about_scroll(int8_t dir);     /* 上/下滚动 (正=向下) */
void about_draw(void);              /* 绘制关于内容 */

#endif
