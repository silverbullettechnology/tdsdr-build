#!/bin/sh
# Filesystem tweak: add entries to $PATH in /etc/profile if not already present
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

profile=$ROMFSDIR/etc/profile

if grep -q "PATH.*ad9361" $profile; then
	echo "$1 already listed in PATH"
	exit 0
fi

echo "# Add entry for AD9361 scripts" >> $profile
echo -n "PATH=\$PATH" >> $profile
[ -n "$1" ] && echo -n ":/media/card/script/$1" >> $profile
echo -n ":/media/card/script" >> $profile
[ -n "$1" ] && echo -n ":/usr/lib/ad9361/script/$1" >> $profile
echo ":/usr/lib/ad9361/script" >> $profile
echo "$1 added to $profile"

