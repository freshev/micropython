#include "common_api.h"
#include "luat_base.h"
#include "luat_pm.h"
#include "luat_msgbus.h"

#include "flash_rt.h"
#include "mem_map.h"


#define LUAT_LOG_TAG "sfd"
#include "luat_log.h"

int sfd_onchip_init (void* userdata) {
    return 0;
}

int sfd_onchip_status (void* userdata) {
    return 0;
}

int sfd_onchip_read (void* userdata, char* buf, size_t offset, size_t size) {
    int ret = 0;
    if (size == 0)
        return 0;
    ret = BSP_QSPI_Read_Safe((uint8_t *)buf, (FLASH_FDB_REGION_START + offset), size);
    return ret == 0 ? size : -1;
}

int sfd_onchip_write (void* userdata, const char* buf, size_t offset, size_t size) {
    int ret = 0;
    if (size == 0)
        return 0;
    // Note that the buf of BSP_QSPI_Write_Safe cannot be constant data on flash
    // XIP will be closed when writing to flash, resulting in the buf value being unreadable.
    //The following various judgments are to copy the constant data to ram and then write it
    uint8_t tmp_small[256]; //The reason why it is changed to 256 here is that fskv always writes in 256 bytes
    uint8_t *tmp = NULL;
    uint32_t addr = (uint32_t)buf;
    if (size <= 256) {
        // For smaller data, just copy it directly in the stack memory without judging
        memcpy(tmp_small, buf, size);
        ret = BSP_QSPI_Write_Safe((uint8_t *)tmp_small, (FLASH_FDB_REGION_START + offset), size);
    }
    else if (addr >= 0x00400000 && addr <= 0x00500000) {
        //The data is already in ram and can be written directly
        ret = BSP_QSPI_Write_Safe((uint8_t *)buf, (FLASH_FDB_REGION_START + offset), size);
    }
    else {
        // Constant data exceeding 256 bytes should not exist. The following logic is mainly defense code.
        tmp = malloc(size);
        if (tmp == NULL) {
            DBG("out of memory when malloc flash write buff");
            return -1;
        }
        memcpy(tmp, buf, size);
        ret = BSP_QSPI_Write_Safe((uint8_t *)tmp, (FLASH_FDB_REGION_START + offset), size);
        free(tmp);
    }
    return ret == 0 ? size : -1;
}
int sfd_onchip_erase (void* userdata, size_t offset, size_t size) {
    int ret = 0;
    ret = BSP_QSPI_Erase_Safe((FLASH_FDB_REGION_START + offset), size);
    return ret == 0 ? 0 : -1;
}

int sfd_onchip_ioctl (void* userdata, size_t cmd, void* buff) {
    return 0;
}

// #endif
