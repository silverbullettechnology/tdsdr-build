#!/bin/sh

gpio=55
wait=62500

cd /sys/class/gpio
echo $gpio > export
echo out > gpio$gpio/direction

while true; do
	echo 0 > gpio$gpio/value
	usleep $wait
	echo 1 > gpio$gpio/value
	usleep $wait
done

