#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/set_memory.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>

// 假设这些是你原本定义在头文件中的依赖
#include "comm.h"
#include "memory.h"
#include "process.h"
#include "hide_proc.h"

// 字符设备相关变量
static int majorNumber;
static struct class* sysopClass = NULL;
static struct device* sysopDevice = NULL;
static struct cdev sysop_cdev;

static DEFINE_MUTEX(init_mutex);
static bool is_initialized = false;

// 原有的核心逻辑函数
static long sysop_dispatch_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    COPY_MEMORY cm;
    MODULE_BASE mb;
    char name[0x100] = {0};
    HIDE_PROC *hp = NULL;
    int ret = -1;

    switch (cmd)
    {
    case OP_READ_MEM:
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0) return -EFAULT;
        return sysop_read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) ? 0 : -EFAULT;

    case OP_WRITE_MEM:
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0) return -EFAULT;
        return sysop_write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) ? 0 : -EFAULT;

    case OP_MODULE_BASE:
        if (copy_from_user(&mb, (void __user *)arg, sizeof(mb)) != 0 || 
            copy_from_user(name, (void __user *)mb.name, sizeof(name) - 1) != 0)
            return -EFAULT;
        mb.base = sysop_get_module_base(mb.pid, name);
        return copy_to_user((void __user *)arg, &mb, sizeof(mb)) ? -EFAULT : 0;

    case OP_HIDE_PROC:
        if (!is_initialized) {
            mutex_lock(&init_mutex);
            if (!is_initialized) {
#ifdef CONFIG_HIDE_PROC_MODE
                if (hide_proc_init() == 0) is_initialized = true;
#endif
            }
            mutex_unlock(&init_mutex);
        }
        
        hp = kmalloc(sizeof(HIDE_PROC), GFP_KERNEL);
        if (!hp) return -ENOMEM;
        if (copy_from_user(hp, (void __user *)arg, sizeof(HIDE_PROC)) != 0) {
            kfree(hp); return -EFAULT;
        }
        
        if (hp->action == ACTION_HIDE) add_hidden_pid(hp->pid);
        else if (hp->action == ACTION_UNHIDE) remove_hidden_pid(hp->pid);
        else if (hp->action == ACTION_CLEAR) clear_hidden_pids();
        
        kfree(hp);
        return 0;

    default:
        return -EINVAL;
    }
}

// 设备操作接口
static long sysop_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    return sysop_dispatch_ioctl(file, cmd, arg);
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = sysop_ioctl,
};

static int __init sysop_init(void) {
    dev_t dev;
    
    // 1. 注册设备号
    alloc_chrdev_region(&dev, 0, 1, "sysop_control");
    majorNumber = MAJOR(dev);
    
    // 2. 初始化字符设备
    cdev_init(&sysop_cdev, &fops);
    cdev_add(&sysop_cdev, dev, 1);
    
    // 3. 创建设备节点 (/dev/sysop_control)
    sysopClass = class_create(THIS_MODULE, "sysop");
    sysopDevice = device_create(sysopClass, NULL, dev, NULL, "sysop_control");
    
    return 0;
}

static void __exit sysop_exit(void) {
    device_destroy(sysopClass, MKDEV(majorNumber, 0));
    class_destroy(sysopClass);
    cdev_del(&sysop_cdev);
    unregister_chrdev_region(MKDEV(majorNumber, 0), 1);
}

module_init(sysop_init);
module_exit(sysop_exit);
MODULE_LICENSE("GPL");
