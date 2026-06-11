#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kprobes.h>
#include <linux/device.h>
#include "stealth_node.h"

// 将类定义为静态，防止命名冲突
static struct class *stealth_class;
static struct kprobe kp = { .symbol_name = "vfs_ioctl" };

static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    struct file *file = (struct file *)regs->regs[1];
    if (file && file->f_path.dentry) {
        if (strcmp(file->f_path.dentry->d_name.name, TARGET_NODE) == 0) {
            regs->regs[0] = 0; 
            p->post_handler = NULL;
            return 1;
        }
    }
    return 0;
}

static int __init stealth_node_init(void) {
    // 1. 创建一个类名为 "input" 的类，完美伪装成系统输入设备类
    // 这样在 /sys/class/input/ 下创建节点，极度安全
    stealth_class = class_create(THIS_MODULE, "input");
    if (IS_ERR(stealth_class)) return PTR_ERR(stealth_class);

    // 2. 创建设备节点，挂在 "input" 类下
    device_create(stealth_class, NULL, MKDEV(0, 0), NULL, TARGET_NODE);

    // 3. 注册 Hook
    kp.pre_handler = handler_pre;
    register_kprobe(&kp);

    return 0;
}

static void __exit stealth_node_exit(void) {
    device_destroy(stealth_class, MKDEV(0, 0));
    class_destroy(stealth_class);
    unregister_kprobe(&kp);
}

late_initcall(stealth_node_init);
MODULE_LICENSE("GPL");
