/** \file      src/lib/ad9361_sysfs.h
 *  \brief     Private sysfs functions shared with debugfs
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
 *
 *             Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *             use this file except in compliance with the License.  You may obtain a copy
 *             of the License at: 
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *             Unless required by applicable law or agreed to in writing, software
 *             distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *             WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 *             License for the specific language governing permissions and limitations
 *             under the License.
 *
 * vim:ts=4:noexpandtab
 */
#ifndef _SRC_LIB_AD9361_SYSFS_H_
#define _SRC_LIB_AD9361_SYSFS_H_


int ad9361_sysfs_get_enum (unsigned dev, const char *root, const char *leaf,
                           const char **list, int *val);
int ad9361_sysfs_set_enum (unsigned dev, const char *root, const char *leaf,
                           const char **list, int val);

int ad9361_sysfs_get_int (unsigned dev, const char *root, const char *leaf, int      *val);
int ad9361_sysfs_get_u32 (unsigned dev, const char *root, const char *leaf, uint32_t *val);
int ad9361_sysfs_get_u64 (unsigned dev, const char *root, const char *leaf, uint64_t *val);
int ad9361_sysfs_set_int (unsigned dev, const char *root, const char *leaf, int       val);
int ad9361_sysfs_set_u32 (unsigned dev, const char *root, const char *leaf, uint32_t  val);
int ad9361_sysfs_set_u64 (unsigned dev, const char *root, const char *leaf, uint64_t  val);


#endif /* _SRC_LIB_AD9361_SYSFS_H_ */
