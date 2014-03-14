#!/bin/sh
# Filesystem tweak: add network intefaces to /etc/network/interfaces
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

interfaces=$ROMFSDIR/etc/network/interfaces

if grep -q "$1" $interfaces; then
	echo "$1 already added to $interfaces"
	exit 0
fi

echo "auto $1"            >> $interfaces
echo "iface $1 inet dhcp" >> $interfaces

