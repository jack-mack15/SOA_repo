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
    list_add_rcu(&new_hack->list, &hacked_syscall_list);
    
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

    list_for_each_entry(entry, &hacked_syscall_list, list) {
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
    list_del_rcu(&to_remove->list);

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
    list_for_each_entry(curr, &uid_list, list) {
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
    list_add_rcu(&new_uid->list, &uid_list);
    atomic64_inc(&uids_len);
    spin_unlock(&write_lock);

    printk(KERN_INFO "Throttling module: UID %u registered\n", user_id);
    
    return 0;
}


//api che deregistra user id
int deregister_user_id(const uid_t user_id){
	//update rcu
    struct registered_uid *curr;
    struct registered_uid *to_remove = NULL;

    spin_lock(&write_lock);

    //ricerca e sgancio
    list_for_each_entry(curr, &uid_list, list) {
        if (curr->uid == user_id) {
            //sgancio dalla lista (primo step)
            list_del_rcu(&curr->list);
            atomic64_dec(&uids_len);
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

    new_prog = kmalloc(sizeof(struct registered_prog), GFP_KERNEL);
    if (!new_prog) {
        printk(KERN_ERR "Throttling module: kmalloc error in register_prog_name\n");
        return -ENOMEM;
    }

    //copio i nomi
    strncpy(new_prog->name, prog_name, sizeof(new_prog->name) - 1);
    new_prog->name[sizeof(new_prog->name) - 1] = '\0';

    spin_lock(&write_lock);

    list_for_each_entry(curr, &prog_list, list) {

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
    
    list_add_rcu(&new_prog->list, &prog_list);
    
    spin_unlock(&write_lock);

    printk(KERN_INFO "Throttling module: prog name '%s' registered\n", prog_name);
    return 0;
}


//api che deregistra il program name specificato
int deregister_prog_name(const char *prog_name){
	struct registered_prog *curr;
    struct registered_prog *to_delete = NULL;

    spin_lock(&write_lock);

    list_for_each_entry(curr, &prog_list, list) {
        if (strncmp(curr->name, prog_name, sizeof(curr->name)) == 0) {
            
            list_del_rcu(&curr->list); 
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
        wake_up_interruptible(&thrott_wq);

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



//api che ritorna le statistiche
struct thread_stats_cr_struct *get_thread_stats(void){
    
    struct thread_stats_cr_struct *to_ret = kmalloc(sizeof(struct thread_stats_cr_struct), GFP_KERNEL);
    if(!to_ret) {
        printk(KERN_ERR "Throttling module: kmalloc error in get_thread_stats\n");
        return ERR_PTR(-ENOMEM);
    }

    unsigned long temp = jiffies - atomic64_read(&(info_threads.start_time));
    to_ret->sum_blocked = atomic64_read(&(info_threads.sum_blocked));
    to_ret->elapsed = jiffies_to_msecs(temp);
    to_ret->peak_blocked = atomic_read(&(info_threads.peak_blocked));
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

    list_for_each_entry_rcu(entry, &hacked_syscall_list, list) {
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
    to_ret->peak_delay = curr->stats->peak_delay;
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

    list_for_each_entry_rcu(entry, &uid_list, list) {
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
    
    rcu_read_lock();

    list_for_each_entry_rcu(entry, &prog_list, list) {
        if (strncmp(entry->name, prog_name, sizeof(entry->name)) == 0) {
            to_ret = 1;
            break;
        }
    }

    rcu_read_unlock();

    return to_ret;
}

int get_lenght(const int choose) {
    int count = 0;
    
    //recupero numero system call hackerate
    if (choose == 0) {
        struct hacked_syscall *entry_sys;
        rcu_read_lock();
        list_for_each_entry_rcu(entry_sys, &hacked_syscall_list, list) {
            count++;
        }
        rcu_read_unlock();

      //recupero numero uid registrati
    } else if (choose == 1) {
        struct registered_uid *entry_uid;
        rcu_read_lock();
        list_for_each_entry_rcu(entry_uid, &uid_list, list) {
            count++;
        }
        rcu_read_unlock();

      //recupero numero program name registrati
    } else {
        struct registered_prog *entry_prog;

        rcu_read_lock();
        list_for_each_entry_rcu(entry_prog, &prog_list, list) {
            count++;
        }
        rcu_read_unlock();

    }

    return count;
}

int get_all_syscalls(int in_max, int **out_sys) {
    int copied = 0;
    return copied;
}

//riceve il numero massimo di uid che si desiderano
int get_all_uids(int in_max, uid_t **out_uids) {
    struct registered_uid *entry;
    uid_t *temp_buf;
    int copied = 0;

    //prendo il minimo tra i due
    in_max = min(atomic64_read(&uids_len),in_max);

    temp_buf = kmalloc(in_max * sizeof(uid_t), GFP_KERNEL);
    if (!temp_buf) {
        printk(KERN_ERR "Throttling module: kmalloc error in get_all_uids\n");
        return -ENOMEM;
    }

    rcu_read_lock();

    list_for_each_entry_rcu(entry, &uid_list, list) {
        
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

    //prendo il minimo tra i due
    in_max = min(atomic64_read(&prog_name_len),in_max);

    temp_buf = kmalloc_array(in_max, TASK_COMM_LEN, GFP_KERNEL);
    if (!temp_buf) {
        printk(KERN_ERR "Throttling module: kmalloc error in get_all_progs\n");
        return -ENOMEM;
    }

    rcu_read_lock();

    list_for_each_entry_rcu(entry_prog, &prog_list, list) {
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
    struct hacked_syscall *entry, *tmp;
    struct registered_uid *entry_uid, *tmp2;
    struct registered_prog *entry_prog, *tmp3;
    
    //forzo lo spegnimento del monitor
    switch_off_monitor();

    synchronize_rcu();

    spin_lock(&write_lock);

    list_for_each_entry_safe(entry, tmp, &hacked_syscall_list, list) {
        list_del_rcu(&entry->list);
        kfree(entry);
    }

    list_for_each_entry_safe(entry_uid, tmp2, &uid_list, list) {
        list_del_rcu(&entry_uid->list);
        kfree(entry_uid);
    }

    list_for_each_entry_safe(entry_prog, tmp3, &prog_list, list) {
        list_del_rcu(&entry_prog->list);
        kfree(entry_prog);
    }

    spin_unlock(&write_lock);

    return 0;
}


void check_and_set_statistic(unsigned long delay, int original_sysnum) {
    struct hacked_syscall *entry_sys;
    rcu_read_lock();
        
    list_for_each_entry_rcu(entry_sys,&hacked_syscall_list,list) {
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

    struct registered_uid *entry_uid;
    struct registered_prog *entry_prog;

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

    rcu_read_lock();
    list_for_each_entry_rcu(entry_uid, &uid_list, list) {

        printk(KERN_INFO "user id corr: %u user nella lista %u\n",curr_ueid,entry_uid->uid);

        if (entry_uid->uid == curr_ueid) {
            skip_check = true;
            need_mon = true;
            break;
        }
    }
    rcu_read_unlock();

    if(!skip_check) {
        //entro se non ho trovato euid
        rcu_read_lock();
        list_for_each_entry_rcu(entry_prog, &prog_list, list) {
            //magari sufficiente subname?
            
            printk(KERN_INFO "curr name: %s, nella lista ho %s\n", current->comm,entry_prog->name);
            
            if (strncmp(current->comm,entry_prog->name,TASK_COMM_LEN) == 0) {
                need_mon = true;
                break;
            }
        }
        rcu_read_unlock();
    }

    if (!need_mon) {
        //user id e program name correnti non sono sotto monitoraggio, vado alla system call
        goto original_system_call;
    }

    
    //controllo disponibilità system call
    if (atomic_dec_if_positive(&curr_syscalls) < 0) {
        //entro qua se non posso invocare system call, mi metto in attesa.
        
        //printk(KERN_INFO "Throttling module: syscall blocked. Now blocked threads are :%lld\n",atomic64_read(&blocked_thread));

        //per statistiche (magari sostituire con atomic_inc_return??)
        atomic64_inc(&blocked_thread);
        atomic64_inc(&info_threads.sum_blocked);
        start_time = jiffies;

        //come condizione di risveglio: monitor spento oppure token disponibili e preso token
        int wait_ret = wait_event_interruptible(thrott_wq, 
                    atomic_read(&is_monitor_active) == 0 || atomic_dec_if_positive(&curr_syscalls) >= 0);
        
        delay = (jiffies - start_time);
        

        if (wait_ret != 0) {
            //se si verificano altre condizioni di risveglio
            atomic64_dec(&blocked_thread);
            return -EINTR; 
        }

        atomic64_dec(&blocked_thread);

        //se arrivo qui non è detto che ci sia ancora curr_syscall > 0, quindi 
        //riverifico la condizione del while
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

        struct hacked_syscall *entry_sys;
        long (*to_call)(const struct pt_regs *) = NULL;
        rcu_read_lock();
        
        list_for_each_entry_rcu(entry_sys,&hacked_syscall_list,list) {
            if(entry_sys->syscall_nr == original_sysnum) {
                to_call = entry_sys->original_syscall;
                break;
            }
        }
        
        rcu_read_unlock();

        if (to_call != NULL) {
            //chiamo system call originale
            printk(KERN_INFO "Throttling module: invoco syscall\n");
            long ret_value = to_call(regs);
            return ret_value;

        } else {
            //se non si trova la system call, non dovrebbe accadere
            return -ENOSYS; 
        }

    };
}