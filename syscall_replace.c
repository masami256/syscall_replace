#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/syscalls.h>
#include <linux/mm.h>

MODULE_DESCRIPTION("system call replace test module");
MODULE_AUTHOR("masami256");
MODULE_LICENSE("GPL");

/* following string SYSCALL_TABLE_ADDRESS will be replaced by set_syscall_table_address.sh */
static void **syscall_table = (void *) __SYSCALL_TABLE_ADDRESS__;

asmlinkage long (*orig_sys_reboot)(int magic1, int magic2, unsigned int cmd, void __user *arg);

asmlinkage long syscall_replace_sys_reboot(int magic1, int magic2, unsigned int cmd, void __user *arg)
{
	panic("call original reboot system call\n");
	return orig_sys_reboot(magic1, magic2, cmd, arg);
}

static void save_original_syscall_address(void)
{
	orig_sys_reboot = syscall_table[__NR_reboot];
}

static void change_page_attr_to_rw(pte_t *pte)
{
	set_pte_atomic(pte, pte_mkwrite(*pte));
}

static void change_page_attr_to_ro(pte_t *pte)
{
	set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
}

static void replace_system_call(void *new)
{
	unsigned int level = 0;
	pte_t *pte;

	pte = lookup_address((unsigned long) syscall_table, &level);
	/* Need to set r/w to a page which syscall_table is in. */
	change_page_attr_to_rw(pte);

	syscall_table[__NR_reboot] = new;
	/* set back to read only */
	change_page_attr_to_ro(pte);
}

static int syscall_replace_init(void)
{
	pr_info("sys_call_table address is 0x%p\n", syscall_table);
	
	save_original_syscall_address();
	pr_info("original sys_reboot's address is %p\n", orig_sys_reboot);

	replace_system_call(syscall_replace_sys_reboot);

	pr_info("system call replaced\n");
	return 0;
}

static void syscall_replace_cleanup(void)
{
	pr_info("cleanup");
	if (orig_sys_reboot)
		replace_system_call(orig_sys_reboot);
}

module_init(syscall_replace_init);
module_exit(syscall_replace_cleanup);

