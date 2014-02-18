#!/bin/sh
# Petalinux setup wrapper script for SBT SDRDC 
# Copyright (C) 2014 Silver Bullet Technology

[ -z "$SBT_TOP" ] && exit 1

if [ -e "$SBT_PETALINUX/.sbt-patched" ]; then
	echo "PetaLinux already installed and patched, no action taken."
	exit 0
fi

if [ -z "$SBT_PETALINUX_SDK" ]; then
	echo "No PetaLinux SDK installation path:"
	echo "Please edit Config and point SBT_PETALINUX_SDK at the PetaLinux SDK installation"
	echo "archive downloaded from http://www.xilinx.com/"
	exit 1
elif [ "$SBT_PETALINUX_VER" != "2013.04" ]; then
	echo "Supported PetaLinux version is 2013.04.  Version 2013.10 support is coming"
	echo "soon, but for now please download version 2013.04."
	exit 1
elif ! [ -r "$SBT_PETALINUX_SDK" ]; then
	echo "Unable to read PetaLinux SDK installation archive:"
	echo "  $SBT_PETALINUX_SDK"
	echo "is nonexistant or unreadable.  Please check the path and file permissions and"
	echo "try again."
	exit 1
fi

if [ -e "$SBT_PETALINUX" ]; then
	while true; do
		echo "PetaLinux directory exists but is not yet patched"
		echo ". If you cancelled during the licensing step, you can enter 'continue' at the"
		echo "  prompt below to continue the licensing and patching steps."
		echo ". If you want to reset the PetaLinux tree, you can enter 'reset' at the prompt"
		echo "  below.  The PetaLinux tree will be deleted and reinstalled from the install-"
		echo "  ation tarball.  YOU WILL LOSE ANY CHANGES MADE TO THE PETALINUX FILES."
		echo "  Press Enter to stop here without making changes."
		read rep
		if [ "$rep" = "continue" ]; then
			echo "Continuing with the installation."
			break
		elif [ "$rep" = "reset" ]; then
			echo "Resetting the PetaLinux tree..."
			rm -rf "$SBT_PETALINUX"
			break
		else
			echo "Stopping here."
			exit 1
		fi
	done
fi

/bin/sh "$SBT_TOOLS/setup-petalinux-${SBT_PETALINUX_VER}.sh" || exit 1

(cd "$SBT_FILES/petalinux-$SBT_PETALINUX_VER" && cp -av * "$SBT_PETALINUX")

ls -1 "$SBT_PATCHES/petalinux-$SBT_PETALINUX_VER" | while read p; do
	if ! cat "$SBT_PATCHES/petalinux-$SBT_PETALINUX_VER/$p" | \
	patch --strip=1 --verbose --directory="$SBT_PETALINUX"; then
		echo "Patching PetaLinux SDK failed, stopping here."
		exit 1
	fi
done

touch "$SBT_PETALINUX/.sbt-patched" 

exit 0

