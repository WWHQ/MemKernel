#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include "stealth_node.h"

static long dummy_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    return 0; // 默认维持现有逻辑
}

static const struct file_operations dummy_fops = {
    .owner = THIS_MODULE,
    .open = simple_open,
    .unlocked_ioctl = dummy_ioctl,
};

static struct miscdevice stealth_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = TARGET_NODE_NAME,
    .fops = &dummy_fops,
    .mode = 0666,
};

static int __init stealth_node_init(void) {
    return misc_register(&stealth_misc_dev);
}

static void __exit stealth_node_exit(void) {
    misc_deregister(&stealth_misc_dev);
}

late_initcall(stealth_node_init);
module_exit(stealth_node_exit);
