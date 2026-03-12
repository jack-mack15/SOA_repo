#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "throttling.h"

int main() {
    int fd;

    fd = open("/dev/throttling_device", O_RDWR);
    if (fd < 0) {
        perror("Errore: Impossibile aprire /dev/throttling_device");
        return EXIT_FAILURE;
    }

    int syscall_nr = 2;   // Es: sys_open

    printf("Inviando richiesta di throttling al Kernel...\n");

    
    if (ioctl(fd, IOCTL_DEREGISTER_SYSCALL, &syscall_nr) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Comando accettato con successo dal Kernel!\n");

    close(fd);
    return EXIT_SUCCESS;
}