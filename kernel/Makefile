# rfctl kernel driver
#
#     make
#     sudo make insmod
# or
#     sudo make install
#

ifneq ($(KERNELRELEASE),)

# For kernel build system
obj-m     += rfctl.o

else

# Helpers to call kernel build system and install
ifneq ($(KERNEL_DIR),)
KERNELDIR := $(KERNEL_DIR)
else
ifeq ($(KERNELDIR),)
KERNELDIR := /lib/modules/`uname -r`/build
endif
endif
PWD       := $(shell pwd)

all default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

insmod:
	-insmod rfctl.ko
#	-mknod /dev/rfctl c `grep rf /proc/devices | sed 's/\([0-9]*\) rfctl/\1/'` 0
#	chown root:dialout /dev/rfctl
#	chmod g+rw /dev/rfctl

rmmod:
	-rmmod rfctl.ko

install: insmod
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install
	cp -v udev.rules /etc/udev/rules.d/99-rfctl.rules
	depmod -a
	grep rfctl /etc/modules || echo rfctl >> /etc/modules

endif
