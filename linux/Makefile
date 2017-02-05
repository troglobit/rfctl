ifneq ($(KERNELRELEASE),)
#invoked from the kernel build system

obj-m += pibang.o

else
#invoked directly from the command line
ifneq ($(KERNEL_DIR),)
	KERNELDIR := $(KERNEL_DIR)
else
ifeq ($(KERNELDIR),)
	KERNELDIR := /lib/modules/`uname -r`/build
endif
endif

ifneq ($(EBT_ARCH),)
	ARGS += ARCH=$(EBT_ARCH)
endif

ifneq ($(EBT_HOST_CROSS),)
	ARGS += CROSS_COMPILE=$(EBT_HOST_CROSS)
endif


PWD  := $(shell pwd) 

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $(ARGS) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $(ARGS) clean

# Run this target as root to load driver and create device node
insmod:
	insmod pibang.ko
	mknod /dev/pibang c `grep pibang /proc/devices | sed 's/\([0-9]*\) pibang/\1/'` 0
	chown root:dialout /dev/pibang
	chmod g+rw /dev/pibang

endif
