#ifndef MENU_H
#define MENU_H

void menu_draw(void);
int  menu_get_focus(void);
void menu_focus_up(void);
void menu_focus_down(void);
void menu_reset(void);
int  menu_is_dirty(void);

#endif
