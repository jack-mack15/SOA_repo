#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/syscall.h>
#include <linux/nospec.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/rcupdate.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/minmax.h>
#include <linux/jhash.h>

#include "throttling.h"
#include "throttling_rcu.h"
#include "syscall_table_hack.h"
#include "throttling_hidden.h"
#include "throttling_api.h"

#define MODULE_NAME "THROTTLING MOD"

void check_and_set_statistic(unsigned long delay, int original_sysnum);

//api che registra system call number
int register_system_call(const int sys_num) {

    //alloco fuori dai lock le struct
    struct syscall_stats *stats = kmalloc(sizeof(struct syscall_stats), GFP_KERNEL);
    if (!stats) {
        printk(KERN_ERR "Throttling module: kmalloc error in hack_syscall\n");
        return -ENOMEM;
    }
    struct hacked_syscall *new_hack = kmalloc(sizeof(struct hacked_syscall), GFP_KERNEL);
    if (!new_hack) {
        kfree(stats);
        printk(KERN_ERR "Throttling module: kmalloc error in hack_syscall\n");
        return -ENOMEM;
    }

    //sanitizzazione, il controllo fatto prima
    int safe_nr = array_index_nospec(sys_num, NR_syscalls);

    //riempo alcuni campi delle struct
    stats->syscall_nr = safe_nr;
    stats->peak_delay = 0;
    stats->peak_uid = 0;
    new_hack->stats = stats;

    //prendo lock in scrittura, nessuno deve scrivere oltre me
    spin_lock(&write_lock);

    //controllo se già presente e in caso annullo tutto
    if(syscall_array[safe_nr]) {
        spin_unlock(&write_lock);
        kfree(stats);
        kfree(new_hack);
        printk(KERN_ERR "Throttling module: syscall %d already hacked\n",safe_nr);
        return -EEXIST;
    }

    begin_syscall_table_hack();

    new_hack->syscall_nr = safe_nr;
    
    //inserimento wrapper e salvataggio vecchia syscall table entry
    new_hack-> original_syscall = (void *)hacked_syscall_tbl[safe_nr];
    hacked_syscall_tbl[safe_nr] = (unsigned long *)throttling_wrapper;

    //aggiorno array system call sotto osservazione
    syscall_array[safe_nr] = 1;

    end_syscall_table_hack();

    //aggiunta nuova struct per il recupero
    hash_add_rcu(hacked_syscall_hash, &new_hack->hlist, new_hack->syscall_nr);
    atomic_inc_return(&sys_len);
    spin_unlock(&write_lock);

    return 0;
}

//api che deregistra system call number
int deregister_system_call(const int sys_num){
    struct hacked_syscall *to_remove = NULL;
    struct hacked_syscall *entry;

    //sanitizzazione, il controllo fatto prima
    int safe_nr = array_index_nospec(sys_num, NR_syscalls);

    spin_lock(&write_lock);

    if(!syscall_array[safe_nr]) {
        spin_unlock(&write_lock);
        printk(KERN_ERR "Throttling module: syscall %d not hacked, deregistration did nothing\n",safe_nr);
        return -ENOENT;
    }

    hash_for_each_possible_rcu(hacked_syscall_hash, entry, hlist, safe_nr) {
        if (entry->syscall_nr == safe_nr) {
            to_remove = entry;
            break;
        }
    }

    if(!to_remove) {
        //se entro qua significa che la system call viene indicata come hackerata ma non ho trovato 
        //la struct che mantiene info della originale system call
        spin_unlock(&write_lock);
        printk(KERN_ERR "Throttling module: syscall %d hacked but not found, something went wrong\n",safe_nr);
        return -EFAULT;
    }

    begin_syscall_table_hack();

    //inserimento wrapper e salvataggio vecchia syscall table entry
    hacked_syscall_tbl[safe_nr] = (unsigned long *)to_remove->original_syscall;
    
    end_syscall_table_hack();

    //segnalo system call come non hackerata
    syscall_array[safe_nr] = 0;
    
    //rimozione del nodo di recupero dalla lista
    hash_del_rcu(&to_remove->hlist);

    atomic_dec_return(&sys_len);

    spin_unlock(&write_lock);

    synchronize_rcu();
    
    kfree(to_remove->stats);
    kfree(to_remove);

    return 0;
}

//api che registra user id
int register_user_id(const uid_t user_id){
	struct registered_uid *curr;
	struct registered_uid *new_uid;
	bool exists = false;

    new_uid = kmalloc(sizeof(struct registered_uid), GFP_KERNEL);
    if (!new_uid) {
        printk(KERN_ERR "Throttling module: error in kmalloc registering new uid_t\n");
        return -ENOMEM;
    }

    new_uid->uid = user_id;

    spin_lock(&write_lock);

    //verifica se già è registrato
    hash_for_each_possible_rcu(uid_hash, curr, hlist, user_id) {
        if (curr->uid == user_id) {
            exists = true;
            break;
        }
    }
    if (exists) {
    	spin_unlock(&write_lock);
        kfree(new_uid);
    	printk(KERN_INFO "Throttling module: UID %u already registered\n", user_id);
    	return -EEXIST;
    }
    
    //se non è registrato
    hash_add_rcu(uid_hash, &new_uid->hlist, new_uid->uid);
    atomic64_inc_return(&uids_len);
    spin_unlock(&write_lock);

    printk(KERN_INFO "Throttling module: UID %u registered\n", user_id);
    
    return 0;
}


//api che deregistra user id
int deregister_user_id(const uid_t user_id){
    struct registered_uid *curr;
    struct registered_uid *to_remove = NULL;

    spin_lock(&write_lock);

    //ricerca e sgancio
    hash_for_each_possible_rcu(uid_hash, curr, hlist, user_id) {
        if (curr->uid == user_id) {
            //sgancio dalla lista (primo step)
            hash_del_rcu(&curr->hlist);
            atomic64_dec_return(&uids_len);
            to_remove = curr;
            break;
        }
    }

    spin_unlock(&write_lock);

    if (to_remove) {
        //attesa lettori
        synchronize_rcu();
        //secondo step, rimozione
        kfree(to_remove);
        printk(KERN_INFO "Throttling module: UID %u deregistered\n", user_id);
        return 0;

    }

    //se non trovo l'uid
    printk(KERN_INFO "Throttling module: UID %u not registered, deregistration did nothing\n", user_id);
    return -ENOENT; 
}



//api che registra il program name specificato
int register_prog_name(const char *prog_name){
    struct registered_prog *curr;
    struct registered_prog *new_prog;
    bool exists = false;

    u32 hash_value;

    new_prog = kmalloc(sizeof(struct registered_prog), GFP_KERNEL);
    if (!new_prog) {
        printk(KERN_ERR "Throttling module: kmalloc error in register_prog_name\n");
        return -ENOMEM;
    }

    //copio i nomi
    strncpy(new_prog->name, prog_name, sizeof(new_prog->name) - 1);
    new_prog->name[sizeof(new_prog->name) - 1] = '\0';

    //valore hash per la hash table
    hash_value = jhash(new_prog->name, strnlen(new_prog->name, TASK_COMM_LEN), 0);

    spin_lock(&write_lock);

    hash_for_each_possible_rcu(prog_hash, curr, hlist, hash_value) {

        if (strncmp(curr->name, prog_name, sizeof(curr->name)) == 0) {
            exists = true;
            break;
        }
    }

    if (exists) {
        spin_unlock(&write_lock);
        kfree(new_prog);
        printk(KERN_INFO "Throttling module: prog name '%s' already registered\n", prog_name);
        return -EEXIST;
    }
    
    hash_add_rcu(prog_hash, &new_prog->hlist, hash_value);
    atomic64_inc_return(&prog_name_len);
    spin_unlock(&write_lock);

    printk(KERN_INFO "Throttling module: prog name '%s' registered\n", prog_name);
    return 0;
}


//api che deregistra il program name specificato
int deregister_prog_name(const char *prog_name){
	struct registered_prog *curr;
    struct registered_prog *to_delete = NULL;
    u32 hash_value;

    //valore hash per ricerca
    hash_value = jhash(prog_name, strnlen(prog_name, TASK_COMM_LEN), 0);

    spin_lock(&write_lock);

    hash_for_each_possible_rcu(prog_hash, curr, hlist, hash_value) {
        if (strncmp(curr->name, prog_name, sizeof(curr->name)) == 0) {
            
            hash_del_rcu(&curr->hlist); 
            atomic64_dec_return(&prog_name_len);
            to_delete = curr;
            break;
        }
    }

    spin_unlock(&write_lock);

    if (to_delete) {
        synchronize_rcu(); 
        kfree(to_delete);
        printk(KERN_INFO "Throttling module: prog name '%s' deregistered\n", prog_name);
        return 0;
    }

    printk(KERN_INFO "Throttling module: prog name '%s' not registered, deregistration did nothing\n", prog_name);
    return -ENOENT;
}


//api che accende il monitor se è spento (altrimenti non fa nulla)
int switch_on_monitor(void){
    //se il prof volesse che riattivo tutte le system call, allora non prendo lock e chiamo register_system_call
    //su tutte le system call prese da un'altra lista
    
    if (atomic_cmpxchg(&is_monitor_active, 0, 1) == 0) {
        //printk(KERN_INFO "Throttling module: monitor switched on\n");

        //riattivo il timer della waitqueue
        core_setup();
        return 0;
    }

    //se è già acceso
    printk(KERN_INFO "Throttling module: monitor already on\n");
    return -EALREADY;
}

//api che spegne il monitor se è acceso (altrimenti non fa nnulla)
int switch_off_monitor(void){
    //se il prof volesse che disattivo tutte le system call, allora non prendo lock e chiamo deregister_system_call
    //su tutte le system call prese da un'altra lista

    if (atomic_cmpxchg(&is_monitor_active, 1, 0) == 1) {
        //printk(KERN_INFO "Throttling module: monitor switched off\n");

        //sicuro devo svegliare i thread che sono in attesa 
        wake_up_all(&thrott_wq);

        //aggiusto i tempi per statistiche threads
        unsigned long temp = jiffies - atomic64_read(&info_threads.start_time);
        atomic64_set(&info_threads.elapsed,temp);

        //rimuovo il timer del gestore della wait queue
        core_cleanup();

        return 0;
    }

    //se il monitor è già spento
    printk(KERN_INFO "Throttling module: monitor already turned off\n");
    return -EALREADY;
}


//api che imposta il numero massimo di system call per secondo
int set_max_syscall(int new_max){

    if (new_max <= 0) {
        printk(KERN_INFO "Throttling module: new max value must be greater than 0, u passed: %d\n",new_max);
        return -EINVAL;
    }

    int old_max = atomic_xchg(&max_syscalls_per_sec, new_max);
	
    //print del vecchio valore può servire??
    printk(KERN_INFO "Throttling module: new max value: %d, old value: %d\n",new_max,old_max);
    
    return 0;
}


//api che ritorna le statistiche (0 media aritmetica, 1 media esponenziale)
struct thread_stats_cr_struct *get_thread_stats(int type){
    
    struct thread_stats_cr_struct *to_ret = kmalloc(sizeof(struct thread_stats_cr_struct), GFP_KERNEL);
    if(!to_ret) {
        printk(KERN_ERR "Throttling module: kmalloc error in get_thread_stats\n");
        return ERR_PTR(-ENOMEM);
    }

    if (type == 1) {
        to_ret->elapsed = 0;
        to_ret->sum_blocked = 0;
        to_ret->peak_blocked = 0;
        to_ret->type = 1;
        to_ret->mean = atomic64_read(&exponential);

        return to_ret;
    }

    //questi sono gli intervalli di accensione precedenti (se ce ne fossero)
    unsigned long temp = atomic64_read(&(info_threads.elapsed));
    //se serve sommo anche l'intervallo di accensione attuale
    if (atomic_read(&is_monitor_active) != 0) {
        temp += jiffies - atomic64_read(&(info_threads.start_time));
    }

    to_ret->sum_blocked = atomic64_read(&(info_threads.sum_blocked));
    to_ret->elapsed = jiffies_to_msecs(temp);
    to_ret->peak_blocked = atomic_read(&(info_threads.peak_blocked));
    to_ret->type = 0;
    to_ret->mean = 0;
    return to_ret;
}

struct syscall_cr_struct *get_syscall_stats(int sys_num) {

    struct hacked_syscall *entry;
    struct hacked_syscall *curr = NULL;
    struct syscall_cr_struct *to_ret;

    to_ret = kmalloc(sizeof(struct syscall_cr_struct), GFP_KERNEL);
    if(!to_ret) {
        printk(KERN_ERR "Throttling module: kmalloc error in get_syscall_stats\n");
        return ERR_PTR(-ENOMEM);
    }

    //sanitizzazione, il controllo fatto prima
    int safe_nr = array_index_nospec(sys_num, NR_syscalls);

    rcu_read_lock();

    hash_for_each_possible_rcu(hacked_syscall_hash, entry, hlist, safe_nr) {
        if (entry->syscall_nr == safe_nr) {
            curr = entry;
            break;
        }
    }

    if(!curr) {
        rcu_read_unlock();
        printk(KERN_ERR "Throttling module: syscall %d not hacked, no stats available\n",safe_nr);
        kfree(to_ret);
        return ERR_PTR(-ENOENT);
    }

    to_ret->syscall_nr = curr->stats->syscall_nr;
    to_ret->peak_delay = jiffies_to_msecs(curr->stats->peak_delay);
    to_ret->peak_uid = curr->stats->peak_uid;
    strscpy(to_ret->peak_prog_name, curr->stats->peak_prog_name, sizeof(to_ret->peak_prog_name));

    rcu_read_unlock();

    return to_ret;
}

//funzioni per verificare se system call, program name e user id sono stati registrati
int check_syscall(const int sys_num){
    int to_ret = 0;

    //sanitizzazione, il controllo fatto prima
    int safe_nr = array_index_nospec(sys_num, NR_syscalls);
    
    rcu_read_lock();
    if(syscall_array[safe_nr]) {
        to_ret = 1;
    }
    rcu_read_unlock();

    return to_ret;
}

int check_uid(const uid_t user_id){
    int to_ret = 0;
    struct registered_uid *entry;
    
    rcu_read_lock();

    hash_for_each_possible_rcu(uid_hash, entry, hlist, user_id) {
        if (entry->uid == user_id) {
            to_ret = 1;
            break;
        }
    }

    rcu_read_unlock();

    return to_ret;
}

int check_progname(const char *prog_name){
    int to_ret = 0;
    struct registered_prog *entry;
    u32 hash_value;

    hash_value = jhash(prog_name, strnlen(prog_name, TASK_COMM_LEN), 0);
    
    rcu_read_lock();

    hash_for_each_possible_rcu(prog_hash, entry, hlist, hash_value) {
        if (strncmp(entry->name, prog_name, sizeof(entry->name)) == 0) {
            to_ret = 1;
            break;
        }
    }

    rcu_read_unlock();

    return to_ret;
}

int get_lenght(const int choose) {
    
    //recupero numero system call hackerate
    if (choose == 0) {
        return atomic64_read(&uids_len);
      //recupero numero uid registrati
    } else if (choose == 1) {
        return atomic_read(&sys_len);
      //recupero numero program name registrati
    } else {
        return atomic64_read(&prog_name_len);
    }

    return -1;
}

int get_all_syscalls(int in_max, int **out_sys) {
    int copied = 0;
    int i;
    int *temp_buf;

    in_max = min(atomic_read(&sys_len), in_max);
    temp_buf = kmalloc(in_max * sizeof(int), GFP_KERNEL);
    if (!temp_buf) {
        printk(KERN_ERR "Throttling module: kmalloc error in get_all_syscalls\n");
        return -ENOMEM;
    }

    rcu_read_lock();

    for (i = 0; i < NR_syscalls; i++) {
        if (copied >= in_max) {
            break;
        }
        if (syscall_array[i]) {
            temp_buf[copied] = i;
            copied++;
        }
    }

    rcu_read_unlock();
    printk(KERN_ERR "Throttling module: copied %d syscall\n", copied);
    *out_sys = temp_buf;

    return copied;
}

//riceve il numero massimo di uid che si desiderano
int get_all_uids(int in_max, uid_t **out_uids) {
    struct registered_uid *entry;
    uid_t *temp_buf;
    int copied = 0;
    int curr = 0;

    //prendo il minimo tra i due
    in_max = min(atomic64_read(&uids_len),in_max);

    temp_buf = kmalloc(in_max * sizeof(uid_t), GFP_KERNEL);
    if (!temp_buf) {
        printk(KERN_ERR "Throttling module: kmalloc error in get_all_uids\n");
        return -ENOMEM;
    }

    rcu_read_lock();

    hash_for_each(uid_hash, curr, entry, hlist) {
        
        if (copied < in_max) {
            temp_buf[copied] = entry->uid;
            copied++;        
        } else {
            break;
        }
    }

    rcu_read_unlock();

    *out_uids = temp_buf;

    return copied;
}

int get_all_progs(int in_max, char (**out_buf)[TASK_COMM_LEN]) {
    char (*temp_buf)[TASK_COMM_LEN];
    struct registered_prog *entry_prog;
    int copied = 0;
    int curr;

    //prendo il minimo tra i due
    in_max = min(atomic64_read(&prog_name_len),in_max);

    temp_buf = kmalloc_array(in_max, TASK_COMM_LEN, GFP_KERNEL);
    if (!temp_buf) {
        printk(KERN_ERR "Throttling module: kmalloc error in get_all_progs\n");
        return -ENOMEM;
    }

    rcu_read_lock();

    hash_for_each(prog_hash, curr, entry_prog, hlist) {
        if (copied < in_max) {
            strscpy(temp_buf[copied], entry_prog->name, TASK_COMM_LEN);
            copied++;
        } else {
            break;
        }
    }

    rcu_read_unlock();

    *out_buf = temp_buf;

    return copied;
}


//api che rimuove tutte le strutture dati rcu
int cleanup_rcu(void) {
    struct hacked_syscall *entry;
    struct registered_uid *entry_uid;
    struct registered_prog *entry_prog;
    struct hlist_node *tmp;
    int curr = 0;
    
    //forzo lo spegnimento del monitor
    switch_off_monitor();

    synchronize_rcu();

    spin_lock(&write_lock);

    hash_for_each_safe(hacked_syscall_hash, curr, tmp, entry, hlist) {
        hash_del_rcu(&entry->hlist);
        kfree(entry);
    }

    hash_for_each_safe(uid_hash, curr, tmp, entry_uid, hlist) {
        hash_del_rcu(&entry_uid->hlist);
        kfree(entry_uid);
    }

    hash_for_each_safe(prog_hash, curr, tmp, entry_prog, hlist) {
        hash_del_rcu(&entry_prog->hlist);
        kfree(entry_prog);
    }

    spin_unlock(&write_lock);

    return 0;
}


void check_and_set_statistic(unsigned long delay, int original_sysnum) {
    struct hacked_syscall *entry_sys;
    rcu_read_lock();
        
    hash_for_each_possible_rcu(hacked_syscall_hash, entry_sys, hlist, original_sysnum) {
        if(entry_sys->syscall_nr == original_sysnum) {
            
            if (entry_sys->stats->peak_delay < delay) {
                //aggiorno statistiche
                spin_lock(&stats_lock);

                entry_sys->stats->peak_delay = delay;
                entry_sys->stats->peak_uid = __kuid_val(current_euid());
                strscpy(entry_sys->stats->peak_prog_name, current->comm, TASK_COMM_LEN);

                spin_unlock(&stats_lock);
            }
            
            break;
        }
    }
        
    rcu_read_unlock();
    return;
}


long throttling_wrapper(const struct pt_regs *regs) {
    //se arrivo a questa funzione, sono in una system call monitorata, dunque mi andrò a chiedere
    //chi è finito qua

    bool skip_check = false;
    bool need_mon = false;
    uid_t curr_ueid;

    unsigned long start_time = 0;
    unsigned long delay = 0;

    struct hacked_syscall *entry_sys;

    long (*to_call)(const struct pt_regs *) = NULL;

    //questo per la syscall da invocare post wrapper
    int original_sysnum = regs->orig_ax;

    //prima cosa check sul monitor
    if(atomic_read(&is_monitor_active) == 0) {
        //monitor off, salto diretto alla system call (no throttling)
        goto original_system_call;
    }

    //questa porzione di codice viene sempre raggiunta dalle system call registrate (monitor on)
    //controllo prog name e user id
    curr_ueid = __kuid_val(current_euid());


    need_mon = check_uid(curr_ueid);
    skip_check = need_mon;

    if(!skip_check) {
        //entro se non ho trovato euid
        need_mon = check_progname(current->comm);
    }


    //recupero system call originale
    rcu_read_lock();
        
    hash_for_each_possible_rcu(hacked_syscall_hash, entry_sys, hlist, original_sysnum) {
        if(entry_sys->syscall_nr == original_sysnum) {
            to_call = entry_sys->original_syscall;
            break;
        }
    }
        
    rcu_read_unlock();

    if (!need_mon) {
        //user id e program name correnti non sono sotto monitoraggio, vado alla system call
        goto original_system_call;
    }

    
    //controllo disponibilità system call
    //controllo precedente
    /*if (atomic_dec_if_positive(&curr_syscalls) < 0) {
        //entro qua se non posso invocare system call, mi metto in attesa.
        
        //printk(KERN_INFO "Throttling module: syscall blocked. Now blocked threads are :%lld\n",atomic64_read(&blocked_thread));

        //per statistiche (magari sostituire con atomic_inc_return??)
        atomic64_inc_return(&blocked_thread);
        start_time = jiffies;

        //come condizione di risveglio: monitor spento oppure token disponibili e preso token
        int wait_ret = wait_event_interruptible(thrott_wq, 
                    atomic_read(&is_monitor_active) == 0 || atomic_dec_if_positive(&curr_syscalls) >= 0);
        
        delay = (jiffies - start_time);
        

        if (wait_ret != 0) {
            //se si verificano altre condizioni di risveglio
            atomic64_dec_return(&blocked_thread);
            return -EINTR; 
        }

        atomic64_dec_return(&blocked_thread);
    }*/

    //controllo modificato
    //curr_syscall è il numero di token disponibili
    while(atomic_dec_if_positive(&curr_syscalls) < 0) {
        
        //controllo se monitor è spento (utile se thread non ottiene token nel mentre che viene spento monitor)
        if (atomic_read(&is_monitor_active) == 0) {
            break; 
        }

        //incremento variabili che tengono numero di thread bloccati
        atomic64_inc(&blocked_thread);
        
        //per statistiche del tempo di attesa imposto istante iniziale di attesa
        start_time = jiffies;

        //come condizione di risveglio: monitor spento oppure token disponibili e preso token
        int wait_ret = wait_event_interruptible(thrott_wq, 
                    atomic_read(&is_monitor_active) == 0 || atomic_read(&curr_syscalls) > 0);

        //decremento numero thread bloccati di uno
        atomic64_dec_return(&blocked_thread);

        if (wait_ret != 0) {
            //se si verificano altre condizioni di risveglio
            atomic64_dec_return(&blocked_thread);
            return -EINTR; 
        }

        //calcolo tempo di attesa
        delay += (jiffies - start_time);

    }


    goto original_system_call;
    
    //chiamata alla system call. 
    //ci arrivo sempre, cambia cosa faccio prima
    original_system_call: 
    {   
        if (delay > 0) {
            //aggiorno statistica della system call
            check_and_set_statistic(delay,original_sysnum);
        }
        

        if (to_call != NULL) {
            //chiamo system call originale
            //printk(KERN_INFO "Throttling module: invoco syscall\n");
            long ret_value = to_call(regs);
            return ret_value;

        } else {
            //se non si trova la system call, non dovrebbe accadere
            return -ENOSYS; 
        }

    };
}