#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include "snake_interface.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("CCBP");
MODULE_DESCRIPTION("Character Snake Game");

typedef struct _snake_dev {
    dev_t devid;
    struct cdev cdev;
    snake_t *snake;
} snake_dev_t;
static snake_dev_t *snake_dev;

const int   MAP_SIZE = 11;              // 地图边长
const char *DEV_NAME = "char_snake";    // 设备名称

static ssize_t snake_read(struct file *fp, char __user *buffer, size_t size, loff_t *pos)
{
    snake_dev_t *sdev = fp->private_data;
    char *map_data = NULL;
    size_t map_size;
    int result = 0;

    if (NULL != sdev->snake) {
        map_data = snake_draw_map(sdev->snake);   // 获取地图
        if (NULL == map_data) {
            printk(KERN_ALERT "[%s] can not draw a map!", DEV_NAME);
            result = -EFAULT;
            goto fail;
        }
        map_size = snake_get_map_size(sdev->snake);
        result = (map_size - *pos) > size ? size : (map_size - *pos);
        if (copy_to_user(buffer, (map_data + *pos), result)) {
            result = -EFAULT;
        } else {
            *pos += result;
        }
    }

fail:
    printk(KERN_ALERT "[%s] draw a map, size: %ld  pos: %lld  result: %d\n", 
                        DEV_NAME, size, *pos, result);
    return result;
}

static ssize_t snake_write(struct file *fp, const char __user *buffer, size_t size, loff_t *pos)
{
    snake_dev_t *sdev = fp->private_data;
    int result = 0;
    char dir;

    // 只取写入数据的最后一位作为控制蛇移动的方向
    if (copy_from_user(&dir, (buffer + size - 1), 1)) {
        result = -EFAULT;
        goto fail;
    }

    switch (dir) {
        case 'U':   // 向上
            dir = DIR_UP;
            break;
        case 'D':   // 向下
            dir = DIR_DOWN;
            break;
        case 'L':   // 向左
            dir = DIR_LEFT;
            break;
        case 'R':   // 向右
            dir = DIR_RIGHT;
            break;
        case 'P':   // 暂停
            dir = DIR_PAUSE;
            break;
        default:
            result = -EINVAL;
            goto fail;
            break;
    }
    snake_set_dir(sdev->snake, dir);
    result = dir;

fail:
    printk(KERN_ALERT "[%s] move snake, size: %ld  pos: %lld  result: %d\n", 
                        DEV_NAME, size, *pos, result);
    return result;
}

static int snake_open(struct inode *inode, struct file *fp)
{
    snake_dev_t *sdev = container_of(inode->i_cdev, snake_dev_t, cdev);
    
    fp->private_data = sdev;

    return 0;
}

static int snake_release(struct inode *inode, struct file *fp)
{
    printk(KERN_ALERT "[%s] stoped!\n", DEV_NAME);

    return 0;
}

static loff_t snake_seek(struct file *fp, loff_t offset, int whence)
{
    snake_dev_t *sdev = fp->private_data;
    loff_t pos;

    switch (whence) {
    case 0: // SEEK_SET
        pos = offset;
        break;
    case 1: // SEEK_CUR
        pos += fp->f_pos;
        break;
    case 2: // SEEK_END
        pos += snake_get_map_size(sdev->snake);
        break;
    default:
        pos = -EINVAL;
        goto fail;
    }
    if (pos < 0) {
        pos = -EINVAL;
        goto fail;
    }
    fp->f_pos = pos;
    printk(KERN_ALERT "[%s] seek, offset: %lld  whence: %d  pos: %lld\n", 
        DEV_NAME, offset, whence, pos);
fail:
    return pos;
}

struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = snake_open,
    .release = snake_release,
    .read    = snake_read,
    .write   = snake_write,
    .llseek  = snake_seek,
};

static int __init snake_dev_init(void)
{
    int result = 0;

    snake_dev = kmalloc(sizeof(snake_dev_t), GFP_KERNEL);
    if (NULL == snake_dev) {
        printk(KERN_ALERT "[%s] snake device init failed!\n", DEV_NAME);
        result = -ENOMEM;
        goto fail;
    }
    
    result = alloc_chrdev_region(&snake_dev->devid, 0, 1, DEV_NAME);

    if (result < 0) {
        printk(KERN_ALERT "[%s] register dev error! error code: %d\n", DEV_NAME, result);
        return result;
    }

    result = snake_init(&snake_dev->snake, MAP_SIZE);
    if (NULL == snake_dev->snake) {
        printk(KERN_ALERT "[%s] snake init failed! result: %d\n", DEV_NAME, result);
        result = -ENOMEM;
        goto fail;
    }
    
    cdev_init(&snake_dev->cdev, &fops);
    snake_dev->cdev.owner = THIS_MODULE;
    snake_dev->cdev.ops   = &fops;
    result = cdev_add(&snake_dev->cdev, snake_dev->devid, 1);
    if (result < 0) {
        printk(KERN_ALERT "[%s] cdev add fail! error code: %d\n", DEV_NAME, result);
        goto fail;
    }

    printk(KERN_ALERT "[%s] init success! major: %d\n", DEV_NAME, MAJOR(snake_dev->devid));
    return result;

fail:
    unregister_chrdev_region(snake_dev->devid, 1);
    printk(KERN_ALERT "[%s] init failed!\n", DEV_NAME);
    return result;
}

static void __exit snake_dev_exit(void)
{
    cdev_del(&snake_dev->cdev);
    snake_deinit(&snake_dev->snake);
    unregister_chrdev_region(snake_dev->devid, 1);
    kfree(snake_dev);
    snake_dev = NULL;

    printk(KERN_ALERT "[%s] exit!\n", DEV_NAME);
    return;
}

module_init(snake_dev_init);
module_exit(snake_dev_exit);

