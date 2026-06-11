#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/set_memory.h>

// 驱动节点/dev/virtpipe-common-syzs
#include <linux/namei.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>

#include "comm.h"
#include "memory.h"
#include "process.h"

// 原始 Binder IOCTL 指针
static long (*original_binder_ioctl)(struct file *, unsigned int, unsigned long);

// 核心逻辑函数
static long dispatch_ioctl_parasite(struct file *const file, unsigned int const cmd, unsigned long const arg)
{
	// 移除 static，使用栈内存防止并发重入导致数据混乱
	COPY_MEMORY cm;
	MODULE_BASE mb;
	char name[0x100] = {0};
	
	static char key[0x100] = {0};
	static bool is_verified = false;
    // 并未启用
	if (cmd == OP_INIT_KEY && !is_verified)
	{
		if (copy_from_user(key, (void __user *)arg, sizeof(key) - 1) != 0)
		{
			return -1;
		}
	}
	switch (cmd)
	{
	case OP_READ_MEM:
	{
		if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0)
		{
			return -1;
		}
		if (read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false)
		{
			return -1;
		}
		break;
	}
	case OP_WRITE_MEM:
	{
		if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0)
		{
			return -1;
		}
		if (write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false)
		{
			return -1;
		}
		break;
	}
	case OP_MODULE_BASE:
	{
		if (copy_from_user(&mb, (void __user *)arg, sizeof(mb)) != 0 || copy_from_user(name, (void __user *)mb.name, sizeof(name) - 1) != 0)
		{
			return -1;
		}
		mb.base = get_module_base(mb.pid, name);
		if (copy_to_user((void __user *)arg, &mb, sizeof(mb)) != 0)
		{
			return -1;
		}
		break;
	}
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

// ******************************* 
// 目标节点路径
#define TARGET_NODE "/dev/virtpipe-common-syzs"
// Hook 目标：vfs_ioctl
static struct kprobe kp = {
    .symbol_name = "vfs_ioctl",
};

// 拦截逻辑
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    // ARM64 寄存器约定：x1 为 file 结构体指针，x2 为 cmd
    struct file *file = (struct file *)regs->regs[1];
    
    if (file && file->f_path.dentry) {
        // 匹配文件名
        if (strcmp(file->f_path.dentry->d_name.name, "virtpipe-common-syzs") == 0) {
            // 强行返回 0，模拟握手成功
            regs->regs[0] = 0; 
            
            // 跳过原始 vfs_ioctl 的执行，防止返回 -ENODEV
            p->post_handler = NULL;
            return 1; 
        }
    }
    return 0;
}

// 初始化：创建孤儿节点并挂载 Hook
static int __init parasite_init(void) {
    struct dentry *dentry;
    struct path path;

    // 1. 静默创建字符设备节点
    dentry = kern_path_create(AT_FDCWD, TARGET_NODE, &path, 0);
    if (!IS_ERR(dentry)) {
        // 创建字符设备节点 (S_IFCHR)，无需绑定驱动 (Major/Minor 设为 0)
        vfs_mknod(d_inode(path.dentry), dentry, 0666 | S_IFCHR, MKDEV(0, 0));
        done_path_create(&path, dentry);
    }

    // 2. 挂载 Kprobe 拦截 IOCTL
    kp.pre_handler = handler_pre;
    register_kprobe(&kp);

    return 0;
}
late_initcall(parasite_init);

// 使用 late_initcall 确保在设备驱动初始化后执行
late_initcall(parasite_init);



