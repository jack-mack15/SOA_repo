#ifndef THROTTLING_H
#define THROTTLING_H

#include "throttling_rcu.h"

#define THROTTLING_MAGIC 't'


/*Piccolo remainder:
*   _IOW: Il programma utente SCRIVE verso il Kernel (Write)
*   _IOR: Il programma utente LEGGE dal Kernel (Read)
*   _IO:  Nessun dato scambiato, switch del monitor (es. on/off)
*/

//strutture per il passaggio dei dati da kernel a user
struct thread_stats_cr_struct {
    unsigned long sum_blocked;
    unsigned long start_time;
    unsigned long end_time;
    int peak_blocked;
};

struct syscall_cr_struct {
    int syscall_nr;
    unsigned long peak_delay;
    char peak_prog_name[16];
    uid_t peak_uid;
};

struct check_syscall_cr {
    int syscall_nr;
    bool check;
};

struct check_uid_cr {
    uid_t uid;
    bool check;
};

struct check_progname_cr {
    char name[16];
    bool check;
};

//registrazione
#define IOCTL_REGISTER_SYSCALL  _IOW(THROTTLING_MAGIC, 1, int)           // per syscall number 
#define IOCTL_REGISTER_UID      _IOW(THROTTLING_MAGIC, 2, uid_t)           // per UID utente
#define IOCTL_REGISTER_PROG     _IOW(THROTTLING_MAGIC, 3, char[16])     // per nome programma

//deregistrazuoe
#define IOCTL_DEREGISTER_SYSCALL  _IOW(THROTTLING_MAGIC, 4, int)           // per syscall number 
#define IOCTL_DEREGISTER_UID      _IOW(THROTTLING_MAGIC, 5, uid_t)           // per UID utente
#define IOCTL_DEREGISTER_PROG     _IOW(THROTTLING_MAGIC, 6, char[16])     // per nome programma

//limite
#define IOCTL_SET_MAX_CALLS     _IOW(THROTTLING_MAGIC, 7, int)           // per limite syscall

//switch on/off
#define IOCTL_MONITOR_ON        _IO(THROTTLING_MAGIC, 8)                 // per switch on monitor 
#define IOCTL_MONITOR_OFF       _IO(THROTTLING_MAGIC, 9)                 // per switch off monitor

//ottenere statistiche
#define IOCTL_GET_THREAD_STATS         _IOR(THROTTLING_MAGIC, 10, struct thread_stats_cr_struct)
#define IOCTL_GET_SYSCALL_STATS         _IOWR(THROTTLING_MAGIC, 11, struct syscall_cr_struct)

//per verifica di ciò che è registrato
#define IOCTL_CHECK_SYSCALL  _IOWR(THROTTLING_MAGIC, 12, struct check_syscall_cr)           // check per syscall number 
#define IOCTL_CHECK_UID      _IOWR(THROTTLING_MAGIC, 13, struct check_uid_cr)           // check per UID utente
#define IOCTL_CHECK_PROG     _IOWR(THROTTLING_MAGIC, 14, struct check_progname_cr)           // check per nome programma

#endif 