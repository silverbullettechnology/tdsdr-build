#!/bin/sh
# Create fstab entries for partitions on microSD and eMMC storage 
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

fstab () {
	/bin/mkdir -p "$2"
	/bin/printf '%-20s %-20s %-10s %-21s 0  0\n' "$1" "$2" auto defaults >> /etc/fstab
}

scan () {
	/bin/grep -q " $1" /proc/partitions || return

	case `realpath /sys/block/$1/device | cut -d/ -f7` in
		mmc0) name="card" ;;
		mmc1) name="emmc" ;;
		*) return ;;
	esac

	case `/bin/grep -c " ${1}p" /proc/partitions` in
		0)	fstab /dev/$1     /media/$name ;;
		1)	fstab /dev/${1}p1 /media/$name ;;
		*)	num=1;
			for part in `/bin/grep "${1}p" /proc/partitions | cut -c26-`; do
				fstab "/dev/$part" "/media/$name/$num"
				num=$(($num + 1))
			done
			;;
	esac
}

scan "mmcblk0"
scan "mmcblk1"

