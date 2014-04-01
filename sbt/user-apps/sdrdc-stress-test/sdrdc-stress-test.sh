#!/bin/sh
# Main stress-test script
#
# Copyright 2013,2014 Silver Bullet Technology
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this
# file except in compliance with the License.  You may obtain a copy of the License at:
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the specific language governing
# permissions and limitations under the License.
#
# \copyright Copyright (C) 2013 Silver Bullet Technologies
# vim:ts=4:noexpandtab

[ "$1" = "" ] && set 0
ret=0
stty onlcr 0>&1

cat << EOM
When started, SDRDC stress-test will do the following:
- Reset and configure each AD9361 part in turn
- Start 2 threads of each of the following:
  - each stress-mem allocates and excercises 32Mbyte of DDR memory
  - each stress-fpu allocates 2x512 arrays of doubles for FPU exercise
  - each stress-alu allocates 2x512 arrays of longs for ALU exercise
- Start 1 thread of stress-nor, which will repeatedly read the NOR flash
- Start 1 thread of stress-mmc, which will repeatedly write the eMMC flash

//// WARNING: this test will overwrite the contents of the onboard eMMC flash. 
//// Ensure any valuable data is backed up before testing.  This test does not
//// write to the micro-SD or NOR flash.
EOM

if [ "$1" -lt 1 ]; then
	cat << EOM
During the testing the top program will be run to monitor CPU use.  Pressing Q
will quit top and stop the test threads.  Press Enter now to start the tests, 
or Ctrl-C to cancel.
EOM
	read foo
fi

echo "Start AD1/AD2..."
/usr/bin/ad9361 fdd-4msps-t2r2
sleep 1
/usr/bin/ad9361 -d0 set_ensm_mode fdd
/usr/bin/ad9361 -d1 set_ensm_mode fdd

echo "Start CPU threads..."
rm -f /tmp/sst.pids
stress-mem &
echo $! >> /tmp/sst.pids
stress-mem &
echo $! >> /tmp/sst.pids
stress-fpu &
echo $! >> /tmp/sst.pids
stress-fpu &
echo $! >> /tmp/sst.pids
stress-alu &
echo $! >> /tmp/sst.pids
stress-alu &
echo $! >> /tmp/sst.pids

echo "Start Flash threads..."
stress-mmc &
echo $! >> /tmp/sst.pids
stress-nor &
echo $! >> /tmp/sst.pids

# old interactive mode runs until the user quits top
if [ "$1" -lt 1 ]; then
top -d1

# new mode runs for $1 seconds, or until an error occurs
else
	now=`date +%s`
	end=$(($now + $1))

	dmesg -c > /dev/null
	rm -f /tmp/sst.dmesg
	while [ "$ret" -lt "1" ] && [ "$now" -lt "$end" ]; do

		echo ""
		top -bn1 | head -3

		echo -n "Zynq temp: "
		cat /sys/class/hwmon/hwmon0/device/temp
		ad9361 ADI_get_temperature | grep Corr

		dmesg -c | grep -v spi_bitbang_setup | tee -a /tmp/sst.dmesg | tail -5
		grep -q -f /usr/lib/sst/kernel-errors /tmp/sst.dmesg && ret=1

		for pid in `cat /tmp/sst.pids`; do
			[ -f /proc/$pid/cmdline ] || ret=2
			grep -qa 'stress-' /proc/$pid/cmdline || ret=2
		done

		sleep 1
		now=`date +%s`
	done

fi

echo "Stopping AD1/AD2..."
/usr/bin/ad9361 -d0 set_ensm_mode sleep
/usr/bin/ad9361 -d1 set_ensm_mode sleep

echo "Stopping test threads..."
killall stress-mem stress-fpu stress-mmc stress-nor stress-alu 2>/dev/null
while [ -n "`pidof stress-mem stress-fpu stress-mmc stress-nor stress-alu`" ]; do
	sleep 1
done

case "$ret" in 
	0) echo "Stress test completed" ;;
	1) echo "Stress test failed - kernel errors" ;;
	2) echo "Stress test failed - threads aborted" ;;
esac
echo ""
echo ""

