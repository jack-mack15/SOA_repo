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
    if (ioctl(fd, IOCTL_REGISTER_PROG, victim) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return 0;
}
