#ifndef __SNAKE_INTERFACE_H__
#define __SNAKE_INTERFACE_H__

#include <linux/types.h>

/**
 * 用于表示蛇的运动方向
 */
typedef enum {
    DIR_UP = 0,         // 向上移动
    DIR_DOWN,           // 向下移动
    DIR_LEFT,           // 向左移动
    DIR_RIGHT,          // 向右移动
    DIR_PAUSE           // 暂停移动
} dir_t;

/**
 * 用于表示游戏的运行状态
 */
typedef enum {
    STATE_RUNNING = 0,  // 运行中
    STATE_SUCCESS,      // 游戏成功
    STATE_FAILED,       // 游戏失败
    STATE_PAUSE         // 游戏暂停
} state_t;

typedef struct _snake_t snake_t;

bool snake_is_success(snake_t *snake);
void snake_map_refresh(snake_t *snake);
int  snake_init(snake_t *snake, size_t map_size);
void snake_deinit(snake_t *snake);

size_t snake_get_map_size(snake_t *snake);
char *snake_draw_map(snake_t *snake);
void snake_set_dir(snake_t*snake, dir_t dir);

#endif
