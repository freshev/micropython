/*
 * luat_lib_fatfs.h
 *
 *  Created on: 2023年7月18日
 *      Author: Administrator
 */

#ifndef FATFS_LUAT_LIB_FATFS_H_
#define FATFS_LUAT_LIB_FATFS_H_

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
int luat_fatfs_mount(uint8_t disk_mode, uint32_t device, uint8_t cs_pin, uint32_t speed, uint8_t power_pin, uint16_t power_on_delay, const char *mount_point, const char *vfs_mount_point);
int luat_fatfs_unmount(const char *mount_point);
int luat_fatfs_format(const char *mount_point);
void luat_fatfs_debug_mode(uint8_t on_off);
#endif /* FATFS_LUAT_LIB_FATFS_H_ */
