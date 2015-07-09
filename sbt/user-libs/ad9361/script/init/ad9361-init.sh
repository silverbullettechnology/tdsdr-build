#!/bin/sh
# Startup script for /usr/bin/ad9361 tool using ADI IIO driver
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

### NOTE: This script is not run by default.  To enable it, use the petalinux-config-apps
### tool, or edit the file
###   sbt/vendors/SilverBulletTech/TDSDR/config.vendor
### and set
###   CONFIG_USER_LIBS_AD9361_INIT_AUTOLOAD=y
###
### After changing this file run:
###   make setup reset
### to apply the change.  Note this makes mrproper in the Petalinux tree and may affect
### other config changes you've made.

# Run the ad9361 tool startup script for the Wimax example
/usr/bin/ad9361 /usr/lib/ad9361/script/startup_fdd-4msps-t2r2

# Enable FIR filters

#/usr/bin/ad9361 -d0 set_filter_fir_config             wb_wimax
#/usr/bin/ad9361 -d0 set_in_out_voltage_filter_fir_en  1
#/usr/bin/ad9361 -d0 set_filter_fir_config             wb_wimax
#/usr/bin/ad9361 -d0 get_filter_fir_config

# Setup AD1 for 4MSPS
#/usr/bin/ad9361 -d0 set_out_voltage_sampling_frequency 4000000
#/usr/bin/ad9361 -d0 get_out_voltage_sampling_frequency
#/usr/bin/ad9361 -d0 set_in_voltage_sampling_frequency  4000000
#/usr/bin/ad9361 -d0 get_in_voltage_sampling_frequency

# Enable FIR filters

#/usr/bin/ad9361 -d1 set_filter_fir_config             wb_wimax
#/usr/bin/ad9361 -d1 set_in_out_voltage_filter_fir_en  1
#/usr/bin/ad9361 -d1 set_filter_fir_config             wb_wimax
#/usr/bin/ad9361 -d1 get_filter_fir_config

# Setup AD1 for 4MSPS
#/usr/bin/ad9361 -d1 set_out_voltage_sampling_frequency 4000000
#/usr/bin/ad9361 -d1 get_out_voltage_sampling_frequency
#/usr/bin/ad9361 -d1 set_in_voltage_sampling_frequency  4000000
#/usr/bin/ad9361 -d1 get_in_voltage_sampling_frequency

# Set sleep mode to conserve power
#/usr/bin/ad9361 -d0 set_ensm_mode 0
#/usr/bin/ad9361 -d1 set_ensm_mode 0

