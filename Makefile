obj-m += throttling_module.o

throttling_module-objs := \
	src/throttling_api.o \
	src/throttling_dev.o \
	src/my_usctm.o \
	src/throttling_mod.o \
	src/my_vtpmo.o \
	src/throttling_hidden.o

PWD := $(shell pwd)

ccflags-y := -I$(PWD)/include

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean