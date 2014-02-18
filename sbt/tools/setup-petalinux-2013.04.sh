#!/bin/sh
# Petalinux setup wrapper script for SBT SDRDC 
# Copyright (C) 2014 Silver Bullet Technology

[ -z "$SBT_TOP" ] && exit 1

if ! [ -e "$SBT_PETALINUX" ]; then
	if ! tar xf "$SBT_PETALINUX_SDK" -C "$SBT_TOP"; then
		echo "Failed to extract PetaLinux installation archive.  Supported version 2013.04 is"
		echo "distributed as a tarball, while version 2013.10 is distributed with a shell"
		echo "archive wrapper.  Version 2013.10 support is coming soon, but for now please"
		echo "download version 2013.04."
		exit 1
	fi

	mv "$SBT_TOP/petalinux-v2013.04-final-full" "$SBT_PETALINUX" || exit 1
fi

echo "Initial PetaLinux setup and licenses:"
(cd "$SBT_PETALINUX"; . ./settings.sh)
ret=$?
echo "PetaLinux setup finished with exit code $ret"

if ! [ -e  "$SBT_PETALINUX/.installed" ]; then
	echo "PetaLinux setup not completed: you must accept the PetaLinux licenses to"
	echo "continue with the installation."
	exit 1
fi

if [ -L "$SBT_PETALINUX/software/linux-2.6.x" ]; then
	echo "PetaLinux kernel is already a symlink, no action taken"
elif [ -d "$SBT_PETALINUX/software/linux-2.6.x" ]; then
	mv "$SBT_PETALINUX/software/linux-2.6.x" \
	   "$SBT_PETALINUX/software/stock-linux-2.6.x"
	echo "PetaLinux-supplied kernel renamed"
fi
if ! [ -L "$SBT_PETALINUX/software/linux-2.6.x" ]; then
	ln -s ../../kernel/current "$SBT_PETALINUX/software/linux-2.6.x"
	echo "PetaLinux kernel replaced with symlink to SBT kernel"
fi

# Copy SBT entry into PetaLinux vendors tree - symlink doesn't work since part of the
# build process compares the realpath() to $PETALINUX and breaks the build
dst="$SBT_PETALINUX/software/petalinux-dist/vendors/SilverBulletTech"
src="$SBT_TOP/sbt/vendors/SilverBulletTech"
rm -rf "$dst"
cp -a "$src" "$dst"


exit 0

