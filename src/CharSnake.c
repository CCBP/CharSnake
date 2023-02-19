#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/cdev.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("CCBP");
MODULE_DESCRIPTION("Character Snake Game");

typedef struct _MAP_DEV {
    int size;
    void *data;
    struct cdev cdev;
} MAP_DEV;

#define MAP_SIZE (9 * 9)

static dev_t dev;
static int major = 0;
static int minor = 0;
static const char *DEV_NAME = "CharSnake";
static MAP_DEV *chrSnakeMap;

static void DestoryMapData(MAP_DEV *map_dev)
{
    kfree(map_dev->data);
    map_dev->data = NULL;
}

static ssize_t SnakeDraw(struct file *fp, char __user *buffer, size_t size, loff_t *pos)
{
    MAP_DEV *map_dev = fp->private_data;
    int result = 0;

    if (map_dev->data)
    {
        result = (map_dev->size - *pos) > size ? size : (map_dev->size - *pos);
        if (copy_to_user(buffer, (map_dev->data + *pos), result))
        {
            result = -EFAULT;
        }
        else
        {
            *pos += result;
        }
    }
    printk(KERN_ALERT "[%s] draw a map, size: %ld  pos: %lld  result: %d\n", 
                        DEV_NAME, size, *pos, result);

    return result;
}

static ssize_t SnakeMove(struct file *fp, const char __user *buffer, size_t size, loff_t *pos)
{
    MAP_DEV *map_dev = fp->private_data;
    int result = -ENOMEM;

    if (!map_dev->data)
    {
        map_dev->data = kmalloc(MAP_SIZE, GFP_KERNEL);
        if (!map_dev->data)
        {
            goto fail;
        }
    }

    result = (map_dev->size - *pos) > size ? size : (map_dev->size - *pos);
    if (copy_from_user((map_dev->data + *pos), buffer, result))
    {
        result = -EFAULT;
        goto fail;
    }

    *pos += result;

fail:
    printk(KERN_ALERT "[%s] move snake, size: %ld  pos: %lld  result: %d\n", 
                        DEV_NAME, size, *pos, result);
    return result;
}

static int SnakeStart(struct inode *inode, struct file *fp)
{
    MAP_DEV *map_dev = container_of(inode->i_cdev, MAP_DEV, cdev);
    
    if ((fp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        DestoryMapData(map_dev);
    }

    fp->private_data = map_dev;

    return 0;
}

static int SnakeStop(struct inode *inode, struct file *fp)
{
    printk(KERN_ALERT "[%s] stoped!\n", DEV_NAME);

    return 0;
}

struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = SnakeStart,
    .release = SnakeStop,
    .read    = SnakeDraw,
    .write   = SnakeMove,
};

static int SnakeSetupDev(MAP_DEV *map_dev, int index)
{
    int result = 0;
    dev_t dt = MKDEV(major, index);

    cdev_init(&map_dev->cdev, &fops);
    map_dev->cdev.owner = THIS_MODULE;
    map_dev->cdev.ops   = &fops;
    result = cdev_add(&map_dev->cdev, dt, 1);

    return result;
}

static int SnakeInit(void)
{
    int result = 0;
    
    result = alloc_chrdev_region(&dev, minor, 1, DEV_NAME);
    major = MAJOR(dev);

    if (result < 0)
    {
        printk(KERN_ALERT "[%s] register dev error! error code: %d\n", DEV_NAME, result);
        return result;
    }

    chrSnakeMap = kmalloc(sizeof(MAP_DEV), GFP_KERNEL);
    if (!chrSnakeMap)
    {
        printk(KERN_ALERT "[%s] no memory!\n", DEV_NAME);
        result = -ENOMEM;
        goto fail;
    }

    memset(chrSnakeMap, 0, sizeof(MAP_DEV));
    chrSnakeMap->size = MAP_SIZE;
    
    result = SnakeSetupDev(chrSnakeMap, 0);
    if (result < 0)
    {
        printk(KERN_ALERT "[%s] cdev add fail! error code: %d\n", DEV_NAME, result);
        goto fail;
    }

    printk(KERN_ALERT "[%s] init success! major: %d", DEV_NAME, major);
    return result;

fail:
    unregister_chrdev_region(dev, 1);
    return result;
}

static void SnakeExit(void)
{
    cdev_del(&chrSnakeMap->cdev);
    DestoryMapData(chrSnakeMap);
    kfree(chrSnakeMap);
    chrSnakeMap = NULL;
    unregister_chrdev_region(dev, 1);

    printk(KERN_ALERT "[%s] exit!\n", DEV_NAME);
    return;
}

module_init(SnakeInit);
module_exit(SnakeExit);

