# Syscall Throttling LKM
## 1. System Requirements
- Linux kernel version >= 6.3;
- make and gcc installed;
- root privileges required to load/unload the module;

## 2. Module Installation
To build and load the module and test files, execute the following commands from the project's root directory:
```
make
cd user
make
cd ..
sudo insmod throtting_module.ko
```
Then, in the user directory, two files can be found: interface.c and user.c. The compiled files are inter and victim.

interface.c is the interface of the module. user.c is an example file to monitor.

To launch the interface.c file:
`sudo ./inter` or `./inter`

Some IOCTL commands require root privileges.

## 3. Module Operations
This module allows the user to:
- register/unregister syscalls;
- register/unregister UID;
- register/unregister program names;
- switching on/off monitor;
- access to statistics;
- specify how many monitored syscall invocation per second;
- checking registration of syscall/UID/program name;
- listing all registered entities.

All IOCTL commands are defined in the header file `throttling.h`

## 4. Repository Structure
- `include/` contains all the header files:
  - `syscall_table_hack.h` and `vtpmo.h` expose function to manipulate system call table and the pointer to the system call table;
  - `throttling.h` describes all IOCTL commands and data structures to use in cross ring data moves;
  - `throttling_api.c` describes all internal kernel functions of the module;
  - `throttling_dev.h` exposes two functions for initialization and clean up of the device;
  - `throttling_hidden.h` exposes functions of the timer mechanism of the module and the wait queue;
  - `throttling_rcu.h` exposes all the data structures and variables accessed via RCU.
- `src/` contains all the kernel code:
  - `my_usctm.c` and `my_vtpmo.c` files contain the code for system call table discovery (code of Professor Quaglia Francesco);
  - `throttling_api.c`contains the implementation of all the internal kernel functions of the module;
  - `throttling_dev.c` contains the implementation of the api of the device and the code that handles IOCTL commands;
  - `throttling_hidden.c` contains the implementation of the functions that manage the timer and wait queue;
  - `throttling_mod.c` contains the implementation of the init_module() and cleanup_module() api and all the declaration of data structures and variables of the module.
- `user/` contains the user level code of the project:
  - `interface.c` contains the code of the interface user level that interacts with the module. With this file you can configure and test the LKM;
  - `user.c` is a simple example file that has to be monitored.

## 5. Specification of the Project
This specification is related to a Linux Kernel Module (LKM) implementing a system call throttling mechanism. The LKM should offer a device driver for supporting the registration/deregistration of:
  - program names (executable names)
  - user IDs
  - syscall numbers (according to x86-64 specification) 

Such registration can be supported via the ioctl(...) system call. The registered syscall numbers are used to indicate that the corresponding systen calls can be critical for various aspects, like scalability, performance or security.

Each time one of the registered syscall is invoked by a program that is registered by the device driver or by a user (effective user-ID) that is registered by the device driver, the real execution of the system call needs to be controlled by a monitor offered by the LKM.

In particular, the LKM monitor can be configured to indicate what is the maximum number MAX of registered syscalls that can be actually invoked in a wall-clock-time window of 1 second by the registered program-names/user-IDs. If the actual volume of invocations per wall-clock-time unit exceeds MAX, then the invoking threads need to be temporarily blocked, thus preventing their actual syscall invocation.

The syscalls managed by the LKM can be of whathever nature. Hence they can be either blocking or non-blocking.

The device driver also needs to offer the possibility to verify at each time instant what are the registered program names, user-IDs and syscall numbers. Also, the update of such information can be only carried out by a thread which is running with effective user-ID set to the value 0 (root).

Also, the device driver needs to support the possibility to turn the syscall monitor off/on along time. If it is turned off, no limit on the frequencey of registered syscalls ivocations per wall-clock-time unit gets applied. Additionally, on the basis of the selected value for MAX, the device driver should also provide data related to:

  - The peak delay for the actual execution of an invoked system call, and the corresponding program-name and user-ID
  - The average and peak numbers of threads that had to be blocked 
