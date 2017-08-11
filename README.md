433.92 MHz RF communication on Raspberry Pi
===========================================
[![License Badge][]][License] [![Travis Status][]][Travis]

[rfctl][] is a simple Linux driver and control tool for 433.92 MHz RF
communication on Raspberry Pi.

The following guide assumes the Raspbian Linux distribution is used on
the Raspberry Pi.  However, the kernel driver and the `rfctl` tool
should work fine on any distribution with a relatively modern kernel.

The following formum topic, although dated, also covers this driver and
provides some helpful tips and discussions.

- https://www.raspberrypi.org/forums/viewtopic.php?t=11159


rfctl driver
------------

The default control interface of the `rfctl` tool utilizes `rfctl.ko`.
It is a LIRC style kernel device driver transmitting and recording pulse
and pause lengths by bit banging on a GPIO pin.  See [HARDWARE.md][] for
information on how to connect the GPIO to a common 433 MHz TX module.

To build you first need to install the kernel headers, in Raspbian the
`raspberrypi-kernel-headers` meta package points to the latest kernel
headers, which will install somewhere in `/lib/modules`:

```sh
sudo apt install raspberrypi-kernel-headers
```

Then enter the kernel driver directory to build, load the driver, and
create the device node `rfctl` uses:

```sh
cd rfctl/kernel
make
sudo make insmod
```


rfctl tool
----------

`rfctl` is a small tool that acts as a remote control for switches that
use simple unidirectional communication based on OOK (On Off Keying)
modulation on a 433 MHz carrier.  By default `rfctl` uses the Linux
`rfctl.ko` kernel driver.

To build:

```sh
cd rfctl/src
make
sudo make install
```

A simple test on an old style (not selflearning) NEXA/PROVE/ARC set to
group D, channel 1.

```sh
rfctl -p NEXA -g D -c 1 -l 1
rfctl -p NEXA -g D -c 1 -l 0
```

Some popular (cheap) noname RF sockets, available from e.g. Conrad (DE),
Kjell & C:o (SE), or Maplin (UK) use the SARTANO/ELRO protocol and need
to be encoded like this:

```sh
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
```

Issue `rfctl --help` to get more information on supported protocols and
options.

**Note:** All protocols might not be fully tested due to lack of
receivers and time :)


there are four lights
---------------------

As an example of how to automate your home, there is a small script
included that can be set up to run from cron.  For details, see the
script header comments in [onoff.sh][] it also has optional support
for the Pimoroni [Firefly Light][], this because the author has a
small Raspberry Pi Zero at home.


disclaimer
----------

Do not use this software in situations where operation of, or lack of
operation, may result in property damage, personal injury, or death.
Regulatory bodies may have locked down public use of RF communication in
your location at a national level.  Do not use the `rfctl` software to
break applicable laws and regulations.


origin & references
-------------------

This project orignates from the [rf-bitbanger][] project which was
created by Tord Andersson.  It is released under the GNU General Public
License (GPL), version 2.  See the file [COPYING][] for details.

Code fragments from `rfcmd` by Tord Andersson, Micke Prag, Gudmund
Berggren, Tapani Rintala, and others.  The kernel driver, `rfctl.ko`,
is based on `lirc_serial.c` by Ralph Metzler et al.

[COPYING]:       COPYING
[HARDWARE.md]:   HARDWARE.md
[rfctl]:         https://github.com/troglobit/rfctl
[onoff.sh]:      https://github.com/troglobit/rfctl/onoff.sh
[rf-bitbanger]:  https://github.com/tandersson/rf-bitbanger
[Firefly Light]: https://learn.pimoroni.com/tutorial/sandyj/firefly-light
[License]:       https://en.wikipedia.org/wiki/GPL_license
[License Badge]: https://img.shields.io/badge/License-GPL%20v2-blue.svg
[Travis]:        https://travis-ci.org/troglobit/rfctl
[Travis Status]: https://travis-ci.org/troglobit/rfctl.png?branch=master
