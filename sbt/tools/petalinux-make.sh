#!/bin/sh
# Petalinux build wrapper script for SBT SDRDC 
# Copyright (C) 2014 Silver Bullet Technology

cd "$SBT_PETALINUX" || exit 0
. ./settings.sh
make $*

