#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>

#define DEVICE_NAME "virtpipe-common-yyb"
#define CLASS_NAME  "virtpipe_class"

static int major_number;
static struct class* virtpipe_class  = NULL;
static struct device* virtpipe_device = NULL;

// 伪装的 TP 协议头（模拟握手数据）
static char fake_handshake[] = {0x55, 0xAA, 0x01, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};

static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    // 每次读取固定返回伪造的协议数据，防止 TP 检测到 EOF
    if (*offset >= sizeof(fake_handshake)) return 0;
    if (copy_to_user(buffer, fake_handshake + *offset, sizeof(fake_handshake) - *offset))
        return -EFAULT;
    *offset += sizeof(fake_handshake);
    return sizeof(fake_handshake);
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
    // 核心伪装：无论 TP 发送什么探测指令，统统返回成功 (0)
    // 这可以让 TP 认为你的虚拟驱动版本与真机一致
    return 0; 
}

static int dev_release(struct inode *inodep, struct file *filep) {
    return 0;
}

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .unlocked_ioctl = dev_ioctl,
    .release = dev_release,
};

static int __init virtpipe_init(void) {
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) return major_number;

    virtpipe_class = class_create(THIS_MODULE, CLASS_NAME);
    virtpipe_device = device_create(virtpipe_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    
    return 0;
}

static void __exit virtpipe_exit(void) {
    device_destroy(virtpipe_class, MKDEV(major_number, 0));
    class_unregister(virtpipe_class);
    class_destroy(virtpipe_class);
    unregister_chrdev(major_number, DEVICE_NAME);
}

module_init(virtpipe_init);
module_exit(virtpipe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Collaborator");
MODULE_DESCRIPTION("Virtual Pipe for TP Anti-Cheat Spoofing");
