#ifndef HW_BREAKPOINT_H
#define HW_BREAKPOINT_H

#include <linux/types.h>

// 硬件断点动作类型 (配合 handle_hw_breakpoint_control)
#define HW_BP_ADD    0
#define HW_BP_REMOVE 1

// 触发类型
#define HW_BP_TYPE_EXECUTE 0
#define HW_BP_TYPE_WRITE   1
#define HW_BP_TYPE_RW      2

// 劫持动作类型 (Action Types)
#define ACTION_WRITE_MEM   1 // 写入完整长整型
#define ACTION_PATCH_BYTE  2 // 修改单个字节
#define ACTION_SET_ZERO    3 // 置零

// 硬件断点控制结构体
typedef struct {
    int action;             // ADD 或 REMOVE
    pid_t pid;              // 目标进程 ID
    uintptr_t addr;         // 触发断点的地址 (Trigger Address)
    int len;                // 长度 (通常用 HW_BREAKPOINT_LEN_8)
    int type;               // 触发类型 (EXECUTE, WRITE 等)
    
    // 劫持动作参数
    uintptr_t target_addr;  // 要修改的内存地址 (Target Address)
    unsigned long new_val;  // 要写入的值
    int action_type;        // 动作类型 (1, 2, 3)
} HW_BREAKPOINT_CTL, *PHW_BREAKPOINT_CTL;

// 必要的函数声明
int khack_hw_breakpoint_init(struct perf_event_attr *attr);
int add_hw_breakpoint(PHW_BREAKPOINT_CTL ctl);
int remove_hw_breakpoint(PHW_BREAKPOINT_CTL ctl);
int handle_hw_breakpoint_control(PHW_BREAKPOINT_CTL ctl);
int khack_hw_bp_module_init(void);
void khack_hw_bp_module_exit(void);

// 如果你的代码里有 PRINT_DEBUG，确保这里定义了
#ifndef PRINT_DEBUG
#define PRINT_DEBUG(fmt, ...) printk(KERN_INFO "khack: " fmt, ##__VA_ARGS__)
#endif

#endif // HW_BREAKPOINT_H
