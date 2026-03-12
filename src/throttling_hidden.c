#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/sched/signal.h>

#include "throttling_hidden.h"
#include "throttling_rcu.h"

struct timer_list epoch_timer;

int core_setup(void) {
	
	timer_setup(&epoch_timer,epoch_handler,0);

	//prima scadenza
	mod_timer(&epoch_timer, jiffies + HZ);

	return 0;
}

void core_cleanup(void) {
	timer_delete_sync(&epoch_timer);
	return;
}

void epoch_handler(struct timer_list *t){

	//si potrebbe rimuovere?
	if (atomic_read(&is_monitor_active) == 0) return;

	int max = atomic_read(&max_syscalls_per_sec);

	atomic_set(&curr_syscalls, max);

	//svegliare i thread. Per ora semplice così, probabile da migliorare
	wake_up_interruptible(&thrott_wq);

	printk(KERN_INFO "aggiorno il timer del modulo\n");
	//prossima scadenza +1 secondo
	mod_timer(&epoch_timer, jiffies + HZ);
}