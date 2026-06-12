#include <linux/kernel.h>
#include <linux/sched.h>

phys_addr_t sysop_translate_linear_address(struct mm_struct *mm, uintptr_t va);

bool sysop_read_physical_address(phys_addr_t pa, void *buffer, size_t size);

bool sysop_write_physical_address(phys_addr_t pa, void *buffer, size_t size);

bool sysop_read_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size);

bool sysop_write_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size);
