/* ============================================================
 *  display_test.c — 6 种动画演示实现
 *
 *  每种动画内部维护自己的状态变量，通过 disp_test_enter()
 *  初始化。switch-case 调度到对应绘制函数。
 *  进度条和弹跳球每帧更新一次物理量。
 * ============================================================ */
#include "display_test.h"
#include "display.h"
#include "display_layout.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>

#define Y_MIN TEST_Y_MIN
#define Y_MAX TEST_Y_MAX

static int active_test = -1;            /* 当前动画索引，-1 表示未进入 */
static const char *test_names[DISP_TEST_COUNT] = {
    "Sine Wave", "Bounce Ball", "Progress", "Shapes", "Charset", "Marquee"
};

/* ================================================================
 * 1. 正弦波 — 滚动波形
 * ================================================================ */
#define SINE_AMP  18.0f
#define SINE_CY   36.0f
#define SINE_FREQ (3.14159265f * 2.0f / 32.0f)

static float sine_phase = 0.0f;

static void draw_sine(void)
{
    display_clear();
    int py = (int)(SINE_CY + SINE_AMP * sinf(sine_phase));
    for (int x = 1; x < 128; x++) {
        float rad = sine_phase + x * SINE_FREQ;
        int y = (int)(SINE_CY + SINE_AMP * sinf(rad));
        display_draw_line(x - 1, py, x, y);
        py = y;
    }
    display_update();
    sine_phase += SINE_FREQ;
    if (sine_phase > 3.14159265f * 2.0f) sine_phase -= 3.14159265f * 2.0f;
}

/* ================================================================
 * 2. 弹跳球 — 重力反弹动画
 * ================================================================ */
static int ball_x, ball_y;
static int ball_vx, ball_vy;
static int ball_prev_x, ball_prev_y;

static void ball_init(void)
{
    ball_x = 64; ball_y = 40;
    ball_vx = 3; ball_vy = 2;
    ball_prev_x = -1; ball_prev_y = -1;
}

static void draw_ball(void)
{
    if (ball_x == ball_prev_x && ball_y == ball_prev_y) return;
    ball_prev_x = ball_x;
    ball_prev_y = ball_y;

    display_clear();
    display_draw_circle(ball_x, ball_y, 3, 1);
    display_update();
}

static void ball_update(void)
{
    ball_x += ball_vx;
    ball_y += ball_vy;
    if (ball_x <= 3 || ball_x >= 124) { ball_vx = -ball_vx; ball_x += ball_vx; }
    if (ball_y <= 3 + Y_MIN || ball_y >= 60)  { ball_vy = -ball_vy; ball_y += ball_vy; }
}

/* ================================================================
 * 3. 进度条 — 0→100% 往返填充
 * ================================================================ */
static int pb_value = 0;
static int pb_dir = 1;

static void draw_progress(void)
{
    display_clear();
    display_draw_rect(4, 28, 120, 14, 0);
    int fw = (pb_value * 116) / 100;
    if (fw > 0) display_draw_rect(6, 30, fw, 10, 1);
    display_printf(64 - 4 * 6, TEST_TITLE_Y + 6, "%d%%", pb_value);
    display_update();
}

static void progress_update(void)
{
    pb_value += pb_dir;
    if (pb_value >= 100) { pb_value = 100; pb_dir = -1; }
    if (pb_value <= 0)   { pb_value = 0;   pb_dir =  1; }
}

/* ================================================================
 * 4. 几何图形 — 静态构图展示
 * ================================================================ */
static void draw_shapes(void)
{
    display_clear();
    display_draw_rect(2, 18, 40, 20, 0);
    display_draw_rect(50, 18, 30, 20, 1);
    display_draw_circle(100, 28, 10, 0);
    display_draw_circle(100, 28, 4, 1);
    display_draw_line(2, 46, 50, 62);
    display_draw_line(50, 46, 2, 62);
    display_draw_rect(70, 46, 56, 16, 1);
    display_draw_rect(74, 48, 48, 12, 0);
    display_update();
}

/* ================================================================
 * 5. 字符集 — 显示 ASCII 32~126
 * ================================================================ */
static void draw_charset(void)
{
    display_clear();
    display_draw_string(10, TEST_TITLE_Y, "ASCII 32~126");
    int idx = 32;
    int max_row = CONTENT_Y + CONTENT_H;
    for (int row = TEST_TITLE_Y + 1; row < TEST_TITLE_Y + 6 && row < max_row; row++) {
        for (int col = 0; col < 21; col++) {
            if (idx > 126) break;
            display_draw_char(col * 6, row, (char)idx);
            idx++;
        }
        if (idx > 126) break;
    }
    display_update();
}

/* ================================================================
 * 6. 跑马灯 — 四角爬行亮线 (36px 尾迹)
 * ================================================================ */
static int mx, my, mdx, mdy;

static void marquee_step(void)
{
    if (mx == 126 && my == Y_MIN && mdx == 1)  { mx = 127; mdx = 0;  mdy = 1; }
    else if (mx == 127 && my == Y_MAX && mdy == 1) { mx = 127; mdx = -1; mdy = 0; }
    else if (mx == 1  && my == Y_MAX && mdx == -1)  { mx = 0;   mdx = 0;  mdy = -1; }
    else if (mx == 0  && my == Y_MIN && mdy == -1)  { mx = 0;   mdx = 1;  mdy = 0; }
    else { mx += mdx; my += mdy; }
}

static void draw_marquee(void)
{
    display_clear();
    for (int i = 0; i < 36; i++) {
        int px = mx - mdx * i;
        int py = my - mdy * i;
        if (px >= 0 && px < 128 && py >= Y_MIN && py <= Y_MAX) {
            display_draw_pixel(px, py);
            if (mdx) { display_draw_pixel(px, py - 1); display_draw_pixel(px, py + 1); }
            if (mdy) { display_draw_pixel(px - 1, py); display_draw_pixel(px + 1, py); }
        }
    }
    display_update();
}

/* ================================================================
 * 公共 API
 * ================================================================ */
void disp_test_enter(int index)
{
    active_test = index;
    if (index >= 0 && index < DISP_TEST_COUNT)
        display_set_title(test_names[index]);
    switch (index) {
    case 0: sine_phase = 0.0f; break;
    case 1: ball_init(); break;
    case 2: pb_value = 0; pb_dir = 1; break;
    case 5: mx = 0; my = Y_MIN; mdx = 1; mdy = 0; break;
    default: break;
    }
}

void disp_test_next(void)
{
    disp_test_enter((active_test + 1) % DISP_TEST_COUNT);
}

void disp_test_prev(void)
{
    disp_test_enter((active_test - 1 + DISP_TEST_COUNT) % DISP_TEST_COUNT);
}

void disp_test_draw(joy_state_t *js)
{
    (void)js;
    if (active_test < 0) return;
    switch (active_test) {
    case 0: draw_sine(); break;
    case 1: ball_update(); draw_ball(); break;
    case 2: progress_update(); draw_progress(); break;
    case 3: draw_shapes(); break;
    case 4: draw_charset(); break;
    case 5: marquee_step(); draw_marquee(); break;
    }
}
