#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include "stealth_node.h"

// 所有的 IOCTL 请求直接返回 0，模拟握手成功
static long dummy_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    return 0; 
}

// 最小化的文件操作集，仅包含打开和 ioctl
static const struct file_operations dummy_fops = {
    .owner = THIS_MODULE,
    .open = simple_open,
    .unlocked_ioctl = dummy_ioctl,
};

// 定义 misc 设备结构
static struct miscdevice stealth_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = TARGET_NODE_NAME,
    .fops = &dummy_fops,
    .mode = 0666, // 强制设置权限为 0666，确保游戏进程无需 root 即可访问
};

// 模块初始化
static int __init stealth_node_init(void) {
    int ret = misc_register(&stealth_misc_dev);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

// 模块卸载
static void __exit stealth_node_exit(void) {
    misc_deregister(&stealth_misc_dev);
}

late_initcall(stealth_node_init);
module_exit(stealth_node_exit);
