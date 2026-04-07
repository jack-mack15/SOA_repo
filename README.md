# Throttling module

### System requirements
- Linux kernel version >= 6.3;
- make and gcc installed;
- root privileges required to load/unload the module;

### Module installation
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

### Module Operations
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
