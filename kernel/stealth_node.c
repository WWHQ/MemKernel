#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kprobes.h>
#include "stealth_node.h"

// 挂钩 vfs_ioctl 实现零驱动响应
static struct kprobe kp = { .symbol_name = "vfs_ioctl" };

// 拦截逻辑
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    struct file *file = (struct file *)regs->regs[1];
    
    if (file && file->f_path.dentry) {
        if (strcmp(file->f_path.dentry->d_name.name, TARGET_NODE) == 0) {
            // 强行修改返回寄存器 x0 为 0，欺骗游戏已完成 IOCTL 通信
            regs->regs[0] = 0;
            p->post_handler = NULL; // 直接跳过原函数执行
            return 1;
        }
    }
    return 0;
}

static int __init stealth_node_init(void) {
    struct device *my_dev;

    // 1. 注册 Hook
    kp.pre_handler = handler_pre;
    register_kprobe(&kp);

    // 2. 挂靠系统原生 misc 类 (完全隐身，无类名特征)
    my_dev = device_create(misc_class, NULL, MKDEV(MISC_MAJOR, TARGET_MINOR), NULL, TARGET_NODE);
    
    if (!IS_ERR(my_dev)) {
        // 3. 强制赋予可读写权限 (0666)
        sysfs_chmod_file(&my_dev->kobj, NULL, 0666);
    }

    return 0;
}

static void __exit stealth_node_exit(void) {
    device_destroy(misc_class, MKDEV(MISC_MAJOR, TARGET_MINOR));
    unregister_kprobe(&kp);
}

late_initcall(stealth_node_init);
module_exit(stealth_node_exit);

MODULE_LICENSE("GPL");
