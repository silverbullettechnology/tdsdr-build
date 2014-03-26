#!/bin/sh
# SBT Kernel Git setup script for SBT SDRDC 
# Copyright (C) 2014 Silver Bullet Technology

[ -z "$SBT_TOP" ] && exit 1

# Symlink user-apps/libs/modules containing Kconfig file
for type in apps libs modules; do
	find "$SBT_TOP/sbt/user-$type" -name 'Kconfig*' | rev | cut -d/ -f2 | rev |\
	sort | uniq | while read name; do
		link="$SBT_PETALINUX/software/user-$type/$name"
		if [ -L "$link" ]; then
			echo "User-$type $name is already a symlink, no action taken"
		elif [ -e "$link" ]; then
			echo "User-$type $name is obscured, no action taken"
		else
			ln -s ../../../sbt/user-$type/$name "$link"
		fi
	done
done

exit 0
