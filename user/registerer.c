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
    char not_registered[16] = "attacker";
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
    if (ioctl(fd, IOCTL_REGISTER_PROG, prog_name) != 0) {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    sleep(3);

    bool check = false;

    //struct syscall_cr_struct
    struct check_syscall_cr test_sys = {
        .syscall_nr = syscall_nr,
        .check = false
    };

    struct check_uid_cr test_uid = {
        .uid = victim_uid,
        .check = false
    };

    struct check_progname_cr test_prog = {
        .check = false
    };

    strscpy(test_prog.name, prog_name, sizeof(prog_name));

    //check di system call
    printf("Check della system call :%d\n", syscall_nr);
    if (ioctl(fd, IOCTL_CHECK_SYSCALL, &test_sys) == 0) {
        if (test_sys.check) {
            printf("System call %d è registrata\n",syscall_nr);
        } else {
            printf("System call %d non è registrata\n", syscall_nr);
        }
    } else {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    //check di prog name
    printf("Check del program name: %s\n", prog_name);
    if (ioctl(fd, IOCTL_CHECK_PROG, &test_prog) == 0) {
        if (test_prog.check) {
            printf("Program name %s è registrato\n",prog_name);
        } else {
            printf("Program name %d non è registrato\n", prog_name);
        }
    } else {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    //check di prog name non registrato
    test_prog.check = false;
    strscpy(test_prog.name, not_registered, sizeof(not_registered));
    
    printf("Check del program name non registrato: %s\n", not_registered);
    if (ioctl(fd, IOCTL_CHECK_PROG, &test_prog) == 0) {
        if (test_prog.check) {
            printf("Program name %s è registrato\n",not_registered);
        } else {
            printf("Program name %d non è registrato\n", not_registered);
        }
    } else {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }
    

    //check di uid_t registrato
    printf("Check del uid non registrato: %d\n", victim_uid);
    if (ioctl(fd, IOCTL_CHECK_UID, &test_uid) == 0) {
        if (test_uid.check) {
            printf("Uid %s è registrato\n",victim_uid);
        } else {
            printf("Uid %d non è registrato\n", victim_uid);
        }
    } else {
        perror("Errore: ioctl fallita! Il Kernel ha rifiutato il comando");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return 0;
}