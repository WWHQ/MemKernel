#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include "stealth_node.h"

// 统一前缀：sysop_
static long sysop_dummy_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    return 0; 
}

static const struct file_operations sysop_dummy_fops = {
    .owner = THIS_MODULE,
    .open = simple_open,
    .unlocked_ioctl = sysop_dummy_ioctl,
};

static struct miscdevice sysop_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = TARGET_NODE_NAME, // 节点名字
    .fops = &sysop_dummy_fops,
    .mode = 0666,
};

// 统一初始化名称
static int __init sysop_init(void) {
    return misc_register(&sysop_misc_dev);
}

// 统一卸载名称
static void __exit sysop_exit(void) {
    misc_deregister(&sysop_misc_dev);
}

late_initcall(sysop_init);
module_exit(sysop_exit);
