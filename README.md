# Throttling Module

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
- `user/`:
  - `interface.c` contains the code of the interface user level that interacts with the module;
  - `user.c` is a simple example file that has to be monitored.
