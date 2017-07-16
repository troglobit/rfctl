kernel driver
=============

This LIRC style driver transmits and records pulses and pause lengths
using GPIO.  To build on target you first need to install the kernel
headers:

```sh
sudo apt install raspberrypi-kernel-headers
```

Now we can build the kernel driver:

```sh
cd rfctl/linux
make
sudo make insmod
```

Most users are done now and can start playing with `rfctl`.  If you run
into problems, check the below section for some pointers.


troubleshooting
---------------

The `KERNELDIR=` environment variable may be necessary to set if your
kernel headers are not in `/lib/modules`:

```sh
make KERNELDIR=/lib/modules/`uname -r`/build
sudo insmod rfctl.ko
```

Verify that the device node is created after `insmod`, add it if not
already there:

```sh
ls -al /dev/rfctl
dmesg
cat /proc/devices |grep rfctl
sudo mknod /dev/rfctl c 243 0
sudo chown root:dialout /dev/rfctl
sudo chmod g+rw /dev/rfctl
```

The dynamically allocated major device number can be found in the file
`/proc/devices`, here the example `243` is used but it will vary
depending on your system:

```sh
grep rfctl /proc/devices | sed 's/\([0-9]*\) rfctl/\1/'
```

