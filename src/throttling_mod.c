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
#include "throttling_hidden.h"
#include "throttling_api.h"

#define MODULE_NAME "THROTTLING MOD"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maccari Gianluca");
MODULE_DESCRIPTION("A LKM Implementing a system call throttling mechanism");

//definizione variabili
LIST_HEAD(hacked_syscall_list);
LIST_HEAD(uid_list);
LIST_HEAD(prog_list);
DEFINE_SPINLOCK(write_lock);
DEFINE_SPINLOCK(stats_lock);
DECLARE_WAIT_QUEUE_HEAD(thrott_wq);

int syscall_array[NR_syscalls] = {0};
atomic_t is_monitor_active = ATOMIC_INIT(1);
atomic_t max_syscalls_per_sec = ATOMIC_INIT(3);
atomic_t curr_syscalls = ATOMIC_INIT(3);
atomic64_t blocked_thread = ATOMIC_INIT(0);

struct thread_stats info_threads = {
    .peak_blocked = ATOMIC_INIT(0),
    .sum_blocked = ATOMIC_INIT(0),
    .start_time = ATOMIC_INIT(0)
};

int init_module(void) {
	printk(KERN_INFO "%s: Module init...\n", MODULE_NAME);
	
	//system call hacking
	if (init_system_call_table() != 0) {
			printk(KERN_INFO "%s: Syscall table hacking failed\n", MODULE_NAME);
			return -1;
	}

	//init del device
	if (dev_init() != 0) {
		printk(KERN_INFO "%s: Init device driver for throttling failed\n", MODULE_NAME);
		return -1;
	}

	//init del timer 
	if (core_setup() != 0) {
		printk(KERN_INFO "%s: Init core for throttling failed\n", MODULE_NAME);
		return -1;
	}

	return 0;

}

void cleanup_module(void) {
	printk(KERN_INFO "%s: cleanup system call table\n", MODULE_NAME);

	//clean up syscall table hacking
	cleanup_system_call_table();

	printk(KERN_INFO "%s: cleanup dev cleanup\n", MODULE_NAME);
	//clean up dev
	dev_cleanup();

	printk(KERN_INFO "%s: cleanup rcu cleanup\n", MODULE_NAME);
	//clean up rcu e rimozione timer
	cleanup_rcu();
}