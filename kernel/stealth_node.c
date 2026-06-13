#include <linux/module.h>
#include <linux/kobject.h>

static struct kobject *my_kobj;

static int __init sysop_init(void) {
    // 在 /sys/kernel/ 下创建一个名为 "my_folder" 的文件夹
    my_kobj = kobject_create_and_add("my_folder", kernel_kobj);
    if (!my_kobj)
        return -ENOMEM;
    return 0;
}

static void __exit sysop_exit(void) {
    kobject_put(my_kobj);
}

module_init(sysop_init);
module_exit(sysop_exit);
MODULE_LICENSE("GPL");
