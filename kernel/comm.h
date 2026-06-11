typedef struct _COPY_MEMORY
{
	pid_t pid;
	uintptr_t addr;
	void *buffer;
	size_t size;
} COPY_MEMORY, *PCOPY_MEMORY;

typedef struct _MODULE_BASE
{
	pid_t pid;
	char *name;
	uintptr_t base;
} MODULE_BASE, *PMODULE_BASE;

typedef struct {
    int action;             // 0: ADD, 1: REMOVE
    pid_t pid;
    uintptr_t addr;         // 断点触发地址 (Trigger)
    int len;
    int type;               // 触发类型
    
    // 新增：劫持动作参数
    int hijack_active;      // 设置为 1 开启劫持
    uintptr_t target_addr;  // 你真正想改的内存地址 (Target)
    unsigned long new_val;  // 你想写入的新值 (New Value)
} HW_BREAKPOINT_CTL, *PHW_BREAKPOINT_CTL;

enum OPERATIONS
{
	OP_INIT_KEY = 0x3000,
	OP_READ_MEM = 0x30001,
	OP_WRITE_MEM = 0x30002,
	OP_MODULE_BASE = 0x30003,
	OP_HW_BREAKPOINT_CTL = 0x30004,
};
