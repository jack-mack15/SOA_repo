#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>

#ifndef __user
#define __user
#endif

#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#endif

#include "throttling.h"

#define DEVICE_PATH "/dev/throttling_device"

void print_menu() {
    printf("\n=======================================================\n");
    printf("      THROTTLING MODULE INTERFACE\n");
    printf("=======================================================\n");
    printf("  1. Registra Syscall     4. Deregistra Syscall\n");
    printf("  2. Registra UID         5. Deregistra UID\n");
    printf("  3. Registra Programma   6. Deregistra Programma\n");
    printf("-------------------------------------------------------\n");
    printf("  7. Imposta Limite Token per Syscall\n");
    printf("  8. Accendi Monitor      9. Spegni Monitor\n");
    printf("-------------------------------------------------------\n");
    printf(" 10. Statistiche Globali Thread\n");
    printf(" 11. Statistiche Singola Syscall\n");
    printf("-------------------------------------------------------\n");
    printf(" 12. Verifica Syscall    15. Ottieni Numero Registrati\n");
    printf(" 13. Verifica UID        16. Ottieni tutte le Syscall\n");
    printf(" 14. Verifica Programma  17. Ottieni tutti gli UID\n");
    printf("                         18. Ottieni tutti i Programmi\n");
    printf("-------------------------------------------------------\n");
    printf("  0. Esci\n");
    printf("=======================================================\n");
    printf("Scelta: ");
}

int main() {
    //per gestire device
    int fd;
    //per la scelta
    int choice;
    int int_val;
    uid_t uid_val;
    char str_val[16];

    // Apertura del device file
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Errore apertura del device");
        return EXIT_FAILURE;
    }

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); 
            continue;
        }

        if (choice == 0) {
            printf("Chiusura Interfaccia\n");
            break;
        }

        switch (choice) {

            case 1:
                printf("Inserire codice syscall: ");
                scanf("%d", &int_val);
                if (int_val < 0) {
                    perror("Valore non valido per il comando\n");
                    break;
                }
                if (ioctl(fd, IOCTL_REGISTER_SYSCALL, &int_val) < 0) {
                    if (errno == EEXIST) {
                        printf("Errore: La System Call %d è già registrata\n", int_val);
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else 
                    printf("Syscall number %d registrata correttamente\n",int_val);
                break;

            case 4:
                printf("Inserire codice syscall: ");
                scanf("%d", &int_val);
                if (int_val < 0) {
                    perror("Valore non valido per il comando\n");
                    break;
                }
                if (ioctl(fd, IOCTL_DEREGISTER_SYSCALL, &int_val) < 0) {
                    if (errno == ENOENT) {
                        printf("Errore: La System Call %d non è registrata\n", int_val);
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else 
                    printf("Syscall number %d deregistrata correttamente\n",int_val);
                break;

            case 2:
                printf("Inserire UID: ");
                scanf("%d", &uid_val);
                if (int_val < 0) {
                    perror("Valore non valido per il comando\n");
                    break;
                }
                if (ioctl(fd, IOCTL_REGISTER_UID, &uid_val) < 0) {
                    if (errno == EEXIST) {
                        printf("Errore: UID %d è già registrato\n", uid_val);
                        break;
                    }
                    perror("Errore comando IOCTL");

                }
                else
                    printf("UID %u registrato correttamente\n",uid_val);
                break;
            case 5:
                printf("Inserire UID: ");
                scanf("%d", &uid_val);
                if (int_val < 0) {
                    perror("Valore non valido per il comando\n");
                    break;
                }
                if (ioctl(fd, IOCTL_DEREGISTER_UID, &uid_val) < 0) {
                    if (errno == ENOENT) {
                        printf("Errore: UID %d non è registrato\n", uid_val);
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else
                    printf("UID %u deregistrato correttamente\n",uid_val);
                break;

            case 3:
                printf("Inserire il nome del programma: ");
                scanf("%15s", str_val);
                if (ioctl(fd, IOCTL_REGISTER_PROG, str_val) < 0) {
                    if (errno == EEXIST) {
                        printf("Errore: programma %s è già registrato\n", str_val);
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else
                    printf("Programma %s registrato correttamente\n", str_val);
                break;
            case 6:
                printf("Inserire il nome del programma: ");
                scanf("%15s", str_val);
                if (ioctl(fd, IOCTL_DEREGISTER_PROG, str_val) < 0) {
                    if (errno == ENOENT) {
                        printf("Errore: programma %s è già registrato\n", str_val);
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else
                    printf("Programma %s registrato correttamente\n", str_val);
                break;

            case 7:
                printf("Inserire il nuovo limite massimo (Token): ");
                scanf("%d", &int_val);
                if (ioctl(fd, IOCTL_SET_MAX_CALLS, &int_val) < 0) perror("Errore IOCTL");
                else printf("Valore Token impostato a %d.\n", int_val);
                break;

            case 8:
                if (ioctl(fd, IOCTL_MONITOR_ON) < 0) perror("Errore IOCTL");
                else printf("MONITOR ACCESO.\n");
                break;

            case 9:
                if (ioctl(fd, IOCTL_MONITOR_OFF) < 0) perror("Errore IOCTL");
                else printf("MONITOR SPENTO.\n");
                break;

            case 10: {
                struct thread_stats_cr_struct t_stats = {0};
                if (ioctl(fd, IOCTL_GET_THREAD_STATS, &t_stats) < 0) {
                    perror("Errore IOCTL");
                } else {
                    printf("\n--- STATISTICHE THREAD ---\n");
                    printf("Thread bloccati totali: %lu\n", t_stats.sum_blocked);
                    printf("Media thread bloccati: %.5f thread/s\n", (double)t_stats.sum_blocked / (double)t_stats.elapsed);
                    printf("Picco thread bloccati: %d\n", t_stats.peak_blocked);
                }
                break;
            }

            case 11: {
                struct syscall_cr_struct s_stats = {0};
                printf("Di quale Syscall vuoi le statistiche? ");
                scanf("%d", &s_stats.syscall_nr);
                if (ioctl(fd, IOCTL_GET_SYSCALL_STATS, &s_stats) < 0) {
                    perror("Errore IOCTL");
                } else {
                    printf("\n--- STATISTICHE SYSCALL %d ---\n", s_stats.syscall_nr);
                    printf("Picco Delay: %lu\n", s_stats.peak_delay);
                    printf("Nome Prog Picco: %s\n", s_stats.peak_prog_name);
                    printf("UID Picco: %d\n", s_stats.peak_uid);
                }
                break;
            }

            case 12: {
                struct check_syscall_cr c_sys = {0};
                printf("Syscall da verificare: ");
                scanf("%d", &c_sys.syscall_nr);
                if (ioctl(fd, IOCTL_CHECK_SYSCALL, &c_sys) < 0) perror("Errore IOCTL");
                else printf("Esito verifica: %s (Codice %d)\n", c_sys.check ? "REGISTRATA" : "NON REGISTRATA", c_sys.check);
                break;
            }
            // ... (i case 13 e 14 sono concettualmente identici al 12, omessi per brevità, ma puoi copiarli usando check_uid_cr e check_progname_cr!)


            case 15:
                printf("Inserire: 0=Syscalls, 1=UIDs, 2=Progs\n");
                scanf("%d", &int_val);
                if (int_val < 0 || int_val > 2) {
                    printf("Valore non valido per questo comando\n");
                    break;
                }
                if (ioctl(fd, IOCTL_GET_NUMBER, &int_val) < 0) perror("Errore IOCTL");
                else printf("Il Kernel ha risposto: ci sono %d elementi.\n", int_val);
                break;

            case 17: { // ESEMPIO: FETCH DEGLI UID
                // 1. Chiedo quanti sono
                int target = 2; // Supponiamo 2 significhi UID per il tuo IOCTL_GET_NUMBER
                int count = target; 
                if (ioctl(fd, IOCTL_GET_NUMBER, &count) < 0) {
                    perror("Errore conteggio"); break;
                }
                
                if (count == 0) {
                    printf("Nessun UID registrato.\n"); break;
                }

                // 2. Alloco e preparo la struct
                struct fetch_all_uids f_uids;
                uid_t *array_uids = malloc(count * sizeof(uid_t));
                f_uids.list = array_uids;
                f_uids.max = count;
                f_uids.copied = 0;

                // 3. Eseguo il fetch
                if (ioctl(fd, IOCTL_GET_ALL_UIDS, &f_uids) < 0) {
                    perror("Errore Fetch");
                } else {
                    printf("\n--- UID REGISTRATI (%d) ---\n", f_uids.copied);
                    for (unsigned int i = 0; i < f_uids.copied; i++) {
                        printf("- UID: %d\n", array_uids[i]);
                    }
                }
                free(array_uids);
                break;
            }

            // Puoi replicare il Case 17 per i Case 16 (Syscalls) e 18 (Programmi) cambiando i tipi!

            default:
                printf("Scelta non valida!\n");
                break;
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}