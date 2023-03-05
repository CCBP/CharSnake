#include "snake_interface.h"
#include <linux/slab.h>
#include <linux/random.h>

#define SNAKE_MAP_DATA_MAX (0x7F)       // 地图最大数据量
/** 
 * 地图原始数据，用于记录地图中各元素的位置
 * 蛇身会在蛇尾的基础上依次递增
 */
typedef enum {
    SNAKE_RAW_MAP = 0,                  // 地图
    SNAKE_RAW_TAIL,                     // 蛇尾
    SNAKE_RAW_FOOD = SNAKE_MAP_DATA_MAX,// 食物

    SNAKE_RAW_MAX = SNAKE_RAW_FOOD,
} map_raw_t;

/**
 * 地图绘制数据，根据原始数据用以下内容绘制各元素
 */
const char SNAKE_DRAW_HEAD = '@';        // 蛇头
const char SNAKE_DRAW_FAIL = 'X';        // 失败
const char SNAKE_DRAW_SUCC = 'O';        // 成功
const char SNAKE_DRAW_BODY = '#';        // 蛇身
const char SNAKE_DRAW_TAIL = '*';        // 蛇尾
const char SNAKE_DRAW_FOOD = '$';        // 食物
const char SNAKE_DRAW_MAP  = '.';        // 地图
const char SNAKE_DRAW_LF   = '\n';       // 换行符

struct _snake_t {
    state_t state;                       // 游戏状态
    dir_t move_dir;                      // 移动方向
    int length;                          // 蛇身长度
    // 坐标以地图左上角为原点
    int head_x;                          // 蛇头X轴坐标
    int head_y;                          // 蛇头y轴坐标
    size_t map_size;                     // 地图大小
    char *map_raw;                       // 地图原始数据
    char *map_draw;                      // 地图显示数据
};

/**
 * get_map_raw - 获取目标位置的地图数据
 * @snake: 需要获取的snake_t类型指针
 * @x: 需要获取的x轴坐标
 * @y: 需要获取的y轴坐标
 *
 * Return: 目标位置的地图数据
 */
static char get_map_raw(snake_t *snake, int x, int y)
{
    y *= snake->map_size;
    return *(snake->map_raw + y + x);
}

/**
 * set_map_raw - 修改地图数据
 * @snake: 需要修改地图的snake_t类型指针
 * @x: 需要修改位置的x轴坐标
 * @y: 需要修改位置的y轴坐标
 * @data: 需要修改的数据
 */
static void set_map_raw(snake_t *snake, int x, int y, char data)
{
    y *= snake->map_size;
    *(snake->map_raw + y + x) = data;
}

/**
 * is_beyond_border - 判断目标位置是否超出地图
 * @snake: 需要判断是否超出地图的snake_t类型指针
 * @x: 目标位置的x轴坐标
 * @y: 目标位置的y轴坐标
 * 
 * Return: true 超出地图 false 未超出地图
 */
static bool is_beyond_border(snake_t *snake, int x, int y)
{
    bool result = true;
    if ((x >= 0) && (y >= 0) &&
        (x < snake->map_size) &&
        (y < snake->map_size) ) {
        result = false;
    }
    return result;
}

/**
 * snake_refresh - 遍历蛇身，实现蛇的移动
 * @snake: 需要遍历蛇身的snake_t类型指针
 * @x: 蛇头的x轴坐标
 * @y: 蛇头的y轴坐标
 * @last_data: 蛇的长度
 *
 * 此函数为递归调用，递归遍历当前坐标的上、下、左、右四个方向，
 * 判断蛇身是否需要移动，直到遇到地图边界或者无蛇身
 */
static void snake_refresh(snake_t *snake, int x, int y, char last_data)
{
    char data;
    if (is_beyond_border(snake, x, y)) {
        return; // 超出地图
    }
    data = get_map_raw(snake, x, y);
    if ((data == SNAKE_RAW_MAP ) ||
        (data == SNAKE_RAW_FOOD) ) {
        return; // 非蛇身
    }
    if (data - 1 == last_data) {        // 蛇身移动
        data -= 1;
        set_map_raw(snake, x, y, data);
    }
    if (data == last_data) {    // 确保正向遍历
        snake_refresh(snake, x, y - 1, data - 1);
        snake_refresh(snake, x, y + 1, data - 1);
        snake_refresh(snake, x - 1, y, data - 1);
        snake_refresh(snake, x + 1, y, data - 1);
    }
}

/**
 * generate_food - 通过随机数生成食物
 * @snake: 需要生成食物的snake_t类型指针
 */
static void generate_food(snake_t *snake)
{
    unsigned char random_x;
    unsigned char random_y;
    char data = SNAKE_RAW_MAX;
    do {
        get_random_bytes(&random_x, sizeof(random_x));
        get_random_bytes(&random_y, sizeof(random_y));
        if (!is_beyond_border(snake, random_x, random_y)) {
            data = get_map_raw(snake, random_x, random_y);
        }
        printk(KERN_ALERT "x: %d  y: %d\n", random_x, random_y);
    } while (SNAKE_RAW_MAX == data);
    set_map_raw(snake, random_x, random_y, SNAKE_RAW_FOOD);
}

/**
 * snake_is_success - 判断游戏是否成功
 * @snake: 需要判断是否成功的snake_t类型指针
 *
 * Return: true 游戏结束 false 游戏继续
 */
bool snake_is_success(snake_t *snake)
{
    bool result = false;
    int map_size = snake_get_map_size(snake);
    if ((snake->length >= SNAKE_RAW_MAX) ||
        (  snake->length >= map_size   ) ) {
        result = true;
    }
    return result;
}

/**
 * snake_map_refresh - 根据设定的移动方向刷新地图
 * @snake: 需要刷新地图的snake_t类型指针
 */
void snake_map_refresh(snake_t *snake)
{
    int new_head_x = snake->head_x;
    int new_head_y = snake->head_y;

    if (snake_is_success(snake)) {
        return;     // 游戏成功，禁止移动
    }

    switch (snake->move_dir) {
        case DIR_UP:    // 上移
            new_head_y -= 1;
            break;
        case DIR_DOWN:  // 下移
            new_head_y += 1;
            break;
        case DIR_LEFT:  // 左移
            new_head_x -= 1;
            break;
        case DIR_RIGHT: // 右移
            new_head_x += 1;
            break;
        default:        // 游戏暂停
            snake->state = STATE_PAUSE;
            break;
    }

    if (!is_beyond_border(snake, new_head_x, new_head_y)) {
        char data = get_map_raw(snake, new_head_x, new_head_y);
        // 前进方向为空地或食物时继续移动
        // 未移动或后退时不响应动作
        if ((SNAKE_RAW_MAP  == data) ||
            (SNAKE_RAW_FOOD == data)) {
            snake->state = STATE_RUNNING;
            // 更新蛇头位置
            snake->head_x = new_head_x;
            snake->head_y = new_head_y;
            if (SNAKE_RAW_FOOD == data) {
                snake->length += 1;              // 吃到食物，蛇身变长
                if (!snake_is_success(snake)) {  // 若游戏未结束则生成新食物
                    generate_food(snake);
                } else {
                    snake->state = STATE_SUCCESS;
                }
            }
            // 移动蛇头，并以此为基础更新蛇身位置
            set_map_raw(snake, new_head_x, new_head_y, snake->length);
            snake_refresh(snake, new_head_x, new_head_y, snake->length);
        } else if ((  snake->length != data  ) && // 未移动 
                   (snake->length - 1 != data)) { // 后退
            snake->state = STATE_FAILED;
        }
    } else {
        snake->state = STATE_FAILED;
    }
}

/**
 * snake_get_map_size - 获取地图尺寸（占用的内存空间）
 * @snake: 需要获取地图尺寸的snake_t类型指针
 *
 * Return: 地图尺寸（占用的内存空间）
 */
size_t snake_get_map_size(snake_t *snake)
{
    return snake->map_size * snake->map_size + snake->map_size;
}

/**
 * snake_init - snake的构造函数
 * @snake: 需要初始化snake_t类型指针
 * @map_size: 初始化地图大小，其值应为正方形地图的边长，若为偶数则自动加1使地图中心对称
 *
 * Return: 0为初始化成功，非0为初始化失败
 */
int snake_init(snake_t **snake, size_t map_size)
{
    int result = 0;

    if ((             map_size <= 0              ) ||
        (map_size * map_size > SNAKE_MAP_DATA_MAX)) {
        *snake = NULL;
        result = -EINVAL;
        goto fail;
    }

    // 确保地图中心对称
    if (map_size % 2 == 0) {
        map_size += 1;
    }

    *snake = kmalloc(sizeof(snake_t), GFP_KERNEL);
    if (NULL == *snake) {
        result = -ENOMEM;
        goto fail;
    }

    (*snake)->map_size = map_size;
    // 申请储存原始地图数据所需的内存空间
    (*snake)->map_raw = kmalloc(map_size * map_size, GFP_KERNEL);
    if (NULL == (*snake)->map_raw) {
        kfree(*snake);
        *snake = NULL;
        result = -ENOMEM;
        goto fail;
    }
    memset((*snake)->map_raw, SNAKE_RAW_MAP, map_size * map_size);
    // 申请绘制地图所需的内存空间，需留出换行符所需空间
    (*snake)->map_draw = kmalloc(snake_get_map_size(*snake), GFP_KERNEL);
    if (NULL == (*snake)->map_draw) {
        kfree((*snake)->map_raw);
        kfree(*snake);
        snake = NULL;
        result = -ENOMEM;
        goto fail;
    }
    memset((*snake)->map_draw, SNAKE_DRAW_MAP, snake_get_map_size(*snake));
    result = map_size;      // 构造成功

    (*snake)->move_dir = DIR_PAUSE;
    // 蛇头初始化在地图中心
    (*snake)->head_x = (*snake)->map_size / 2;
    (*snake)->head_y = (*snake)->map_size / 2;
    (*snake)->length = 1;       // 初始长度为1
    snake_map_refresh(*snake);// 生成蛇头
    generate_food(*snake);    // 生成食物

fail:
    return result;
}

/**
 * snake_deinit - snake的析构函数
 * @snake: 需要析构的snake_t类型指针
 */
void snake_deinit(snake_t **snake)
{
    kfree((*snake)->map_raw);
    kfree((*snake)->map_draw);
    kfree(*snake);
    *snake = NULL;
}

/**
 * snake_draw_map - 绘制由原始数据转换而来的地图
 * @snake: 需要绘制地图的snake_t类型指针
 * 
 * Return: 指向地图数据的指针
 */
char *snake_draw_map(snake_t *snake)
{
    for (int i = 0, j = 0; j < snake_get_map_size(snake); i++, j++) {
        char data_raw = *(snake->map_raw + i);
        char data_draw;
        switch (data_raw) {
            case SNAKE_RAW_TAIL:
                data_draw = SNAKE_DRAW_TAIL;
                break;
            case SNAKE_RAW_FOOD:
                data_draw = SNAKE_DRAW_FOOD;
                break;
            case SNAKE_RAW_MAP:
                data_draw = SNAKE_DRAW_MAP;
                break;
            default:
                data_draw = SNAKE_DRAW_BODY;
                break;
        }
        if (data_raw == snake->length) {    // 单独绘制蛇头
            switch (snake->state) {
                case STATE_SUCCESS:
                    data_draw = SNAKE_DRAW_SUCC;
                    break;
                case STATE_FAILED:
                    data_draw = SNAKE_DRAW_FAIL;
                    break;
                default:
                    data_draw = SNAKE_DRAW_HEAD;
                    break;
            }
        }
        *(snake->map_draw + j) = data_draw;
        if ((i + 1) % snake->map_size == 0) { // 绘制换行符
            j += 1;
            *(snake->map_draw + j) = SNAKE_DRAW_LF;
        }
    }
    return snake->map_draw;
}

/**
 * snake_set_dir - 设置snake的移动方向
 * @snake: 需要设置移动方向的snake_t类型指针
 * @dir: 移动方向
 */
void snake_set_dir(snake_t *snake, dir_t dir)
{
    snake->move_dir = dir;
}
