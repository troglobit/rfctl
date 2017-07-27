rfctl driver
============

This LIRC style driver transmits and records 433.92 MHz pulses and pause
lengths by bit banging a single GPIO pin.  To build on target you first
need to install the kernel headers:

```sh
sudo apt install raspberrypi-kernel-headers
```

Now we can build and install the kernel driver:

```sh
cd rfctl/kernel
make
sudo make install
```

The `install` target calls `make insmod`, sets the correct direction of
the GPIO pin, and also creates the device node used by the `rfctl` tool.

Most users are done now and can start playing with `rfctl`.  If you run
into problems, check the below sections for some pointers.


gpio direction
--------------

For modern Raspberry Pi kernels the GPIO handling is a bit different.
One needs to define in a device tree overlay the desired behavior of our
pin(s).  Writing a device tree overlay is outside the scope of this tiny
README, but we can abuse an existing overlay.  Add this line to the file
`/boot/config.txt`:

    dtoverlay=gpio-poweroff,gpiopin=17,active_low=1


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

