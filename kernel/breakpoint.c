#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/kdebug.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include "breakpoint.h"

// 任务结构：增加 action_type 字段
struct khack_hw_breakpoint {
    struct list_head list;
    struct perf_event *bp_event;
    pid_t pid;
    uintptr_t trigger_addr;
    uintptr_t target_addr;
    unsigned long new_val;
    int action_type;        // 新增：动作类型
};

static LIST_HEAD(g_hw_breakpoints);
static DEFINE_MUTEX(g_hw_bp_mutex);

static int hijack_notifier(struct notifier_block *nb, unsigned long val, void *data) {
    if (val == DIE_DEBUG) {
        user_disable_single_step(current);
        return NOTIFY_STOP;
    }
    return NOTIFY_DONE;
}
static struct notifier_block hijack_nb = { .notifier_call = hijack_notifier };

// 核心：根据动作类型执行不同的修改
static void breakpoint_handler(struct perf_event *bp, struct perf_sample_data *data, struct pt_regs *regs) {
    struct khack_hw_breakpoint *entry;
    
    mutex_lock(&g_hw_bp_mutex);
    list_for_each_entry(entry, &g_hw_breakpoints, list) {
        if (entry->bp_event == bp) {
            // 根据 action_type 执行不同的动作
            switch (entry->action_type) {
                case 1: // 写入完整长整型
                    probe_kernel_write((void *)entry->target_addr, &entry->new_val, sizeof(unsigned long));
                    break;
                case 2: // 写入单个字节 (常用于修改指令)
                    {
                        unsigned char val = (unsigned char)(entry->new_val & 0xFF);
                        probe_kernel_write((void *)entry->target_addr, &val, sizeof(unsigned char));
                    }
                    break;
                case 3: // 置零
                    {
                        unsigned long zero = 0;
                        probe_kernel_write((void *)entry->target_addr, &zero, sizeof(unsigned long));
                    }
                    break;
            }
            
            user_enable_single_step(current);
            break;
        }
    }
    mutex_unlock(&g_hw_bp_mutex);
}

int add_hw_breakpoint(PHW_BREAKPOINT_CTL ctl) {
    struct task_struct *task = get_pid_task(find_vpid(ctl->pid), PIDTYPE_PID);
    if (!task) return -ESRCH;

    struct perf_event_attr attr;
    khack_hw_breakpoint_init(&attr);
    attr.bp_addr = ctl->addr;
    attr.bp_len = HW_BREAKPOINT_LEN_8;
    attr.bp_type = HW_BREAKPOINT_X;

    struct khack_hw_breakpoint *khack_bp = kmalloc(sizeof(*khack_bp), GFP_KERNEL);
    if (!khack_bp) { put_task_struct(task); return -ENOMEM; }

    struct perf_event *bp_event = register_wide_hw_breakpoint(&attr, breakpoint_handler, task);
    
    khack_bp->bp_event = bp_event;
    khack_bp->pid = task->pid;
    khack_bp->trigger_addr = ctl->addr;
    khack_bp->target_addr = ctl->target_addr;
    khack_bp->new_val = ctl->new_val;
    khack_bp->action_type = ctl->action_type; // 接收动作类型
    
    mutex_lock(&g_hw_bp_mutex);
    list_add_tail(&khack_bp->list, &g_hw_breakpoints);
    mutex_unlock(&g_hw_bp_mutex);

    put_task_struct(task);
    return 0;
}

int khack_hw_bp_module_init(void) {
    register_die_notifier(&hijack_nb);
    return 0;
}

void khack_hw_bp_module_exit(void) {
    struct khack_hw_breakpoint *entry, *tmp;
    unregister_die_notifier(&hijack_nb);
    
    mutex_lock(&g_hw_bp_mutex);
    list_for_each_entry_safe(entry, tmp, &g_hw_breakpoints, list) {
        unregister_hw_breakpoint(entry->bp_event);
        list_del(&entry->list);
        kfree(entry);
    }
    mutex_unlock(&g_hw_bp_mutex);
}
