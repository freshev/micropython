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


#include "common_api.h"

#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_mem.h"
#include "luat_debug.h"

#include "stdio.h"
#include "stdlib.h"

#include "flash_rt.h"
#include "mem_map.h"
extern void fotaNvmNfsPeInit(uint8_t isSmall);
int luat_flash_read(char* buff, size_t addr, size_t len) {
    int ret = 0;
    if (len == 0)
        return 0;
    ret = BSP_QSPI_Read_Safe((uint8_t *)buff, addr, len);
    return ret == 0 ? len : -1;
}

int luat_flash_write(char* buff, size_t addr, size_t len) {
    int ret = 0;
    if (len == 0)
        return 0;
    if (addr >= NVRAM_FACTORY_PHYSICAL_BASE)
    {
    	DBG("address %x", addr);
    	return -1;
    }
    // Note that the buf of BSP_QSPI_Write_Safe cannot be constant data on flash
    // XIP will be closed when writing to flash, resulting in the buf value being unreadable.
    //The following various judgments are to copy the constant data to ram and then write it
    uint8_t tmp_small[128];
    uint8_t *tmp = NULL;
    uint32_t buff_addr = (uint32_t)buff;
    if (len <= 128) {
        // For smaller data, just copy it directly in the stack memory without judging
        memcpy(tmp_small, buff, len);
        ret = BSP_QSPI_Write_Safe((uint8_t *)tmp_small, addr, len);
    }
    else if (buff_addr >= 0x00400000 && buff_addr <= 0x00500000) {
        //The data is already in ram and can be written directly
        ret = BSP_QSPI_Write_Safe((uint8_t *)buff, addr, len);
    }
    else {
        // Constant data exceeding 128 bytes should not exist. The following logic is mainly defense code.
        tmp = malloc(len);
        if (tmp == NULL) {
            LUAT_DEBUG_PRINT("out of memory when malloc flash write buff");
            return -1;
        }
        memcpy(tmp, buff, len);
        ret = BSP_QSPI_Write_Safe((uint8_t *)tmp, addr, len);
        free(tmp);
    }
    return ret == 0 ? len : -1;
}

int luat_flash_erase(size_t addr, size_t len) {
    int ret = 0;
    if (addr >= NVRAM_FACTORY_PHYSICAL_BASE)
    {
    	DBG("address %x", addr);
    	return -1;
    }
    ret = BSP_QSPI_Erase_Safe(addr, len);
    return ret == 0 ? 0 : -1;
}

int luat_flash_write_without_check(char* buff, size_t addr, size_t len) {
    if (addr >= NVRAM_FACTORY_PHYSICAL_BASE)
    {
    	DBG("address %x", addr);
    	return -1;
    }
	int ret = BSP_QSPI_Write_Safe((uint8_t *)buff, addr, len);
    return ret == 0 ? len : -1;
}

void luat_flash_ctrl_fw_sectors(uint8_t is_unlock)
{
	fotaNvmNfsPeInit(is_unlock);
}

