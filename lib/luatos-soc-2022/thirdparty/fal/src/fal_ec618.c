
#include "common_api.h"
#include "FreeRTOS.h"
#include "task.h"

#include "lfs_port.h"
#include "rng.h"
#include "lfs_util.h"
#include "flash_rt.h"

#include "mem_map.h"
#include DEBUG_LOG_HEADER_FILE
#include "osasys.h"
#include "plat_config.h"

#include "fal.h"

#define BLOCK_SIZE (4096)
#define TOTOL_SIZE (64*1024)

#define MEM_MODE 0

#if MEM_MODE
static uint8_t vbuff[TOTOL_SIZE];
static int (onchip_flash_init)(void) {
    memset(vbuff, 0, TOTOL_SIZE);
    return 0;
}
static int (onchip_flash_read)(long offset, uint8_t *buf, size_t size) {
    if (size == 0)
        return 0;
    memcpy(buf, vbuff + offset, size);
    return size;
}

static int (onchip_flash_write)(long offset, const uint8_t *buf, size_t size) {
    if (size == 0)
        return 0;
    memcpy(vbuff + offset, buf, size);
    return size;
}

static int (onchip_flash_erase)(long offset, size_t size) {
    memset(vbuff + offset, 0, size);
    return 0;
}

#else // MEM_MODE == 0

// static void mem_diff(const char* tag) {
//     int ret = 0;
//     uint8_t *tmp = malloc(BLOCK_SIZE);
//     for (size_t offset = 0; offset < TOTOL_SIZE; offset+=BLOCK_SIZE)
//     {
//         ret = BSP_QSPI_Read_Safe((uint8_t *)tmp, (FLASH_FDB_REGION_START + offset), BLOCK_SIZE);
//         if (ret != 0) {
//             DBG("%s BSP_QSPI_Read_Safe ret %d %04X", tag, ret, offset);
//             break;
//         }
//         if (memcmp(tmp, vbuff + offset, BLOCK_SIZE)) {
//             DBG("%s Data NOT Match %04X", tag, offset);
//             break;
//         }
//     }
//     free(tmp);
// }

// extern uint8_t* QSPI_FLASH_ReadUUID(void);

static int (onchip_flash_init)(void) {
    return 0;
}
static int (onchip_flash_read)(long offset, uint8_t *buf, size_t size) {
    int ret = 0;
    if (size == 0)
        return 0;
    ret = BSP_QSPI_Read_Safe((uint8_t *)buf, (FLASH_FDB_REGION_START + offset), size);
    return ret == 0 ? size : -1; // Pay special attention to the size to be read when successful!!!
}

static int (onchip_flash_write)(long offset, const uint8_t *buf, size_t size) {
    int ret = 0;
    if (size == 0)
        return 0;
    // Note that the buf of BSP_QSPI_Write_Safe cannot be constant data on flash
    // XIP will be closed when writing to flash, resulting in the buf value being unreadable.
    //The following various judgments are to copy the constant data to ram and then write it
    uint8_t tmp_small[128];
    uint8_t *tmp = NULL;
    uint32_t addr = (uint32_t)buf;
    if (size <= 128) {
        // For smaller data, just copy it directly in the stack memory without judging
        memcpy(tmp_small, buf, size);
        ret = BSP_QSPI_Write_Safe((uint8_t *)tmp_small, (FLASH_FDB_REGION_START + offset), size);
    }
    else if (addr >= 0x00400000 && addr <= 0x00500000) {
        //The data is already in ram and can be written directly
        ret = BSP_QSPI_Write_Safe((uint8_t *)buf, (FLASH_FDB_REGION_START + offset), size);
    }
    else {
        // Constant data exceeding 128 bytes should not exist. The following logic is mainly defense code.
        tmp = malloc(size);
        if (tmp == NULL) {
            DBG("out of memory when malloc flash write buff");
            return -1;
        }
        memcpy(tmp, buf, size);
        ret = BSP_QSPI_Write_Safe((uint8_t *)tmp, (FLASH_FDB_REGION_START + offset), size);
        free(tmp);
    }
    return ret == 0 ? size : -1; // Pay special attention to the size to be written when successful!!!
}

static int (onchip_flash_erase)(long offset, size_t size) {
    int ret = 0;
    ret = BSP_QSPI_Erase_Safe((FLASH_FDB_REGION_START + offset), size);
    return ret == 0 ? 0 : -1;
}

#endif // end of MEM_MODE

const struct fal_flash_dev onchip_flash = {
    .name = "onchip_flash",
    .len = TOTOL_SIZE,
    .blk_size = BLOCK_SIZE,
    .addr = 0,
    .write_gran = 32,
    .ops = {
        .init  = onchip_flash_init, 
        .read  = onchip_flash_read,
        .write = onchip_flash_write,
        .erase = onchip_flash_erase
    }
};


