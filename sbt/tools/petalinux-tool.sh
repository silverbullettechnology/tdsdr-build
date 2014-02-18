#!/bin/sh
# Petalinux build wrapper script for SBT SDRDC 
# Copyright (C) 2014 Silver Bullet Technology

tool="petalinux-$1"
shift

cd "$SBT_PETALINUX"
. ./settings.sh
$tool $*

