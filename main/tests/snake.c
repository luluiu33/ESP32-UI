/* ============================================================
 *  snake.c — 贪吃蛇游戏实现 (仅 LCD 模式)
 *
 *  循环队列存储蛇身。每次移动在 head+1 处添加新头，
 *  未吃食物时 tail+1 移除尾。撞墙/撞身 → GAME_OVER。
 *  方向键禁止 180° 调头以防止立即自杀。
 *  仅在 LCD 驱动加载时编译实际游戏代码，OLED 时显示
 *  "LCD only" 占位提示。
 * ============================================================ */
#include "snake.h"
#include "display.h"
#include "display_layout.h"
#include "main.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef CONFIG_DISPLAY_LCD
#include "lcd_st7735.h"
#endif

#define SNAKE_COLS      16      /* 网格列数 */
#define SNAKE_ROWS      18      /* 网格行数 */
#define CELL_SIZE       8       /* 每格像素 */
#define GRID_Y          16      /* 网格顶部 Y 偏移 */

#define SNAKE_MAX       (SNAKE_COLS * SNAKE_ROWS)  /* 最大长度 (满格) */
#define INIT_LEN        3       /* 初始长度 */
#define MOVE_MS         200     /* 步进间隔 (ms) */

/* ---- 方向 / 状态枚举 ---- */
typedef enum { SNAKE_UP, SNAKE_DOWN, SNAKE_LEFT, SNAKE_RIGHT } dir_t;
typedef enum { SNAKE_PLAYING, SNAKE_GAME_OVER } state_t;

/* 蛇身节点 */
typedef struct { uint8_t x, y; } pos_t;

/* ---- 游戏状态 ---- */
static pos_t    body[SNAKE_MAX];    /* 循环队列 (body[head] = 蛇头) */
static uint16_t head, tail;         /* 头 / 尾索引 */
static uint16_t snake_len;          /* 当前长度 */
static dir_t    dir, next_dir;      /* 当前方向 / 积压方向 */
static state_t  state;              /* 游戏状态 */
static uint32_t last_move;          /* 上次移动时间戳 */
static uint16_t score;              /* 得分 */

static pos_t food;                  /* 食物位置 */

/* 检查 (x, y) 是否在蛇身上（不含头） */
static int on_snake(uint8_t x, uint8_t y)
{
    for (uint16_t i = tail; i != head; i = (i + 1) % SNAKE_MAX)
        if (body[i].x == x && body[i].y == y) return 1;
    return 0;
}

/* 在空位上随机放置食物 */
static void place_food(void)
{
    do {
        food.x = rand() % SNAKE_COLS;
        food.y = rand() % SNAKE_ROWS;
    } while (on_snake(food.x, food.y));
}

void snake_init(void)
{
    head = INIT_LEN - 1;
    tail = 0;
    snake_len = INIT_LEN;
    for (uint8_t i = 0; i < INIT_LEN; i++) {
        body[i].x = i;
        body[i].y = SNAKE_ROWS / 2;
    }
    dir = SNAKE_RIGHT;
    next_dir = SNAKE_RIGHT;
    state = SNAKE_PLAYING;
    last_move = 0;
    score = 0;
    place_food();
}

/* 按积压方向前进一步 */
static void move_snake(void)
{
    pos_t n;
    n.x = body[head].x;
    n.y = body[head].y;

    switch (next_dir) {
        case SNAKE_UP:    n.y--; break;
        case SNAKE_DOWN:  n.y++; break;
        case SNAKE_LEFT:  n.x--; break;
        case SNAKE_RIGHT: n.x++; break;
    }

    /* 撞墙 / 撞身 */
    if (n.x >= SNAKE_COLS || n.y >= SNAKE_ROWS) { state = SNAKE_GAME_OVER; return; }
    if (on_snake(n.x, n.y))                      { state = SNAKE_GAME_OVER; return; }

    dir = next_dir;
    head = (head + 1) % SNAKE_MAX;
    body[head] = n;
    snake_len++;

    if (n.x == food.x && n.y == food.y) {
        score++;                /* 吃到食物 → 不弹出尾 */
        place_food();
    } else {
        tail = (tail + 1) % SNAKE_MAX;  /* 未吃到 → 移除尾 */
        snake_len--;
    }
}

void snake_draw(input_event_t ev, joy_state_t *js)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

#ifdef CONFIG_DISPLAY_LCD
    /* ---- Game Over 处理 ---- */
    if (state == SNAKE_GAME_OVER) {
        if (ev == EV_CONFIRM) {
            snake_init();
            display_clear();
        }
        return;
    }

    /* ---- 方向输入（禁止 180° 调头） ---- */
    if      (js->up    && dir != SNAKE_DOWN)  next_dir = SNAKE_UP;
    else if (js->down  && dir != SNAKE_UP)    next_dir = SNAKE_DOWN;
    else if (js->left  && dir != SNAKE_RIGHT) next_dir = SNAKE_LEFT;
    else if (js->right && dir != SNAKE_LEFT)  next_dir = SNAKE_RIGHT;

    /* ---- 定时移动 ---- */
    if (!last_move) last_move = now;
    if (now - last_move < MOVE_MS) return;
    last_move = now;

    move_snake();

    /* 移动后检查是否 Game Over */
    if (state == SNAKE_GAME_OVER) {
        display_clear();
        lcd_draw_string(20, 10 * 8, "Game Over!", COLOR_RED, COLOR_WHITE);
        char buf[24];
        snprintf(buf, sizeof(buf), "Score: %u", score);
        lcd_draw_string(36, 12 * 8, buf, COLOR_BLACK, COLOR_WHITE);
        lcd_update();
        return;
    }

    /* ---- 绘制 ---- */
    display_clear();
    lcd_fill_rect(food.x * CELL_SIZE, GRID_Y + food.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, COLOR_RED);
    for (uint16_t i = tail; i != head; i = (i + 1) % SNAKE_MAX)
        lcd_fill_rect(body[i].x * CELL_SIZE, GRID_Y + body[i].y * CELL_SIZE, CELL_SIZE, CELL_SIZE, COLOR_GREEN);
    lcd_fill_rect(body[head].x * CELL_SIZE, GRID_Y + body[head].y * CELL_SIZE, CELL_SIZE, CELL_SIZE, COLOR_GREEN);
    lcd_update();
#else
    (void)now;
    display_clear();
    display_draw_string(20, 5, "LCD only");
    display_update();
#endif
}
