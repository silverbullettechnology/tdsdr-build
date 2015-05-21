# Scripts and datafiles Makefile
#
# Copyright 2014,2015 Silver Bullet Technology
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

.PHONY: all install clean

all:
	true

install:
	mkdir -p $(DESTDIR)/etc/rcS.d $(DESTDIR)/usr/lib
	-cp -av etc/* $(DESTDIR)/etc
	-cp -av lib/* $(DESTDIR)/usr/lib
	rm -f $(DESTDIR)/etc/rcS.d/S91vita49-agent
ifeq ($(CONFIG_USER_APPS_VITA49_AGENT_AUTOLOAD),y)
	ln -sf ../init.d/vita49-agent $(ROMFSDIR)/etc/rcS.d/S91vita49-agent
endif

clean:
	true

