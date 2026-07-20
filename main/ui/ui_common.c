/* ============================================================
 *  ui_common.c — 共享绘图原语实现
 *  display_layout.h 中 CX / CY / GAP 由 CONFIG 编译决定。
 * ============================================================ */
#include "ui_common.h"
#include "display.h"
#include "display_layout.h"

/* 五方向像素坐标 (Up / Down / Left / Right / Center) */
const uint8_t pos[5][2] = {
    { CX,       CY - GAP },
    { CX,       CY + GAP },
    { CX - GAP, CY },
    { CX + GAP, CY },
    { CX,       CY },
};

/* 绘制三角形箭头（dir 0~3 对应 Up/Down/Left/Right）
 * fill=1 时用扫描线填充三角形内部 */
void draw_arrow(uint8_t cx, uint8_t cy, uint8_t dir, uint8_t fill)
{
    int s = ARROW_SZ, x[3], y[3];
    switch (dir) {
        case 0: x[0]=cx;   y[0]=cy-s; x[1]=cx-s; y[1]=cy+s; x[2]=cx+s; y[2]=cy+s; break;
        case 1: x[0]=cx;   y[0]=cy+s; x[1]=cx-s; y[1]=cy-s; x[2]=cx+s; y[2]=cy-s; break;
        case 2: x[0]=cx-s; y[0]=cy;   x[1]=cx+s; y[1]=cy-s; x[2]=cx+s; y[2]=cy+s; break;
        case 3: x[0]=cx+s; y[0]=cy;   x[1]=cx-s; y[1]=cy-s; x[2]=cx-s; y[2]=cy+s; break;
    }
    /* 绘制三条边 */
    for (int i = 0; i < 3; i++)
        display_draw_line(x[i], y[i], x[(i+1)%3], y[(i+1)%3]);

    if (fill) {
        /* 扫描线填充：从 mn+1 到 mx 每隔 2 行画水平线 */
        int mn = y[0], mx = y[0];
        for (int i = 1; i < 3; i++) {
            if (y[i] < mn) mn = y[i];
            if (y[i] > mx) mx = y[i];
        }
        int step = 2;
        for (int yy = mn + 1; yy < mx; yy += step) {
            int xl = 255, xr = 0;
            for (int i = 0; i < 3; i++) {
                int j = (i + 1) % 3;
                if ((y[i] <= yy && y[j] > yy) || (y[j] <= yy && y[i] > yy)) {
                    int xi = x[i] + (yy - y[i]) * (x[j] - x[i]) / (y[j] - y[i]);
                    if (xi < xl) xl = xi;
                    if (xi > xr) xr = xi;
                }
            }
            if (xl <= xr) display_draw_hline(xl, yy, xr - xl + 1);
        }
    }
}
