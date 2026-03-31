#ifndef THROTTLER_H
#define THROTTLER_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/rcupdate.h>
#include <linux/types.h>

//uso tre strutture dati differenti per syscall number, user id e program names
//una struttura dati aggiuntiva per ricordarmi le system call hackerate

//struct per info della system call per statistiche
struct syscall_stats {
    int syscall_nr;
    unsigned long peak_delay;
    char peak_prog_name[16];
    uid_t peak_uid;
};

//struct per le system call hackerate
struct hacked_syscall {
    int syscall_nr;
    asmlinkage long (*original_syscall)(const struct pt_regs *);
    struct syscall_stats *stats;
    struct list_head list;
};

//struct per user id
struct registered_uid {
    uid_t uid;
    struct list_head list;
};

//struct per program names
struct registered_prog {
    char name[16];
    struct list_head list;
};

//struct per stats 
struct thread_stats {
    atomic64_t sum_blocked;
    atomic64_t elapsed;
    atomic64_t start_time;
    atomic_t peak_blocked;
};

//le liste rcu
extern struct list_head hacked_syscall_list;
extern struct list_head uid_list;
extern struct list_head prog_list;
//array dei tutte le system call monitorate
extern int syscall_array[];
//variabile per info thread
extern struct thread_stats info_threads;

//variabili che indicano quante entità sono registrate
extern atomic64_t uids_len;
extern atomic64_t prog_name_len;
extern atomic_t sys_len;

//per ora metto un lock solo per tutte le liste. meno lock e sicuro no deadlock.
//se ne metto 3 sicuro prestazioni migliori ma occhio deadlock.
//CHIEDERE AL PROF 
extern spinlock_t write_lock;
extern spinlock_t stats_lock;

//variabile che indica se monitor è attivo o non (per switch on/off)
extern atomic_t is_monitor_active;

//numero massimo di system call per secondo
extern atomic_t max_syscalls_per_sec;

//numero corrente (per epoca) di system call invocate
extern atomic_t curr_syscalls;

//numero di thread bloccati attualmente
extern atomic64_t blocked_thread;

#endif // THROTTLER_H