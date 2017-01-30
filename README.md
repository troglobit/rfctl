pibang
======

A simple Linux driver and control tool for 433.92 MHz RF communication
on Raspberry Pi.

The following guide assumes the Raspbian Linux distribution is used on
the Raspberry Pi. The driver and tool should work on any distribution,
as long as the kernel is relatively new.  YMMV.

The following formum topic, although dated, also covers this dirver and
provides some helpful tips and discussions.

- https://www.raspberrypi.org/forums/viewtopic.php?t=11159


rfbb driver
-----------

This is a LIRC style device driver that transmits and records pulse and
pause lengths using GPIO.  Uses code from `lirc_serial.c` by Ralph
Metzler et al.  See the file [HARDWARE.md][] for information on how to
connect the GPIO to a common 433 MHz TX module.

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


rfctl
-----

`rfctl` is a small tool, that acts as a remote control for switches that
use simple unidirectional communication based on OOK (On Off Keying)
modulation on a 433 MHz carrier.  `rfctl` uses the Linux `rfbb.ko`
kernel driver.

To build:

    cd pibang/rfctl
    make
    sudo make install

A simple test on an old style (not selflearning) NEXA/PROVE/ARC set to
group D, channel 1.

    rfctl -p NEXA -g D -c 1 -l 1
    rfctl -p NEXA -g D -c 1 -l 0

Some popular (cheap) noname RF sockets, available from e.g. Conrad (DE),
Kjell & C:o (SE), or Maplin (UK) use the SARTANO protocol and need to be
encoded like this:

    rfctl -p SARTANO -c 1000100000 -l 1     # I - 1
    rfctl -p SARTANO -c 1000010000 -l 1     # I - 2
    rfctl -p SARTANO -c 1000001000 -l 1     # I - 3
    rfctl -p SARTANO -c 1000000100 -l 1     # I - 4

    rfctl -p SARTANO -c 0100100000 -l 1     # II - 1
    rfctl -p SARTANO -c 0100010000 -l 1     # II - 2
    rfctl -p SARTANO -c 0100001000 -l 1     # II - 3
    rfctl -p SARTANO -c 0100000100 -l 1     # II - 4

    rfctl -p SARTANO -c 0010100000 -l 1     # III - 1
    rfctl -p SARTANO -c 0010010000 -l 1     # III - 2
    rfctl -p SARTANO -c 0010001000 -l 1     # III - 3
    rfctl -p SARTANO -c 0010000100 -l 1     # III - 4

    rfctl -p SARTANO -c 0001100000 -l 1     # IV - 1
    rfctl -p SARTANO -c 0001010000 -l 1     # IV - 2
    rfctl -p SARTANO -c 0001001000 -l 1     # IV - 3
    rfctl -p SARTANO -c 0001000100 -l 1     # IV - 4

Issue `rfctl --help` to get more information on supported protocols and
options.

**Note:** All protocols might not be fully tested due to lack of
receivers and time :)


Disclaimer
----------

Do not use this software in situations where operation of, or lack of
operation, may result in property damage, personal injury, or death.
Regulatory bodies may have locked down public use of RF communication in
your location at a national level.  Do not use the pibang software to
break applicable laws and regulations.


Origin & References
-------------------

This project orignates from the [rf-bitbanger][] project which was
created by Tord Andersson.  It is released under the GNU General Public
License (GPL), version 2.  See the file [COPYING][] for details.

[COPYING]:      COPYING
[HARDWARE.md]:  HARDWARE.md
[rf-bitbanger]: https://github.com/tandersson/rf-bitbanger
