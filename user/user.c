//PROGRAMMA CHE DEVE ESSERE MONITORATO DAL MODULO

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <pthread.h>

#define NUM_THREADS 5
#define NUM_OPENS_PER_THREAD 30

//secondo test multithread
void *spam_open(void *arg) {
    int id = *((int *)arg);
    int test_fd;
    int i = 0;

    printf("[Thread %d] Inizio a spammare la system call 2 (open)...\n", id);

    while(i < NUM_OPENS_PER_THREAD) {
        // Accesso crudo alla sys_open
        test_fd = syscall(SYS_open, "test.txt", O_CREAT | O_RDWR, 0644);
        
        if (test_fd < 0) {
            perror("Errore nell'apertura del file di test");
        } else {
            // È importante chiudere solo se l'apertura è andata a buon fine!
            close(test_fd);
        }

        i++;
        
        // usleep(50000); // Toglilo dal commento se vuoi rallentarli un po'
    }

    printf("[Thread %d] Ho eseguito %d\n", id, i);

    printf("[Thread %d] Ho terminato le mie %d chiamate.\n", id, NUM_OPENS_PER_THREAD);
    return NULL;
}


int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    printf("Il mio User ID è %d\n", getuid());
    printf("Sto per lanciare %d thread...\n\n", NUM_THREADS);

    // creazione dei thread figli
    for(int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, spam_open, &thread_ids[i]) != 0) {
            perror("Errore nella creazione del thread");
            return EXIT_FAILURE;
        }
    }

    //semplice attesa dei figli
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return EXIT_SUCCESS;
}


//primo test single thread
/*int main() {
    printf("Sono il programma VITTIMA. Il mio PID è %d\n", getpid());
    printf("Il mio User ID è %d\n", getuid());
    printf("Inizio a spammare la system call 2 (open)...\n");

    int test_fd;
    int i = 0;

    while(i < 30) {
        //accesso alla sys open
        test_fd = syscall(SYS_open, "test.txt", O_CREAT | O_RDWR, 0644);
        
        if (test_fd < 0) {
            perror("Errore nell'apertura del file di test");
        }

        //usleep(50000);
        close(test_fd);
        i++;
    }

    return 0;
}*/