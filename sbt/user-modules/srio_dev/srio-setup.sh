#!/bin/sh

dmesg -n8
dmesg -c > /dev/null
modprobe srio

