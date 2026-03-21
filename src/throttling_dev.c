#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <asm/syscall.h>

#include "throttling.h"
#include "throttling_dev.h"
#include "throttling_api.h"

#define MODULE_NAME "THROTTLING MOD"
#define DEVICE_NAME "throttling_device"

static int major;
static struct class *throttling_class = NULL;
static struct device *throttling_device = NULL;

static int driver_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "%s: Device opened\n", MODULE_NAME);
    return 0;
}

static int driver_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "%s: Device closed\n", MODULE_NAME);
    return 0;
}

/*
Funzione per gestire le operazioni ioctl sul device
file: puntatore alla struttura file;    
cmd: comando ioctl;
arg: argomento passato all'ioctl.
 */
static long int throttling_ioctl(struct file *file, unsigned cmd, unsigned long arg) {

    int param_int;
    char prog_name[TASK_COMM_LEN] = {0};
    uid_t param_uid;

    
    //se è non root può accedere comunque alle statistiche e info di cosa è registrato
    switch(cmd) {
            case IOCTL_GET_THREAD_STATS:
            //per ottenere statistiche dei thread bloccati
            printk(KERN_INFO "%s: Getting thread stats\n", MODULE_NAME);

            struct thread_stats_cr_struct *t_stats;

            t_stats = get_thread_stats();
            
            if (IS_ERR(t_stats)) {
                return PTR_ERR(t_stats);
            }
        
            if (copy_to_user((void __user*)arg, t_stats, sizeof(struct thread_stats_cr_struct)) != 0) {
                printk(KERN_ERR "Throttler: Impossibile copiare i dati in User Space!\n");
                return -EFAULT; 
            }
            return 0;
        
        case IOCTL_GET_SYSCALL_STATS:
            //per ottenere statistiche di una system call
            printk(KERN_INFO "%s: Getting system call stats\n", MODULE_NAME);

            struct syscall_cr_struct input_sys_stats;

            if (copy_from_user(&input_sys_stats, (void __user *)arg, sizeof(struct syscall_cr_struct))) {
                printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }
            
            //controllo range del parametro passato in input
            if (input_sys_stats.syscall_nr < 0 || input_sys_stats.syscall_nr >= NR_syscalls) {
                printk(KERN_ERR "%s: Syscall number %d not valid\n",MODULE_NAME, input_sys_stats.syscall_nr);
                return -1;
            }

            //kmalloc interna a get_syscall_stats()
            struct syscall_cr_struct *output_sys_stats = get_syscall_stats(input_sys_stats.syscall_nr);

            if(!output_sys_stats) {
                kfree(output_sys_stats);
                return -ENODATA;
            }

            if (IS_ERR(output_sys_stats)) {
                kfree(output_sys_stats);
                return PTR_ERR(output_sys_stats);
            }
            
            if (copy_to_user((void __user*)arg, output_sys_stats, sizeof(struct syscall_cr_struct)) != 0) {
                kfree(output_sys_stats);
                printk(KERN_ERR "%s: Failed to copy data to user space\n", MODULE_NAME);
                return -EFAULT; 
            }

            kfree(output_sys_stats);

            return 0;

        case IOCTL_CHECK_SYSCALL:
            struct check_syscall_cr sys_check;
            if (copy_from_user(&sys_check, (void __user *)arg, sizeof(struct check_syscall_cr))) {
                printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }
            if (sys_check.syscall_nr < 0 || sys_check.syscall_nr >= NR_syscalls) {
                printk(KERN_ERR "%s: Syscall number %d not valid\n",MODULE_NAME, param_int);
                return -1;
            }

            printk(KERN_INFO "%s: Checking syscall number %d\n", MODULE_NAME, sys_check.syscall_nr);

            sys_check.check = check_syscall(sys_check.syscall_nr);
            
            if (copy_to_user((struct check_syscall_cr __user *)arg, &sys_check, sizeof(struct check_syscall_cr))) {
                printk(KERN_ERR "%s: Failed to copy data to user space\n", MODULE_NAME);
                return -EFAULT; 
            }

            return 0;

        case IOCTL_CHECK_UID:
            struct check_uid_cr uid_check;
            if (copy_from_user(&uid_check, (void __user *)arg, sizeof(struct check_uid_cr))) {
                printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }

            printk(KERN_INFO "%s: Checking User id %d\n", MODULE_NAME, uid_check.uid);

            uid_check.check = check_uid(uid_check.uid);

            if (copy_to_user((struct check_uid_cr __user *)arg, &uid_check, sizeof(struct check_uid_cr))) {
                printk(KERN_ERR "%s: Failed to copy data to user space\n", MODULE_NAME);
                return -EFAULT; 
            }

            return 0;

        case IOCTL_CHECK_PROG:
            struct check_progname_cr prog_check;
            if (copy_from_user(&prog_check, (char __user *)arg, sizeof(struct check_progname_cr))) {
                printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT;
            }

            //per sicurezza aggiungo il terminatore di stringa
            prog_check.name[sizeof(prog_check.name) - 1] = '\0';

            printk(KERN_INFO "%s: Checking program name %s\n", MODULE_NAME, prog_check.name);

            prog_check.check = check_progname(prog_check.name);

            if (copy_to_user((struct check_progname_cr __user *)arg, &prog_check, sizeof(struct check_progname_cr))) {
                printk(KERN_ERR "%s: Failed to copy data to user space\n", MODULE_NAME);
                return -EFAULT; 
            }            

            return 0;
    }

    
    //controllo se l'utente è root per i prossimi comandi
    if (current_uid().val != 0) {
        printk(KERN_WARNING "Throttling Module: u must be root!\n");
        return -EPERM;
    }
        
    switch(cmd) {
        case IOCTL_REGISTER_SYSCALL:
        	//registrazione syscall
        	if (get_user(param_int, (int __user *)arg)) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }
            if (param_int < 0 || param_int >= NR_syscalls) {
                printk(KERN_ERR "%s: Syscall number %d not valid\n",MODULE_NAME, param_int);
                return -1;
            }

            printk(KERN_INFO "%s: Registering syscall number %d\n", MODULE_NAME, param_int);

            return register_system_call(param_int);

        case IOCTL_REGISTER_UID:
        	//registrazione user id
        	if (get_user(param_uid, (int __user *)arg)) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }

            printk(KERN_INFO "%s: Registering User id %d\n", MODULE_NAME, param_int);

            return register_user_id(param_uid);

        case IOCTL_REGISTER_PROG: {
        	//registrazione program name 
            
            int copied_len = strncpy_from_user(prog_name, (const char __user *)arg, TASK_COMM_LEN - 1);
        	if (copied_len < 0) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return copied_len;
            }

            //per sicurezza aggiungo il terminatore di stringa
            prog_name[sizeof(prog_name) - 1] = '\0';

            printk(KERN_INFO "%s: Registering program name %s\n", MODULE_NAME, prog_name);

            return register_prog_name(prog_name);
        }

        case IOCTL_DEREGISTER_SYSCALL:
        	//per deregistrare system call
        	if (get_user(param_int, (int __user *)arg)) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }
            if (param_int < 0 || param_int >= NR_syscalls) {
                printk(KERN_ERR "%s: Syscall number %d not valid\n",MODULE_NAME, param_int);
                return -1;
            }

        	printk(KERN_INFO "%s: Deregistering syscall number %d\n", MODULE_NAME, param_int);

        	return deregister_system_call(param_int);


        case IOCTL_DEREGISTER_UID:
        	//per deregistrazione user id
        	if (get_user(param_uid, (int __user *)arg)) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }

        	printk(KERN_INFO "%s: Deregistering User id %d\n", MODULE_NAME, param_int);

        	return deregister_user_id(param_uid);

        case IOCTL_DEREGISTER_PROG: {
        	//per deregistrare programma
        	int copied_len = strncpy_from_user(prog_name, (const char __user *)arg, TASK_COMM_LEN - 1);
            
            if (copied_len < 0) {
                printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return copied_len;
            }

            //per sicurezza aggiungo il terminatore di stringa
            prog_name[sizeof(prog_name) - 1] = '\0';

        	printk(KERN_INFO "%s: Deregistering program %s\n", MODULE_NAME, prog_name);

        	return deregister_prog_name(prog_name);
        }

        case IOCTL_SET_MAX_CALLS:
        	//per impostare max syscall
        	if (get_user(param_int, (int __user *)arg)) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }

        	printk(KERN_INFO "%s: Setting MAX param to %d\n", MODULE_NAME, param_int);

        	return set_max_syscall(param_int);

        case IOCTL_MONITOR_ON:
        	//per accendere monitor
        	printk(KERN_INFO "%s: Switching on monitor\n", MODULE_NAME);

        	return switch_on_monitor();

        case IOCTL_MONITOR_OFF:
        	//per spegnere monitor
        	printk(KERN_INFO "%s: Switching off monitor\n", MODULE_NAME);

        	return switch_off_monitor();

        default:
            printk(KERN_ERR "%s: Unknown ioctl command %u\n", MODULE_NAME, cmd);
            return -EINVAL;
    }
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_release,
    .unlocked_ioctl = throttling_ioctl,
};

static char *throttling_devnode(const struct device *dev, umode_t *mode) {
    if (mode) {
        *mode = 0666; 
    }
    return NULL;
}

//funzione per registrare device
int dev_init(void) {
    major = register_chrdev(0, MODULE_NAME, &fops);
    if(major < 0) {
        printk(KERN_ERR "%s: Failed to register character device with error %d\n", MODULE_NAME, major);
        return major;
    }

    throttling_class = class_create(MODULE_NAME);

    if (IS_ERR(throttling_class)) {
        printk(KERN_ERR "%s: Failed to create class\n", MODULE_NAME);
        unregister_chrdev(major, MODULE_NAME);
        return PTR_ERR(throttling_class);
    }

    //per modificare i permessi, vedere se va bene
    throttling_class->devnode = throttling_devnode;

    throttling_device = device_create(throttling_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    
    if(IS_ERR(throttling_device)) {
        printk(KERN_ERR "%s: Failed to create device\n", MODULE_NAME);
        class_destroy(throttling_class);
        unregister_chrdev(major, MODULE_NAME);
        return PTR_ERR(throttling_device);
    }

    printk(KERN_INFO "%s: Device registered with major number %d\n", MODULE_NAME, major);
    return 0;
}
    
//funzione per deregistrare device
void dev_cleanup(void) {
    
    device_destroy(throttling_class, MKDEV(major, 0));

    class_destroy(throttling_class);

    unregister_chrdev(major, MODULE_NAME);

}