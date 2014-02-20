#!/bin/sh
# Petalinux vendor setup script for SBT SDRDC 
# Copyright (C) 2014 Silver Bullet Technology

# Copy SBT entry into PetaLinux vendors tree - symlink doesn't work since part of the
# build process compares the realpath() to $PETALINUX and breaks the build
dst="$SBT_PETALINUX/software/petalinux-dist/vendors/SilverBulletTech"
src="$SBT_TOP/sbt/vendors/SilverBulletTech"
rm -rf "$dst"
cp -a "$src" "$dst"

exit 0
