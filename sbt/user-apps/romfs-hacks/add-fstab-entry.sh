#!/bin/sh
# Filesystem tweak: add an entry to /etc/fstab if not already present
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

fstab=$ROMFSDIR/etc/fstab

if grep -q "^$1" $fstab; then
	echo "$1 already listed in $fstab"
	exit 0
fi

echo $* >> $fstab
echo "$1 added to $fstab"

