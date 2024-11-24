/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef LUAT_flash_H
#define LUAT_flash_H
#include "luat_base.h"
/**
 * @defgroup luatos_flash on-chip Flash operation
 * @{*/

/**
 * @brief Read Flash data in the specified area
 *
 * @param buff[OUT] read data
 * @param addr offset, related to the specific device
 * @param len read length
 * @return int <= 0 error >0 actual read size*/
int luat_flash_read(char* buff, size_t addr, size_t len);

/**
 * @brief writes flash data in the specified area
 *
 * @param buff[IN] written data
 * @param addr offset, related to the specific device
 * @param len writing length
 * @return int <= 0 error >0 actual written size*/
int luat_flash_write(char* buff, size_t addr, size_t len);

/**
 * @brief Erase flash data in the specified area
 *
 * @param addr offset, related to the specific device
 * @param len erasure length, usually the area size, such as 4096
 * @return int != 0 error =0 normal*/
int luat_flash_erase(size_t addr, size_t len);

/**
 * @brief writes the flash data in the specified area and does not perform security checks on the buff.
 *
 * @param buff[IN] written data
 * @param addr offset, related to the specific device
 * @param len writing length
 * @return int <= 0 error >0 actual written size*/
int luat_flash_write_without_check(char* buff, size_t addr, size_t len);
/**
 * @brief Unlock part of the protected space in the firmware area and open it to users for read and write operations. It is currently the last 896KB of the program area and the starting address is 0x00224000.
 *
 * @param is_unlock 0 lock, other unlock
 * @return int != 0 error =0 normal*/
void luat_flash_ctrl_fw_sectors(uint8_t is_unlock);
/**
 * @}
 */
#endif
