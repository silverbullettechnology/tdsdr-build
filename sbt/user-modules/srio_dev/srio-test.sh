#!/bin/sh

size=$1
[ -z "$size" ] && size=256

modprobe srio

dd if=/dev/urandom of=/dev/srio_dev bs=$size count=1
