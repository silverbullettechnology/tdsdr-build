# Host Makefile for CAL_CTL userspace tool
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
export CONFIG_USER_LIBS_CAL_CTL_INCLUDES := y
export CONFIG_USER_LIBS_CAL_CTL_STATIC   := y
export CONFIG_USER_LIBS_CAL_CTL_READLINE := y
export CONFIG_LINUXDIR            := /lib/modules/$(shell uname -r)/build
export STAGEDIR                   := stage
export CFLAGS                     += -Istage/usr/include
export HOST_CFLAGS                += -DSIM_HAL
