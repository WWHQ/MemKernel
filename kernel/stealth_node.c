#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kprobes.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include "stealth_node.h"

static struct kprobe kp = { .symbol_name = "vfs_ioctl" };

static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    struct file *file = (struct file *)regs->regs[1];
    if (file && file->f_path.dentry) {
        if (strcmp(file->f_path.dentry->d_name.name, TARGET_NODE) == 0) {
            regs->regs[0] = 0; // 模拟成功
            p->post_handler = NULL;
            return 1;
        }
    }
    return 0;
}

static int __init stealth_node_init(void) {
    struct dentry *dentry;
    struct path path;
    int err;

    // 1. 注册 IOCTL 拦截器
    kp.pre_handler = handler_pre;
    register_kprobe(&kp);

    // 2. 直接在 VFS 层创建节点，不依赖任何驱动模型
    err = kern_path("/dev", 0, &path);
    if (err == 0) {
        // 创建节点：类型为字符设备，权限 0666
        dentry = lookup_one_len(TARGET_NODE, path.dentry, strlen(TARGET_NODE));
        if (!IS_ERR(dentry)) {
            // 注意：MKDEV(0, 0) 是合法的，它代表一个未关联驱动的字符设备
            vfs_mknod(d_inode(path.dentry), dentry, S_IFCHR | 0666, MKDEV(0, 0));
            dput(dentry);
        }
        path_put(&path);
    }
    return 0;
}

late_initcall(stealth_node_init);
MODULE_LICENSE("GPL");
