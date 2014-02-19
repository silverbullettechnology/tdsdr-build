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
			echo "SBT vendor tree in PetaLinux is obscured, no action taken"
		else
			ln -s ../../../sbt/user-$type/$name "$link"
		fi
	done
done


# User-libs have a priority number which governs order they're built in.  The interactive
# petalinux-config-apps tool will regenerate these, but it doesn't fit the automated tool
# flow well, so we'll build them here too
conf="$SBT_PETALINUX/software/user-libs/Kconfig.lib.auto"
make="$SBT_PETALINUX/software/user-libs/Makefile.lib.auto"
rm -f $conf $make
for prio in 1 2 3 4 5 6 7 8 9 10 11; do
	find -L "$SBT_PETALINUX/software/user-libs" -name "Kconfig.$prio" |\
	rev | cut -d/ -f2 | rev | sort | uniq | while read name; do
		if [ -e "$SBT_PETALINUX/software/user-libs/$name/Makefile" ]; then
			caps=`echo -n "$name" | tr 'a-z' 'A-Z'`
			echo "menuconfig USER_LIBS_${caps}" >> $conf
			echo "bool \"$name\""               >> $conf
			echo "source $name/Kconfig.$prio"   >> $conf

			echo "dir_${prio}_\$(CONFIG_USER_LIBS_${caps})	+= $name" >> $make
		fi
	done
done

exit 0
