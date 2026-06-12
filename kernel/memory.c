#include "memory.h"
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#endif

extern struct mm_struct *get_task_mm(struct task_struct *task);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61))
extern void mmput(struct mm_struct *);
#endif

phys_addr_t sysop_translate_linear_address(struct mm_struct *mm, uintptr_t va)
{
    pgd_t *pgd = pgd_offset(mm, va);
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    
    if (pgd_none(*pgd) || pgd_bad(*pgd)) return 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61))
    {
        p4d_t *p4d = p4d_offset(pgd, va);
        if (p4d_none(*p4d) || p4d_bad(*p4d)) return 0;
        pud = pud_offset(p4d, va);
    }
#else
    pud = pud_offset(pgd, va);
#endif

    if (pud_none(*pud) || pud_bad(*pud)) return 0;
    pmd = pmd_offset(pud, va);
    if (pmd_none(*pmd)) return 0;
    pte = pte_offset_kernel(pmd, va);
    if (pte_none(*pte) || !pte_present(*pte)) return 0;

    return (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT) + (va & (PAGE_SIZE - 1));
}

bool sysop_read_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct pid *pid_struct = find_get_pid(pid);
    size_t bytes_remaining = size;
    uintptr_t current_addr = addr;
    char __user *current_buffer = (char __user *)buffer;
    bool result = true;
    size_t chunk_size, boundary;
    phys_addr_t pa; // 变量定义移至函数头部

    if (!pid_struct) return false;
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task) return false;
    mm = get_task_mm(task);
    if (!mm) return false;

    while (bytes_remaining > 0)
    {
        boundary = PAGE_SIZE - (current_addr & (PAGE_SIZE - 1));
        chunk_size = (bytes_remaining < boundary) ? bytes_remaining : boundary;
        
        pa = sysop_translate_linear_address(mm, current_addr);
        if (!pa) {
            if (find_vma(mm, current_addr) && clear_user(current_buffer, chunk_size) == 0) {
            } else { result = false; break; }
        } else if (!sysop_read_physical_address(pa, current_buffer, chunk_size)) {
            result = false; break;
        }
        bytes_remaining -= chunk_size; 
        current_addr += chunk_size; 
        current_buffer += chunk_size;
    }
    mmput(mm);
    return result;
}

bool sysop_write_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct pid *pid_struct = find_get_pid(pid);
    size_t bytes_remaining = size;
    uintptr_t current_addr = addr;
    char __user *current_buffer = (char __user *)buffer;
    bool result = true;
    size_t chunk_size, boundary;
    phys_addr_t pa; // 变量定义移至函数头部

    if (!pid_struct) return false;
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task) return false;
    mm = get_task_mm(task);
    if (!mm) return false;

    while (bytes_remaining > 0)
    {
        boundary = PAGE_SIZE - (current_addr & (PAGE_SIZE - 1));
        chunk_size = (bytes_remaining < boundary) ? bytes_remaining : boundary;
        
        pa = sysop_translate_linear_address(mm, current_addr);
        if (!pa || !sysop_write_physical_address(pa, current_buffer, chunk_size)) {
            result = false; break;
        }
        bytes_remaining -= chunk_size; 
        current_addr += chunk_size; 
        current_buffer += chunk_size;
    }
    mmput(mm);
    return result;
}
