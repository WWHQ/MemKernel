#include <linux/init.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "stealth_node.h"

// 1. Hook sys_openat：欺骗游戏，让它认为文件存在
static struct kprobe kp_open = { .symbol_name = "sys_openat" };

static int handler_open_pre(struct kprobe *p, struct pt_regs *regs) {
    const char __user *pathname = (const char __user *)regs->regs[1];
    char name[64];
    
    if (copy_from_user(name, pathname, sizeof(name)) == 0) {
        if (strstr(name, TARGET_NODE_NAME)) {
            // 当游戏尝试打开 virtpipe-common-syzs 时
            // 我们将其重定向到 /dev/null，欺骗它成功打开了一个句柄
            copy_to_user((void __user *)pathname, "/dev/null", 10);
        }
    }
    return 0;
}

// 2. Hook vfs_ioctl：处理游戏发送的握手指令
static struct kprobe kp_ioctl = { .symbol_name = "vfs_ioctl" };

static int handler_ioctl_pre(struct kprobe *p, struct pt_regs *regs) {
    struct file *file = (struct file *)regs->regs[1];
    // 如果是游戏针对我们伪造的句柄发送 IOCTL
    if (file && file->f_path.dentry) {
        // 这里通过判断文件路径或相关上下文确认是否为目标通信
        // 由于我们将路径改为了 /dev/null，可以判断 f_path
        if (strstr(file->f_path.dentry->d_name.name, "null")) {
            regs->regs[0] = 0; // 直接返回成功
            p->post_handler = NULL;
            return 1;
        }
    }
    return 0;
}

static int __init stealth_node_init(void) {
    kp_open.pre_handler = handler_open_pre;
    register_kprobe(&kp_open);

    kp_ioctl.pre_handler = handler_ioctl_pre;
    register_kprobe(&kp_ioctl);
    
    return 0;
}

static void __exit stealth_node_exit(void) {
    unregister_kprobe(&kp_open);
    unregister_kprobe(&kp_ioctl);
}

late_initcall(stealth_node_init);
module_exit(stealth_node_exit);
