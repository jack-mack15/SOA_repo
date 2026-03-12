//FILE DA ESEGUIRE COME root PER REGISTRARE SYSTEM CALL NUMBER
//PROGRAM NAME E USER ID

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

    
    if (ioctl(fd, IOCTL_REGISTER_SYSCALL, &syscall_nr) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    //tentativo di registrare stessa syscall
    if (ioctl(fd, IOCTL_REGISTER_SYSCALL, &syscall_nr) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    //registrazione user id
    if (ioctl(fd, IOCTL_REGISTER_UID, &victim_uid) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    //registrazione program name
    if (ioctl(fd, IOCTL_REGISTER_PROG, victim) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return 0;
}