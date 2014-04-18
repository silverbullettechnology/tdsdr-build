#!/bin/sh
# Create an init script to auto-run the stress-test
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
# \copyright Copyright (C) 2013 Silver Bullet Technologies
# vim:ts=4:noexpandtab

[ "$2" = ""    ] && exit 0
[ "$2" -lt "1" ] && exit 0

echo "#!/bin/sh" > $1
echo "/usr/bin/sdrdc-stress-test.sh $2" >> $1
chmod +x $1

