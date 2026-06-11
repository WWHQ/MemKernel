#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/set_memory.h>

// 驱动节点/dev/virtpipe-common-syzs
#include <linux/cdev.h>
#include <linux/device.h>

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
// 1. 定义“哑”IOCTL 处理函数，所有请求直接返回 0
static long dummy_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // 游戏询问任何环境参数，都返回 0 (表示成功/已连接)
    return 0; 
}

// 2. 定义 file_operations，只实现必须的 open 和 ioctl
static const struct file_operations dummy_fops = {
    .owner = THIS_MODULE,
    .open = simple_open, // 使用内核内置的空打开函数
    .unlocked_ioctl = dummy_ioctl,
};

// 3. 将其关联到节点
static struct cdev my_cdev;
static struct class *my_class;

static int __init setup_dummy_driver(void) {
    dev_t dev_num;
    alloc_chrdev_region(&dev_num, 0, 1, "fake_node_driver");
    
    cdev_init(&my_cdev, &dummy_fops);
    cdev_add(&my_cdev, dev_num, 1);
    
    // 创建设备节点，关联 dummy_fops
    my_class = class_create(THIS_MODULE, "fake_node_class");
    device_create(my_class, NULL, dev_num, NULL, "virtpipe-common-syzs");
    
    return 0;
}

late_initcall(setup_dummy_driver);

// 使用 late_initcall 确保在设备驱动初始化后执行
late_initcall(parasite_init);



