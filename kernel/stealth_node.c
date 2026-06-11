#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include "stealth_node.h"

static long dummy_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    unsigned char buf[32];
    
    // 即使读取失败，也要记录下来，否则如果只是因为读取失败返回错误，判断就不准了
    if (copy_from_user(buf, (void __user *)arg, sizeof(buf)) == 0) {
        printk(KERN_INFO "virtpipe-debug: CMD: %u, Data: %*ph\n", cmd, (int)sizeof(buf), buf);
    }

    // 测试阶段：如果你想观察如果不返回 0 的反应
    // return -EINVAL; // 这一行替换掉 return 0;
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
