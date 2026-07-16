#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* * 这是一个简化的演示思路：
 * 真正的完全伪装通常需要覆盖 seq_file 的 show 函数，
 * 这里演示如何创建一个自定义的 proc 文件来模拟 cpuinfo
 */

static int my_cpuinfo_show(struct seq_file *m, void *v)
{
    // 在这里写入你想要伪装的任何内容
    seq_printf(m, "processor\t: 0\n");
    seq_printf(m, "model name\t: Intel(R) Core(TM) i9-12900K CPU @ 3.20GHz\n");
    seq_printf(m, "bogomips\t: 9999.99\n");
    return 0;
}

static int my_cpuinfo_open(struct inode *inode, struct file *file)
{
    return single_open(file, my_cpuinfo_show, NULL);
}

static const struct proc_ops my_proc_ops = {
    .proc_open    = my_cpuinfo_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init init_fake_cpu(void)
{
    // 如果要完全覆盖原有的 /proc/cpuinfo，需要移除原入口并替换
    // 注意：这在某些内核版本中可能需要修改内核符号表
    proc_create("cpuinfo_fake", 0, NULL, &my_proc_ops);
    printk(KERN_INFO "Fake CPU module loaded\n");
    return 0;
}

static void __exit exit_fake_cpu(void)
{
    remove_proc_entry("cpuinfo_fake", NULL);
}

module_init(init_fake_cpu);
module_exit(exit_fake_cpu);
MODULE_LICENSE("GPL");
