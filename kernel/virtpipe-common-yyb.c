#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>

#define FAKE_NAME "QEMU HARDDISK"
#define FAKE_TYPE "MMC"
#define FAKE_CID  "00000000000000000000000000000000"

static struct kobject *mmc_kobj;
static struct kobject *device_kobj;

static ssize_t name_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", FAKE_NAME);
}
static ssize_t type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", FAKE_TYPE);
}
static ssize_t cid_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", FAKE_CID);
}

static struct kobj_attribute name_attr = __ATTR(name, 0444, name_show, NULL);
static struct kobj_attribute type_attr = __ATTR(type, 0444, type_show, NULL);
static struct kobj_attribute cid_attr  = __ATTR(cid, 0444, cid_show, NULL);

static int __init virtpipe_init(void) {
    struct kobject *block_kobj = kset_find_obj(block_kset, "block");
    if (!block_kobj) block_kobj = kernel_kobj;

    mmc_kobj = kobject_create_and_add("mmcblk0", block_kobj);
    if (!mmc_kobj) return -ENOMEM;

    device_kobj = kobject_create_and_add("device", mmc_kobj);
    if (!device_kobj) {
        kobject_put(mmc_kobj);
        return -ENOMEM;
    }

    sysfs_create_file(device_kobj, &name_attr.attr);
    sysfs_create_file(device_kobj, &type_attr.attr);
    sysfs_create_file(device_kobj, &cid_attr.attr);

    return 0;
}

static void __exit virtpipe_exit(void) {
    sysfs_remove_file(device_kobj, &name_attr.attr);
    sysfs_remove_file(device_kobj, &type_attr.attr);
    sysfs_remove_file(device_kobj, &cid_attr.attr);
    kobject_put(device_kobj);
    kobject_put(mmc_kobj);
}

module_init(virtpipe_init);
module_exit(virtpipe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("System");
MODULE_DESCRIPTION("virtpipe-common-yyb");
