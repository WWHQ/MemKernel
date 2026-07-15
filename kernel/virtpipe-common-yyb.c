#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/kernel.h>

#define DEVICE_NAME "virtpipe-common-yyb"
#define IOCTL_CHECK_CMD 0xC0046209
#define IOCTL_INIT_CMD  0x40046205

static int major_number;
static struct class* fake_class = NULL;
static struct cdev fake_cdev;

// 核心欺骗逻辑
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    printk(KERN_INFO "FAKE_DRIVER: 收到 ioctl 请求, CMD: 0x%X\n", cmd);

    if (cmd == IOCTL_CHECK_CMD) {
        int fake_val = 8; // 伪造校验结果为 8
        if (copy_to_user((int __user *)arg, &fake_val, sizeof(int)))
            return -EFAULT;
        return 0; // 返回成功 (0)
    } 
    else if (cmd == IOCTL_INIT_CMD) {
        return 0; // 模拟初始化成功
    }
    return -EINVAL;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = dev_ioctl,
};

static int __init mod_init(void) {
    dev_t dev;
    alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    major_number = MAJOR(dev);
    cdev_init(&fake_cdev, &fops);
    cdev_add(&fake_cdev, dev, 1);
    fake_class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(fake_class, NULL, dev, NULL, DEVICE_NAME);
    
    printk(KERN_INFO "FAKE_DRIVER: 已加载，虚拟管道设备 /dev/%s 已创建\n", DEVICE_NAME);
    return 0;
}

static void __exit mod_exit(void) {
    device_destroy(fake_class, MKDEV(major_number, 0));
    class_destroy(fake_class);
    cdev_del(&fake_cdev);
    unregister_chrdev_region(MKDEV(major_number, 0), 1);
}

module_init(mod_init);
module_exit(mod_exit);
MODULE_LICENSE("GPL");
