#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/syscall.h>
#include <linux/nospec.h>

#include "throttling.h"
#include "throttling_rcu.h"
#include "syscall_table_hack.h"

#define MODULE_NAME "THROTTLING MOD"

//api che registra system call number
int register_system_call(const int syscall_numb){

	//hack system call table
    hack_syscall(syscall_numb);
    return 0;

}

//api che deregistra system call number
int deregister_system_call(const int syscall_numb){
	//dishack system call table
    dishack_syscall(syscall_numb);
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
    	return 0;
    }
    
    //se non è registrato
    list_add_rcu(&new_uid->list, &uid_list);
    
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
    return 0; 
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

    //copio al massimo 255, chiedere se va bene
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
    return 0;
}



//api che accende il monitor se è spento (altrimenti non fa nulla)
int switch_on_monitor(void){

    spin_lock(&write_lock);
    if (is_monitor_active) {
        spin_unlock(&write_lock);
        printk(KERN_INFO "Throttling module: monitor already on\n");
        return 0;
    }
    //se non è acceso

    //se il prof volesse che riattivo tutte le system call, allora non prendo lock e chiamo register_system_call
    //su tutte le system call prese da un'altra lista

    is_monitor_active = true;
    spin_unlock(&write_lock);
    printk(KERN_INFO "Throttling module: monitor turned on\n");
    return 0;

}

//api che spegne il monitor se è acceso (altrimenti non fa nnulla)
int switch_off_monitor(void){
    spin_lock(&write_lock);
    if (!is_monitor_active) {
        spin_unlock(&write_lock);
        printk(KERN_INFO "Throttling module: monitor already off\n");
        return 0;
    }
    //se non è acceso

    //se il prof volesse che disattivo tutte le system call, allora non prendo lock e chiamo deregister_system_call
    //su tutte le system call prese da un'altra lista

    is_monitor_active = false;
    spin_unlock(&write_lock);
    printk(KERN_INFO "Throttling module: monitor turned off\n");
    return 0;
}



//api che imposta il numero massimo di system call per secondo
int set_max_syscall(const int new_max){

    if (new_max < 0) {
        printk(KERN_INFO "Throttling module: new max value must be greater than 0, u passed: %d\n",new_max);
        return -EINVAL;
    }

	spin_lock(&write_lock);
    max_syscalls_per_sec = new_max;
    spin_unlock(&write_lock);
    return 0;
}



//api che ritorna le statistiche
struct throttling_stats get_stats(void){
    
    struct throttling_stats test;// = kmalloc(sizeof(struct throttling_stats), GFP_KERNEL);
    test.max_blocked_threads = 3;
    test.avg_blocked_threads = 3;
    return test;
}


//funzione che sostituisce la origniale system call con il mio wrapper per throttling
int hack_syscall(int sys_num) {

    //alloco fuori dai lock
    struct hacked_syscall *new_hack = kmalloc(sizeof(struct hacked_syscall), GFP_KERNEL);
    if (!new_hack) {
        printk(KERN_ERR "Throttling module: kmalloc error in hack_syscall\n");
        return -ENOMEM;
    }

    //sanitizzazione, il controllo fatto prima
    int safe_nr = array_index_nospec(sys_num, NR_syscalls);

    //prendo lock in scrittura, nessuno deve scrivere oltre me
    spin_lock(&write_lock);

    //controllo se già presente e in caso annullo tutto
    if(syscall_array[safe_nr]) {
        spin_unlock(&write_lock);
        kfree(new_hack);
        printk(KERN_ERR "Throttling module: syscall %d already hacked\n",safe_nr);
        return -EINVAL;
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

//funzione che ripristina la vecchia system call
int dishack_syscall(int sys_num){

    struct hacked_syscall *to_remove = NULL;
    struct hacked_syscall *entry;

    //sanitizzazione, il controllo fatto prima
    int safe_nr = array_index_nospec(sys_num, NR_syscalls);

    spin_lock(&write_lock);

    if(!syscall_array[safe_nr]) {
        spin_unlock(&write_lock);
        printk(KERN_ERR "Throttling module: syscall %d not hacked, deregistration did nothing\n",safe_nr);
        return -EINVAL;
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
    
    kfree(to_remove);

    return 0;
}

//api che rimuove tutte le strutture dati rcu
int cleanup_rcu(void) {
    struct hacked_syscall *entry, *tmp;
    struct registered_uid *entry_uid, *tmp2;
    struct registered_prog *entry_prog, *tmp3;
    
    spin_lock(&write_lock);
    //forzo lo spegnimento del monitor
    is_monitor_active = false;

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

    spin_lock(&write_lock);

    return 0;
}

long throttling_wrapper(const struct pt_regs *) {
    //se arrivo a questa funzione, sono in una system call monitorata, dunque mi andrò a chiedere
    //chi è finito qua

    long ret = 0;
    //set up del contesto

    //verifica se monitor è acceso

    //verifica user id o program name

    //controllo numero system call disponibili
    return ret;

}