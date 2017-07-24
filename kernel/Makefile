# rfctl kernel driver
#
#     make
#     sudo make insmod
# or
#     sudo make install
#

ifneq ($(KERNELRELEASE),)

# For kernel build system
obj-m += rfctl.o

else

# Helpers to call kernel build system and install
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

all default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $(ARGS) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $(ARGS) clean

install: gpio insmod
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $(ARGS) modules_install
	depmod -a
	grep rfctl /etc/modules     || echo rfctl >> /etc/modules
	grep 17 /etc/rc.local       || (sed -i 's/exit 0/echo out > /sys/class/gpio/gpio17/direction/' /etc/rc.local)
	grep 'exit 0' /etc/rc.local || echo 'exit 0' >> /etc/rc.local

gpio:
	echo 17  > /sys/class/gpio/export
	echo out > /sys/class/gpio/gpio17/direction

insmod:
	-insmod rfctl.ko
	-mknod /dev/rfctl c `grep rf /proc/devices | sed 's/\([0-9]*\) rfctl/\1/'` 0
	chown root:dialout /dev/rfctl
	chmod g+rw /dev/rfctl

rmmod:
	-rmmod rfctl.ko

endif
