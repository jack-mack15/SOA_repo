#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/sched/signal.h>

#include "throttling_hidden.h"
#include "throttling_rcu.h"

struct timer_list epoch_timer;

int core_setup(void) {

	atomic64_set(&info_threads.start_time, jiffies);
	
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

	if (atomic_read(&is_monitor_active) == 0) return;

	int max = atomic_read(&max_syscalls_per_sec);

	//leggo il numero di thread attualemnte bloccati, lo sommo alla variabile cumulativa
	unsigned long curr_blocked = atomic64_read(&blocked_thread);
	
	//atomic64_add(curr_blocked,&(info_threads.sum_blocked));

	//se necessario aggiorno il picco
	if (curr_blocked > atomic_read(&info_threads.peak_blocked)) {
		atomic_set(&(info_threads.peak_blocked), curr_blocked);
	}

	//aggiornamento media esponenziale
	//moltiplico per 1024 (evito problemi valori bassi)
	curr_blocked = curr_blocked << 10;
	unsigned long temp = atomic64_read(&exponential);

	if (temp == 0) {
		//primo step
		atomic64_set(&exponential,curr_blocked);
	} else {
		//me = 3/4 me + threads/4
		//3/4 calcolati con shift right 
		temp = temp - (temp >> 2) + (curr_blocked >> 2);
		atomic64_set(&exponential,temp);
	}


	atomic_set(&curr_syscalls, max);

	//svegliare i thread. Per ora semplice così, probabile da migliorare
	wake_up_interruptible(&thrott_wq);
	
	//prossima scadenza +1 secondo
	mod_timer(&epoch_timer, jiffies + HZ);
}