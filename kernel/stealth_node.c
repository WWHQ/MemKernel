#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include "stealth_node.h"

static long dummy_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // 假设 arg 是一个指针，打印前 32 字节，看看数据特征
    unsigned char buf[32];
    if (copy_from_user(buf, (void __user *)arg, sizeof(buf)) == 0) {
        printk(KERN_INFO "virtpipe-debug: cmd=%u, data=%*ph\n", cmd, (int)sizeof(buf), buf);
    }
    
    // 如果它是挑战包，你需要根据其特征返回特定的握手包，而不能只返回 0
    return 0; 
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
