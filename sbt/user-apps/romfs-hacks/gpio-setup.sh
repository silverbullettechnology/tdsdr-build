#!/bin/sh

tbl=/etc/gpio-setup

cd /sys/class/gpio
cat $tbl | sed 's/#.*$//' | while read pin dir val disc; do
	[ -z "$pin" ] && continue
	[ -z "$dir" ] && continue
	[ "$dir$val" = "out" ] && continue

	echo $pin > export
	echo $dir > gpio$pin/direction
	if [ "$dir" = "out" ]; then
		case "$val" in
			lo) echo 0 > gpio$pin/value ;;
			hi) echo 1 > gpio$pin/value ;;
		esac
	fi
done

