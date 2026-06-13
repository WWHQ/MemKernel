#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include "stealth_node.h"

MODULE_LICENSE("GPL");

static struct miscdevice sysop_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = TARGET_NODE_NAME,
    .mode = 0666,
};

static int __init sysop_init(void) {
    return misc_register(&sysop_misc_dev);
}

static void __exit sysop_exit(void) {
    misc_deregister(&sysop_misc_dev);
}

module_init(sysop_init);
module_exit(sysop_exit);
