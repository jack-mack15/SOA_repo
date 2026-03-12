//PROGRAMMA CHE DEVE ESSERE MONITORATO DAL MODULO

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>

int main() {
    printf("Sono il programma VITTIMA. Il mio PID è %d\n", getpid());
    printf("Il mio User ID è %d\n", getuid());
    printf("Inizio a spammare la system call 2 (open)...\n");

    int test_fd;
    int i = 0;

    while(i < 30) {
        //accesso alla sys open
        test_fd = syscall(SYS_open, "test.txt", O_CREAT | O_RDWR, 0644);
        
        if (fd < 0) {
            perror("Errore nell'apertura del file di test");
        }

        //usleep(50000);
        close(test_fd);
        i++;
    }

    return 0;
}