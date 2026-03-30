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
    int c;
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
                while ((c = getchar()) != '\n' && c != EOF);

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
                while ((c = getchar()) != '\n' && c != EOF);

                if (ioctl(fd, IOCTL_DEREGISTER_PROG, str_val) < 0) {
                    if (errno == ENOENT) {
                        printf("Errore: programma %s non è registrato\n", str_val);
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else
                    printf("Programma %s deregistrato correttamente\n", str_val);
                break;

            case 7:
                printf("Inserire il nuovo limite massimo (Token): ");
                scanf("%d", &int_val);
                if (ioctl(fd, IOCTL_SET_MAX_CALLS, &int_val) < 0) {
                    if (errno == EINVAL) {
                        printf("Valore non valido per il nuovo massimo.\n");
                        printf("Valore Token deve essere intero positivo.\n");
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else printf("Valore Token impostato a %d.\n", int_val);
                break;

            case 8:
                if (ioctl(fd, IOCTL_MONITOR_ON) < 0) {
                    if (errno == EALREADY) {
                        printf("Errore: il monitor è già acceso\n");
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else printf("MONITOR ACCESO.\n");
                break;

            case 9:
                if (ioctl(fd, IOCTL_MONITOR_OFF) < 0) {
                    if (errno == EALREADY) {
                        printf("Errore: il monitor è già spento\n");
                        break;
                    }
                    perror("Errore comando IOCTL");
                }
                else printf("MONITOR SPENTO.\n");
                break;

            case 10: {
                struct thread_stats_cr_struct t_stats = {0};
                if (ioctl(fd, IOCTL_GET_THREAD_STATS, &t_stats) < 0) {
                    perror("Errore comando IOCTL");
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
                printf("Inserire System call interessata: ");
                scanf("%d", &s_stats.syscall_nr);
                if (ioctl(fd, IOCTL_GET_SYSCALL_STATS, &s_stats) < 0) {
                    if (errno == ENOENT) {
                        printf("System call %d non registrata\n", s_stats.syscall_nr);
                        break;
                    }
                    perror("Errore comando IOCTL");
                } else {
                    printf("\n--- STATISTICHE SYSCALL %d ---\n", s_stats.syscall_nr);
                    printf("Picco Delay: %lu msec\n", s_stats.peak_delay);
                    printf("Nome Prog Picco: %s\n", s_stats.peak_prog_name);
                    printf("UID Picco: %d\n", s_stats.peak_uid);
                }
                break;
            }

            case 12: {
                struct check_syscall_cr c_sys = {0};
                printf("Syscall da verificare: ");
                scanf("%d", &c_sys.syscall_nr);
                if (ioctl(fd, IOCTL_CHECK_SYSCALL, &c_sys) < 0) {
                    perror("Errore comando IOCTL");
                }
                else printf("System call %d: %s\n", c_sys.syscall_nr, c_sys.check ? "REGISTRATA" : "NON REGISTRATA");
                break;
            }

            case 13:
                struct check_uid_cr c_uid = {0};
                printf("UID da verificare: ");
                scanf("%d", &c_uid.uid);
                if (ioctl(fd, IOCTL_CHECK_UID, &c_uid) < 0) {
                    perror("Errore comando IOCTL");
                }
                else printf("UID %d: %s\n", c_uid.uid, c_uid.check ? "REGISTRATO" : "NON REGISTRATO");
                break;

            case 14:
                struct check_progname_cr c_prog = {0};
                printf("Programma da verificare: ");
                scanf("%15s", c_prog.name);
                while ((c = getchar()) != '\n' && c != EOF);
                if (ioctl(fd, IOCTL_CHECK_PROG, &c_prog) < 0) {
                    perror("Errore comando IOCTL");
                }
                else printf("Programma %s: %s\n", c_prog.name, c_prog.check ? "REGISTRATO" : "NON REGISTRATO");
                break; 

            case 15:
                printf("Inserire un intero dei seguenti 0-Syscalls, 1-UIDs, 2-Progs: ");
                scanf("%d", &int_val);
                if (int_val < 0 || int_val > 2) {
                    printf("Valore non valido per questo comando\n");
                    break;
                }
                if (ioctl(fd, IOCTL_GET_NUMBER, &int_val) < 0) perror("Errore IOCTL");
                else printf("Sono registrati %d elementi.\n", int_val);
                break;

            case 16:
                break;

            case 17: { 
                int count = 1; 
                if (ioctl(fd, IOCTL_GET_NUMBER, &count) < 0) {
                    perror("Errore comando IOCTL"); 
                    break;
                }
                
                if (count == 0) {
                    printf("Nessun UID registrato.\n"); 
                    break;
                }

                struct fetch_all_uids f_uids;
                uid_t *array_uids = malloc(count * sizeof(uid_t));
                f_uids.list = array_uids;
                f_uids.max = count;
                f_uids.copied = 0;

                if (ioctl(fd, IOCTL_GET_ALL_UIDS, &f_uids) < 0) {
                    perror("Errore Fetch");
                } else {
                    printf("\n--- UID REGISTRATI (%d) ---\n", f_uids.copied);
                    for (int i = 0; i < f_uids.copied; i++) {
                        printf("- UID: %d\n", array_uids[i]);
                    }
                }
                free(array_uids);
                break;
            }

            case 18:
                break;

            default:
                printf("Valore inserito non valido. Inserire un valore valido\n");
                break;
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}