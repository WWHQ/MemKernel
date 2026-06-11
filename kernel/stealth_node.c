#include <linux/init.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/compiler.h>
#include "stealth_node.h"

static struct kprobe kp_access, kp_stat, kp_ioctl;

static __maybe_unused int handler_access_pre(struct kprobe *p, struct pt_regs *regs) {
    char name[64];
    const char __user *path = (const char __user *)regs->regs[1];
    if (copy_from_user(name, path, sizeof(name)) == 0 && strstr(name, TARGET_NODE_NAME)) {
        regs->regs[0] = 0; 
        return 1; 
    }
    return 0;
}

// 修正后的 post_handler
static __maybe_unused void handler_stat_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags) {
    struct kstat __user *statbuf = (struct kstat __user *)regs->regs[2];
    if (statbuf) {
        unsigned int mode = S_IFCHR | 0666;
        copy_to_user(&statbuf->mode, &mode, sizeof(mode));
    }
}

static __maybe_unused int handler_ioctl_pre(struct kprobe *p, struct pt_regs *regs) {
    struct file *file = (struct file *)regs->regs[1];
    if (file && file->f_path.dentry && strstr(file->f_path.dentry->d_name.name, TARGET_NODE_NAME)) {
        regs->regs[0] = 0; 
        return 1;
    }
    return 0;
}

static int __init stealth_node_init(void) {
    kp_access.symbol_name = "sys_faccessat";
    kp_access.pre_handler = handler_access_pre;
    register_kprobe(&kp_access);

    kp_stat.symbol_name = "sys_newfstatat";
    kp_stat.post_handler = handler_stat_post; // 现在类型匹配了
    register_kprobe(&kp_stat);

    kp_ioctl.symbol_name = "vfs_ioctl";
    kp_ioctl.pre_handler = handler_ioctl_pre;
    register_kprobe(&kp_ioctl);
    
    return 0;
}

static void __exit stealth_node_exit(void) {
    unregister_kprobe(&kp_access);
    unregister_kprobe(&kp_stat);
    unregister_kprobe(&kp_ioctl);
}

late_initcall(stealth_node_init);
module_exit(stealth_node_exit);
MODULE_LICENSE("GPL");
