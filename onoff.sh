#!/bin/sh
# Example 433 MHz indoor light control
# With optional support for Pimoroni Firely project
#
# Add to your crontab using:
#
#   crontab onoff.tab
#

onoff=$1
#FIREFLY=/home/pi/firefly.py
RFCTL=/usr/local/bin/rfctl

onoff()
{
    for i in `seq 1 4`; do
	echo "$RFCTL -p CONRAD -g 1 -c $i -l $1"
	sleep 1
	$RFCTL -p CONRAD -g 1 -c $i -l $1
    done
}

if [ $# -lt 1 ]; then
    echo "usage: $0 <0|1>"
    exit 1
fi

if [ "x$onoff" = "x0" -o -f /tmp/firefly.pid ]; then
    PID=`cat /tmp/firefly.pid`
    kill -TERM $PID
    onoff 0
    rm /tmp/firefly.pid 2>/dev/null
else
    if [ -n "$FIREFLY" ]; then
	python firefly.py &
	echo $! >/tmp/firefly.pid
    fi
    onoff 1
fi

exit 0
