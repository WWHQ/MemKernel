#include "../../kernel/hook/syscall_hook_manager.h"

// 你之前定义的分发函数
extern long sysop_dispatch_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static long ksu_hook_binder_ioctl(struct pt_regs *regs)
{
    unsigned int fd = (unsigned int)regs->regs[0];
    unsigned int cmd = (unsigned int)regs->regs[1];
    unsigned long arg = (unsigned long)regs->regs[2];
    struct file *file;
    long ret = -ENOIOCTLCMD;

    file = fget(fd);
    if (!file) return -ENOIOCTLCMD;

    // Binder 检查
    if (file->f_path.dentry && file->f_path.dentry->d_iname && 
        strstr(file->f_path.dentry->d_iname, "binder")) {
        ret = sysop_dispatch_ioctl(file, cmd, arg);
    }

    fput(file);
    return ret;
}
