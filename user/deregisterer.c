//FILE DA ESEGUIRE PER DEREGISTRARE QUELLO FATTO CON IL registerer.c

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include "throttling.h"


int main() {

	int fd;
    char prog_name[16] = "victim";
    uid_t victim_uid = 1000;    //da sostituire con quello corretto

    fd = open("/dev/throttling_device", O_RDWR);
    if (fd < 0) {
        perror("Errore: Impossibile aprire /dev/throttling_device");
        return EXIT_FAILURE;
    }

    //registrazione syscall
    int syscall_nr = 2;   //dovrebbe essere sys_open (UNISTD_64)

    //test info threads
    struct thread_stats_cr_struct test_stats = {
        .sum_blocked = 0,
        .elapsed = 0,
        .peak_blocked = 0
    };

    if (ioctl(fd, IOCTL_GET_THREAD_STATS, &test_stats) == 0) {
        printf("picco di thread bloccati: %d\n", test_stats.peak_blocked);
        printf("media dei thread bloccanti: %lu\n", test_stats.sum_blocked / test_stats.elapsed);
    } else {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando\n");
        close(fd);
        return EXIT_FAILURE;
    }

    struct syscall_cr_struct test_sys = {
        .syscall_nr = 2
    };

    //test info syscall
    if (ioctl(fd, IOCTL_GET_SYSCALL_STATS, &test_sys) == 0) {
        printf("picco del tempo di blocco system call: %lu\n", test_sys.peak_delay);
        printf("il picco subito da prog name: %s\n", test_sys.peak_prog_name);
        printf("il picco subito da uid: %d\n", test_sys.peak_uid);
    } else {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    struct syscall_cr_struct test_sys2 = {
        .syscall_nr = 3
    };

    //test info syscall non esistente
    if (ioctl(fd, IOCTL_GET_SYSCALL_STATS, &test_sys2) == 0) {
        printf("picco del tempo di blocco system call: %lu\n", test_sys2.peak_delay);
        printf("il picco subito da prog name: %s\n", test_sys2.peak_prog_name);
        printf("il picco subito da uid: %d\n", test_sys2.peak_uid);
    } else {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

	//deregistrare syscall
    if (ioctl(fd, IOCTL_DEREGISTER_SYSCALL, &syscall_nr) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    //deregistrazione user id
    if (ioctl(fd, IOCTL_REGISTER_UID, &victim_uid) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    //deregistrazione program name
    if (ioctl(fd, IOCTL_REGISTER_PROG, prog_name) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return 0;
}
