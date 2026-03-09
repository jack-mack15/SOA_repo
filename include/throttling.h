#ifndef THROTTLING_H
#define THROTTLING_H

//potrebbe servire?
//#include <linux/ioctl.h>

#define THROTTLING_MAGIC 't'


//struttura per lettura statistiche, vedere che manca
struct throttling_stats {
    int max_blocked_threads;
    int avg_blocked_threads;
};


/*Comandi per il modulo throttling.
*   _IOW: Il programma utente SCRIVE verso il Kernel (Write)
*   _IOR: Il programma utente LEGGE dal Kernel (Read)
*   _IO:  Nessun dato scambiato, switch del monitor (es. on/off)
*/

//registrazione
#define IOCTL_REGISTER_SYSCALL  _IOW(THROTTLING_MAGIC, 1, int)           // per syscall number 
#define IOCTL_REGISTER_UID      _IOW(THROTTLING_MAGIC, 2, uid_t)           // per UID utente
#define IOCTL_REGISTER_PROG     _IOW(THROTTLING_MAGIC, 3, char[256])     // per nome programma

//deregistrazuoe
#define IOCTL_DEREGISTER_SYSCALL  _IOW(THROTTLING_MAGIC, 4, int)           // per syscall number 
#define IOCTL_DEREGISTER_UID      _IOW(THROTTLING_MAGIC, 5, uid_t)           // per UID utente
#define IOCTL_DEREGISTER_PROG     _IOW(THROTTLING_MAGIC, 6, char[256])     // per nome programma

//limite
#define IOCTL_SET_MAX_CALLS     _IOW(THROTTLING_MAGIC, 7, int)           // per limite syscall

//switch on/off
#define IOCTL_MONITOR_ON        _IO(THROTTLING_MAGIC, 8)                 // per switch on monitor 
#define IOCTL_MONITOR_OFF       _IO(THROTTLING_MAGIC, 9)                 // per switch off monitor

//ottenere statistiche
//TODO maybe soezzare le statistiche?
#define IOCTL_GET_STATS         _IOR(THROTTLING_MAGIC, 10, struct throttling_stats)

//api
int register_system_call(const int syscall_numb);
int deregister_system_call(const int syscall_numb);

int register_user_id(const uid_t user_id);
int deregister_user_id(const uid_t user_id);

int register_prog_name(const char *prog_name);
int deregister_prog_name(const char *prog_name);

int switch_on_monitor(void);
int switch_off_monitor(void);

int set_max_syscall(const int new_max);

struct throttling_stats get_stats(void);

int hack_syscall(int sys_num);
int dishack_syscall(int sys_num);
long throttling_wrapper(const struct pt_regs *);

int cleanup_rcu(void);

#endif 