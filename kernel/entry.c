#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/set_memory.h>
#include "comm.h"
#include "memory.h"
#include "process.h"

// 原始 Binder IOCTL 指针
static long (*original_binder_ioctl)(struct file *, unsigned int, unsigned long);

// 核心逻辑函数
static long dispatch_ioctl_parasite(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct CopyMemory cm;
    struct ModuleBase mb;
    char name[0x100] = {0};

    switch (cmd) {
    case OP_READ_MEM:
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0) return -1;
        return readwrite_process_memory(cm.pid, cm.addr, cm.buffer, cm.size, false);
    case OP_WRITE_MEM:
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0) return -1;
        return readwrite_process_memory(cm.pid, cm.addr, cm.buffer, cm.size, true);
    case OP_MODULE_BASE:
        if (copy_from_user(&mb, (void __user *)arg, sizeof(mb)) != 0 ||
            copy_from_user(name, (void __user *)mb.name, sizeof(name) - 1) != 0) return -1;
        mb.base = get_module_base(mb.pid, name);
        if (copy_to_user((void __user *)arg, &mb, sizeof(mb)) != 0) return -1;
        break;
    default:
        return -ENOIOCTLCMD;
    }
    return 0;
}

// 拦截路由函数
static long hooked_binder_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    long ret = dispatch_ioctl_parasite(file, cmd, arg);
    if (ret == -ENOIOCTLCMD) {
        return original_binder_ioctl(file, cmd, arg);
    }
    return ret;
}

// 静态初始化：系统启动自动挂载
static int __init parasite_init(void) {
    struct file_operations *binder_fops = (struct file_operations *)kallsyms_lookup_name("binder_fops");
    if (!binder_fops) return 0;
    
    original_binder_ioctl = binder_fops->unlocked_ioctl;
    
    set_memory_rw((unsigned long)binder_fops & PAGE_MASK, 1);
    binder_fops->unlocked_ioctl = hooked_binder_ioctl;
    set_memory_ro((unsigned long)binder_fops & PAGE_MASK, 1);
    
    return 0;
}
late_initcall(parasite_init);

