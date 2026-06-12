/* --- MemKernel Injection Start --- */
#include <linux/file.h>

// 核心分发逻辑，直接嵌入到 syscall_hook_manager.c 的编译单元中
static long ksu_hook_binder_ioctl(struct pt_regs *regs)
{
    unsigned int fd = (unsigned int)regs->regs[0];
    unsigned int cmd = (unsigned int)regs->regs[1];
    unsigned long arg = (unsigned long)regs->regs[2];
    struct file *file;
    long ret = -ENOIOCTLCMD;

    file = fget(fd);
    if (!file) return -ENOIOCTLCMD;

    // 过滤逻辑：判断是否是 Binder
    if (file->f_path.dentry && file->f_path.dentry->d_iname && 
        strstr(file->f_path.dentry->d_iname, "binder")) {
        
        // 调用你的原始处理逻辑 (假设你已经在其他地方定义好了)
        // 注意：这里为了编译通过，可以直接写死逻辑或者引用你的头文件
        extern long sysop_dispatch_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
        ret = sysop_dispatch_ioctl(file, cmd, arg);
    }

    fput(file);
    return ret;
}
/* --- MemKernel Injection End --- */
