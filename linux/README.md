rfbb driver
-----------

This LIRC style driver transmits and records pulses and pause lengths
using GPIO.  It uses code from `lirc_serial.c` by Ralph Metzler et al.

To build on target you first need to install the kernel headers, in
Raspbian the `raspberrypi-kernel-headers` meta package points to the
latest kernel headers, which will install somewhere in `/lib/modules`:

    sudo apt install raspberrypi-kernel-headers

Then enter the kernel driver directory and build, the `KERNELDIR=`
environment variable only needs to be set if your kernel headers are not
in `/lib/modules`:

    cd pibang/linux
    make KERNELDIR=/lib/modules/`uname -r`/build
    sudo insmod rfbb.ko

Check for device node and add if not already there using dialout as
group.

    ls -al /dev/rfbb
    dmesg
    cat /proc/devices |grep rfbb
    sudo mknod /dev/rfbb c 243 0
    sudo chown root:dialout /dev/rfbb
    sudo chmod g+rw /dev/rfbb

The dynamically allocated major device number can be found in the file
`/proc/devices`, here the example `243` is used but it will vary
depending on your system:

    grep rfbb /proc/devices | sed 's/\([0-9]*\) rfbb/\1/'

There is also the more user-friendly way to do all of the above:

    cd pibang/linux
    make
	sudo make insmod

