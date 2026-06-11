#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/set_memory.h>

// 驱动节点/dev/virtpipe-common-syzs
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/miscdevice.h> // 提供 misc_class 支持

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

// **************************************************** 
// 目标文件节点
#define TARGET_NODE "virtpipe-common-syzs"

// 全局变量：用于保存创建的设备节点指针
static struct device *my_dev;

// 挂钩目标：vfs_ioctl
static struct kprobe kp = {
    .symbol_name = "vfs_ioctl",
};

// 拦截逻辑：当游戏对该节点进行 IOCTL 时，直接返回成功
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    struct file *file = (struct file *)regs->regs[1]; // x1: file*
    
    if (file && file->f_path.dentry) {
        if (strcmp(file->f_path.dentry->d_name.name, TARGET_NODE) == 0) {
            // 将 x0 (返回值寄存器) 设为 0，代表操作成功
            regs->regs[0] = 0;
            // 跳过原函数，直接返回
            p->post_handler = NULL;
            return 1;
        }
    }
    return 0;
}

static int __init stealth_node_init(void) {
    // 1. 注册 Hook
    kp.pre_handler = handler_pre;
    register_kprobe(&kp);

    // 2. 挂靠系统原生的 "misc" 类，绝不创建自定义类
    // 这样在 /sys/class 下就不会出现自定义目录
    my_dev = device_create(misc_class, NULL, MKDEV(MISC_MAJOR, 250), NULL, TARGET_NODE);
    
    if (IS_ERR(my_dev)) {
        return PTR_ERR(my_dev);
    }

    // 3. 强制修改权限为 0666，确保普通游戏进程可读写
    // 在 Android 内核中，可以直接通过 kobject 访问来修改权限
    sysfs_chmod_file(&my_dev->kobj, NULL, 0666);

    return 0;
}

static void __exit stealth_node_exit(void) {
    device_destroy(misc_class, MKDEV(MISC_MAJOR, 250));
    unregister_kprobe(&kp);
}

late_initcall(stealth_node_init);
module_exit(stealth_node_exit);
//************************************************************

// 使用 late_initcall 确保在设备驱动初始化后执行
late_initcall(parasite_init);



