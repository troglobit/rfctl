rf-bitbanger
============

Simple tools for experiments with bitbanged RF communication.

License: GPL v2. See the file COPYING for details.


Disclaimer
----------

Do not use the tools and code in situations where operation or lack of
operation may result in property damage, personal injury, or death.
Rules and regulations may control the use of RF communication at a
national level.  Do not use rf-bitbanger tools or code to break
applicable laws and regulations.

The following guide assumes you are using the Raspbian Linux
distribution on your Raspberry Pi. The driver and tool should
work on any distribution, as long as the kernel is relatively
new.  YMMV.

The following formum topic, although dated, also covers this dirver and
provides some helpful tips and discussions.

- https://www.raspberrypi.org/forums/viewtopic.php?t=11159


rfbb driver
-----------

This is a LIRC style device driver that transmits and records pulse and
pause lengths using GPIO.  Uses code from `lirc_serial.c` by Ralph
Metzler et al.  See the file [HARDWARE.md][] for information on how to
connect the GPIO to a common 433 MHz TX module.

To build on target you first need to install the kernel headers:

    sudo apt install raspberrypi-kernel-headers

Then enter the kernel driver directory and build:

    cd rf-bitbanger/linux
    make KERNELDIR=/lib/modules/`uname -r`/build
    sudo insmod rfbb.ko

Check for device node and add if not already there using dialout as
group.  The dynamically allocated major device number can be found
in the file `/proc/devices`, here the example `252` is used but it
will vary depending on your system.

    ls -al /dev/rfbb
    dmesg
    cat /proc/devices |grep rfbb
    sudo mknod /dev/rfbb c 252 0
    sudo chown root:dialout /dev/rfbb
    sudo chmod g+rw /dev/rfbb


rfctl
-----

`rfctl` is a small tool, that acts as a remote control for switches that
use simple unidirectional communication based on OOK (On Off Keying)
modulation on a 433 MHz carrier.  `rfctl` uses the Linux `rfbb.ko`
kernel driver.

To build:

    cd rf-bitbanger/rfctl
    make
    sudo make install

A simple test on an old style (not selflearning) NEXA/PROVE/ARC set to
group D, channel 1.

    rfctl -d /dev/rfbb -i RFBB -p NEXA -g D -c 1 -l 1
    rfctl -d /dev/rfbb -i RFBB -p NEXA -g D -c 1 -l 0

Some popular (cheap) noname RF sockets, available from e.g. Conrad (DE),
Kjell & C:o (SE), or Maplin (UK) use the SARTANO protocol and need to be
encoded like this:

    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 1000100000 -l 1 ### I - 1
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 1000010000 -l 1 ### I - 2
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 1000001000 -l 1 ### I - 3
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 1000000100 -l 1 ### I - 4

    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0100100000 -l 1 ### II - 1
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0100010000 -l 1 ### II - 2
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0100001000 -l 1 ### II - 3
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0100000100 -l 1 ### II - 4

    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0010100000 -l 1 ### III - 1
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0010010000 -l 1 ### III - 2
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0010001000 -l 1 ### III - 3
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0010000100 -l 1 ### III - 4

    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0001100000 -l 1 ### IV - 1
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0001010000 -l 1 ### IV - 2
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0001001000 -l 1 ### IV - 3
    rfctl -d /dev/rfbb -i RFBB -p SARTANO -c 0001000100 -l 1 ### IV - 4

Issue `rfctl --help` to get more information on supported protocols and
options.

**Note:** All protocols might not be fully tested due to lack of
receivers and time :)




/Last update: 2012-07-03 Tord Andersson
