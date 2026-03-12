#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <asm/syscall.h>

#include "throttling.h"
#include "throttling_dev.h"

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

/**
 *   Funzione per gestire le operazioni ioctl sul device
 *   @file: puntatore alla struttura file;
 *   @cmd: comando ioctl;
 *   @arg: argomento passato all'ioctl.
 */
static long int throttling_ioctl(struct file *file, unsigned cmd, unsigned long arg) {

    int param_int;
    char prog_name[256];
    struct throttling_stats stats;
    uid_t param_uid;

    //verifica se root (serve per tutti i comandi?)
    /*if (current_uid().val != 0) {
        printk(KERN_WARNING "Throttling Module: u must be root!\n");
        return -EPERM; // Error: Operation not permitted
    }*/

    //in base al comando si deve poter: 
    //registrare/deregistrare program name, user id, syscall num;
    //vedere quali di questi sono registrati; 
    //disattivare/attivare monitor;
    //stampa statistiche?
    switch(cmd) {
        case IOCTL_REGISTER_SYSCALL:
        	//registrazione syscall
        	if (copy_from_user(&param_int, (int __user *)arg, sizeof(int))) {
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
        	if (copy_from_user(&param_uid, (int __user *)arg, sizeof(uid_t))) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }

            printk(KERN_INFO "%s: Registering User id %d\n", MODULE_NAME, param_int);

            return register_user_id(param_uid);

        case IOCTL_REGISTER_PROG:
        	//registrazione program name
        	if (copy_from_user(prog_name, (char __user *)arg, sizeof(prog_name))) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT;
            }
            printk(KERN_INFO "%s: Registering program name %s\n", MODULE_NAME, prog_name);

            return register_prog_name(prog_name);

        case IOCTL_DEREGISTER_SYSCALL:
        	//per deregistrare system call
        	if (copy_from_user(&param_int, (int __user *)arg, sizeof(int))) {
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
        	if (copy_from_user(&param_uid, (int __user *)arg, sizeof(uid_t))) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT; 
            }

        	printk(KERN_INFO "%s: Deregistering User id %d\n", MODULE_NAME, param_int);

        	return deregister_user_id(param_uid);

        case IOCTL_DEREGISTER_PROG:
        	//per deregistrare programma
        	if (copy_from_user(prog_name, (char __user *)arg, sizeof(prog_name))) {
        		printk(KERN_ERR "%s: Failed to copy data from user space\n", MODULE_NAME);
                return -EFAULT;
            }

        	printk(KERN_INFO "%s: Deregistering program %s\n", MODULE_NAME, prog_name);

        	return deregister_prog_name(prog_name);

        case IOCTL_SET_MAX_CALLS:
        	//per impostare max syscall
        	if (copy_from_user(&param_int, (int __user *)arg, sizeof(int))) {
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

        case IOCTL_GET_STATS:
        	//per ottenere statistiche
        	printk(KERN_INFO "%s: Getting stats\n", MODULE_NAME);

        	stats = get_stats();
            
            if (copy_to_user((void __user*)arg, &stats, sizeof(struct throttling_stats)) != 0) {
                printk(KERN_ERR "Throttler: Impossibile copiare i dati in User Space!\n");
                // Codice di errore standard POSIX per "Bad Address" (Puntatore utente invalido)
                return -EFAULT; 
            }
        	//TODO implementare delle stampe?
            return 0;

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

/*
*   Funzione per inizializzare il device per esporre le API activate, deactivate e restore.
*/
int dev_init(void) {
    major = register_chrdev(0, MODULE_NAME, &fops);
    if(major < 0) {
        printk(KERN_ERR "%s: Failed to register character device with error %d\n", MODULE_NAME, major);
        return major;
    }

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
    throttling_class = class_create(MODULE_NAME);
    #else
    throttling_class = class_create(THIS_MODULE, MODULE_NAME);
    #endif
    if (IS_ERR(throttling_class)) {
        printk(KERN_ERR "%s: Failed to create class\n", MODULE_NAME);
        unregister_chrdev(major, MODULE_NAME);
        return PTR_ERR(throttling_class);
    }

    if(IS_ERR(throttling_class)) {
        printk(KERN_ERR "%s: Failed to create class\n", MODULE_NAME);
        unregister_chrdev(major, MODULE_NAME);
        return PTR_ERR(throttling_class);
    }

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
    

/*
*   Funzione per deregistrare il device.
*/
void dev_cleanup(void) {
    
    device_destroy(throttling_class, MKDEV(major, 0));

    class_destroy(throttling_class);

    unregister_chrdev(major, MODULE_NAME);

}