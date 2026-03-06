#ifndef THROTTLER_H
#define THROTTLER_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/rcupdate.h>

//uso tre strutture dati differenti per syscall number, user id e program names
//una struttura dati aggiuntiva per ricordarmi le system call hackerate

//struct per le system call hackerate
struct hacked_syscall {
    int syscall_nr;
    asmlinkage long (*original_syscall)(const struct pt_regs *);
    struct list_head list;
};

//struct per user id
struct registered_uid {
    uid_t uid;
    struct list_head list;
};

//struct per program names
struct registered_prog {
    char name[256];
    struct list_head list;
};

//le liste rcu
extern struct list_head hacked_syscall_list;
extern struct list_head uid_list;
extern struct list_head prog_list;
//array dei tutte le system call monitorate
extern int syscall_array[];

//per ora metto un lock solo per tutte le liste. meno lock e sicuro no deadlock.
//se ne metto 3 sicuro prestazioni migliori ma occhio deadlock.
//CHIEDERE AL PROF 
extern spinlock_t write_lock;

//variabile che indica se monitor è attivo o non (per switch on/off)
extern bool is_monitor_active;

//numero massimo di system call per secondo
extern int max_syscalls_per_sec;

//numero corrente (per epoca) di system call invocate
extern int curr_syscalls;

#endif // THROTTLER_H