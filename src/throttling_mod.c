#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <asm/syscall.h>

#include "throttling.h"
#include "throttling_rcu.h"
#include "syscall_table_hack.h"
#include "throttling_dev.h"

#define MODULE_NAME "THROTTLING MOD"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maccari Gianluca");
MODULE_DESCRIPTION("A LKM Implementing a system call throttling mechanism");

LIST_HEAD(hacked_syscall_list);
LIST_HEAD(uid_list);
LIST_HEAD(prog_list);
int syscall_array[NR_syscalls] = {0};
DEFINE_SPINLOCK(write_lock);

//valori di default
bool is_monitor_active = false;
int max_syscalls_per_sec = 100;
int curr_syscalls = 0;

int init_module(void) {
	printk(KERN_INFO "%s: Module init...\n", MODULE_NAME);
	
	//system call hacking
	if (init_system_call_table() != 0) {
			printk(KERN_INFO "%s: Syscall table hacking failed\n", MODULE_NAME);
			return -1;
	}

	//init rcu
	return 0;

}

void clean_up_module(void) {
	printk(KERN_INFO "%s: Module cleanup...\n", MODULE_NAME);

	//clean up syscall table hacking
	cleanup_system_call_table();

}