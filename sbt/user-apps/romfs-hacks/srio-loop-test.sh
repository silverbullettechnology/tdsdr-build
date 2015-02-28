#!/bin/sh

md5=`md5sum /media/card/boot.bin | cut -c1-32`
bin=`grep $md5 /media/card/md5sums | cut -c35-`
log=/media/card/$bin.log

modprobe srio

dmesg -n1

echo "$md5  $bin"      | tee -a $log
echo "start main loop" | tee -a $log
#for pre in 0 9 15 20 ; do
#	for post in 0 9 15 20 ; do
		for diff in 0 2 4 6  9 11 13 15; do

			echo ""                    | tee -a $log
			echo "diffctrl   : $diff"  | tee -a $log
#			echo "precursor  : $pre"   | tee -a $log
#			echo "postcursor : $post"  | tee -a $log

			echo "Ready, press Enter to test"
			read

			echo $diff > /proc/srio_dev/diffctrl
#			echo $pre  > /proc/srio_dev/precursor
#			echo $post > /proc/srio_dev/postcursor

			echo > /proc/srio_dev/reset
			cnt=0
			while [ "$cnt" -lt 30 ]; do
				echo -n "."
				usleep 500000
				cnt=$(($cnt + 1))
			done
			echo "!"
			cat    /proc/srio_dev/counts  | tee -a $log

		done
#	done
#done
echo "done main loop" | tee -a $log

sync
umount /media/card
reboot -f
