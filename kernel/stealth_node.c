#include <linux/init.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/syscalls.h>
#include "stealth_node.h"

// 1. 核心：定义一个全局的“影子路径”
// 当任何程序访问此路径时，我们进行重定向
static int is_target_path(const char __user *pathname) {
    char name[64];
    if (copy_from_user(name, pathname, sizeof(name)) == 0) {
        return strstr(name, TARGET_NODE_NAME) != NULL;
    }
    return 0;
}

// 2. 拦截 sys_faccessat (用于 access() 检查文件是否存在)
static struct kprobe kp_access = { .symbol_name = "sys_faccessat" };
static int handler_access_pre(struct kprobe *p, struct pt_regs *regs) {
    if (is_target_path((const char __user *)regs->regs[1])) {
        // 直接返回 0，告诉游戏：该文件存在且可访问
        regs->regs[0] = 0; 
        return 1; // 阻止原函数执行
    }
    return 0;
}

// 3. 拦截 sys_newfstatat (用于 stat() 检查文件属性)
static struct kprobe kp_stat = { .symbol_name = "sys_newfstatat" };
static int handler_stat_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags) {
    struct kstat __user *statbuf = (struct kstat __user *)regs->regs[2];
    if (statbuf) {
        // 伪造：告诉游戏这是一个合法的字符设备
        unsigned int mode = S_IFCHR | 0666;
        copy_to_user(&statbuf->mode, &mode, sizeof(mode));
    }
    return 0;
}

// 4. 拦截 sys_openat
static struct kprobe kp_open = { .symbol_name = "sys_openat" };
static int handler_open_pre(struct kprobe *p, struct pt_regs *regs) {
    if (is_target_path((const char __user *)regs->regs[1])) {
        // 重定向到一个合法的系统设备，确保句柄合法
        copy_to_user((void __user *)regs->regs[1], "/dev/null", 10);
    }
    return 0;
}

static int __init stealth_node_init(void) {
    register_kprobe(&kp_access);
    register_kprobe(&kp_stat);
    register_kprobe(&kp_open);
    return 0;
}
late_initcall(stealth_node_init);
