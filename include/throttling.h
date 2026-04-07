#ifndef THROTTLING_H
#define THROTTLING_H

#define THROTTLING_MAGIC 't'


/*Piccolo remainder:
*   _IOW: Il programma utente SCRIVE verso il Kernel (Write)
*   _IOR: Il programma utente LEGGE dal Kernel (Read)
*   _IO:  Nessun dato scambiato, switch del monitor (es. on/off)
*   _IOWR: Il programma utente SCRIVE verso il Kernel e Legge dal Kernel
*/

//strutture per il passaggio dei dati da kernel a user
struct thread_stats_cr_struct {
    int type;
    unsigned long mean;
    unsigned long sum_blocked;
    unsigned long elapsed;
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
    int check;
};

struct check_uid_cr {
    uid_t uid;
    int check;
};

struct check_progname_cr {
    char name[16];
    int check;
};

// struct per ottenere tutte le syscall, uidt o prog name
struct fetch_all_syscalls {
    int __user *list;     
    unsigned int max;       // max quello che si aspetta l'user
    unsigned int copied;    // copied quello che effettivamente restituisce il kernel
};

struct fetch_all_uids {
    uid_t __user *list;
    unsigned int max;
    unsigned int copied;
};

struct fetch_all_progs {
    char __user (*list)[TASK_COMM_LEN];
    unsigned int max;
    unsigned int copied;
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
#define IOCTL_GET_THREAD_STATS         _IOWR(THROTTLING_MAGIC, 10, struct thread_stats_cr_struct)
#define IOCTL_GET_SYSCALL_STATS        _IOWR(THROTTLING_MAGIC, 11, struct syscall_cr_struct)

//per verifica di ciò che è registrato, singolo
#define IOCTL_CHECK_SYSCALL  _IOWR(THROTTLING_MAGIC, 12, struct check_syscall_cr)           // check per syscall number 
#define IOCTL_CHECK_UID      _IOWR(THROTTLING_MAGIC, 13, struct check_uid_cr)           // check per UID utente
#define IOCTL_CHECK_PROG     _IOWR(THROTTLING_MAGIC, 14, struct check_progname_cr)           // check per nome programma
//stessa cosa ma totale
#define IOCTL_GET_NUMBER        _IOWR(THROTTLING_MAGIC, 15, int)     // per ottenere quanti user id, system call o program name sono registrati
#define IOCTL_GET_ALL_SYSCALLS   _IOWR(THROTTLING_MAGIC, 16, struct fetch_all_syscalls)     // per ottenere tutte le system call registrate
#define IOCTL_GET_ALL_UIDS       _IOWR(THROTTLING_MAGIC, 17, struct fetch_all_uids)     // per ottenere tutti gli uid_t registrati
#define IOCTL_GET_ALL_PROGS      _IOWR(THROTTLING_MAGIC, 18, struct fetch_all_progs)     // p er ottenere tutti i prog name registrati

#endif 