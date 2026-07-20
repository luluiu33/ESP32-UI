/* ============================================================
 *  menu.h — 滚动菜单 (5 项, 3 项可见)
 *  摇杆 UP/DOWN 切换焦点，RIGHT 进入选中项。
 *  惰性重绘：focus 变化后才刷新。
 * ============================================================ */
#ifndef MENU_H
#define MENU_H

void menu_draw(void);               /* 全屏绘制菜单 */
int  menu_get_focus(void);          /* 获取当前焦点索引 */
void menu_focus_up(void);           /* 焦点上移 */
void menu_focus_down(void);         /* 焦点下移 */
void menu_reset(void);              /* 重置焦点到第 0 项 */
int  menu_is_dirty(void);           /* 检查焦点是否变化 (用于惰性刷新) */

#endif
