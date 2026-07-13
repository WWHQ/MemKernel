#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define DEVICE_NAME "virtpipe-common-yyb"
static int major_number;

static int dev_open(struct inode *inodep, struct file *filep) { return 0; }
static int dev_release(struct inode *inodep, struct file *filep) { return 0; }

// 核心：处理 TP 的底层状态轮询，返回 TGB (腾讯手游助手) 标准响应
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // 这里需要根据抓包到的 TP 交互指令进行适配
    // 简单返回 0 表示支持且运行正常
    return 0; 
}

static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl,
};

static int __init virtpipe_init(void) {
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    // 这样在 /dev 下会自动生成设备节点
    return 0;
}
module_init(virtpipe_init);
