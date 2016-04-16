#!/bin/sh

unset devid
for mnt in `fgrep ' /media/' /proc/mounts | cut -d' ' -f2`; do
	if [ -s "$mnt/srio-config" ]; then
		devid=`grep '^devid=' "$mnt/srio-config" | cut -d= -f2`
		[ -n "$devid" ] && break;
	fi
done

[ -n "$devid" ] && devid="devid=$devid"
/sbin/modprobe srio $devid
