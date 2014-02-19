# Top-level Makefile AD9361 userspace driver, library, and tool
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
# vim:ts=4:noexpandtab

ifneq ($(PETALINUX),)
include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
else
include host.mk
endif

# version numbers
export MAJOR_VERSION := 0
export MINOR_VERSION := 1
export PATCH_VERSION := 0

# output names
export NAME   := ad9361
export ANAME  := lib$(NAME).a
export LDNAME := lib$(NAME).so
export SONAME := $(LDNAME).$(MAJOR_VERSION)
export SHARED := $(SONAME).$(MINOR_VERSION).$(PATCH_VERSION)

# basename of ADI legacy driver source - not redistributable
#export ADI := RC_06132013_Final

# location for output
export BIN := bin

# Separate sub makefiles for shared lib and executable tool
SUBMAKE := lib app

.PHONY: all install romfs clean image
all install romfs clean image:
	mkdir -p $(BIN)
	set -e; \
	for s in $(SUBMAKE); do \
		$(MAKE) -f $$s.mk $@; \
	done

