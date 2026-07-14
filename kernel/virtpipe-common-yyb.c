#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static int fake_cpuinfo_show(struct seq_file *m, void *v) {
    seq_printf(m, "processor\t: 0\n");
    seq_printf(m, "vendor_id\t: GenuineIntel\n");
    seq_printf(m, "cpu family\t: 6\n");
    seq_printf(m, "model\t\t: 165\n");
    seq_printf(m, "model name\t: Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz\n");
    seq_printf(m, "stepping\t: 5\n");
    seq_printf(m, "cpu MHz\t\t: 3800.000\n");
    seq_printf(m, "cache size\t: 16384 KB\n");
    seq_printf(m, "flags\t\t: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf pni pclmulqdq dtes64 monitor ds_cpl vmx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single pti ssbd ibrs ibpb stibp tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid mpx rdseed adx smap clflushopt intel_pt xsaveopt xsavec xgetbv1 xsaves dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp\n");
    return 0;
}

static int fake_cpuinfo_open(struct inode *inode, struct file *file) {
    return single_open(file, fake_cpuinfo_show, NULL);
}

static const struct file_operations fake_cpuinfo_fops = {
    .owner   = THIS_MODULE,
    .open    = fake_cpuinfo_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int __init virtpipe_init(void) {
    remove_proc_entry("cpuinfo", NULL);
    proc_create("cpuinfo", 0444, NULL, &fake_cpuinfo_fops);
    return 0;
}

static void __exit virtpipe_exit(void) {
    remove_proc_entry("cpuinfo", NULL);
}

module_init(virtpipe_init);
module_exit(virtpipe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("System");
MODULE_DESCRIPTION("virtpipe-common-yyb");
