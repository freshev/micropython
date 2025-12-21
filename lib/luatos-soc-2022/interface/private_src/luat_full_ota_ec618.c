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

#include "luat_base.h"
#include "luat_mcu.h"
#include "luat_spi.h"
#include "luat_full_ota.h"
#include "common_api.h"
#include "mem_map.h"
extern void fotaNvmNfsPeInit(uint8_t isSmall);
#define __SOC_OTA_COMMON_DATA_LOAD_ADDRESS__    (AP_FLASH_LOAD_ADDR+0x1F0000)
#define __SOC_OTA_COMMON_DATA_SAVE_ADDRESS__    (AP_FLASH_LOAD_ADDR+0x1F0000 - AP_FLASH_XIP_ADDR)


#if MBEDTLS_VERSION_NUMBER >= 0x03000000
#define mbedtls_md5_starts_ret mbedtls_md5_starts
#define mbedtls_md5_update_ret mbedtls_md5_update
#define mbedtls_md5_finish_ret mbedtls_md5_finish
#endif


#if MBEDTLS_VERSION_NUMBER >= 0x03000000
/*int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 ) {
    mbedtls_sha256_starts(ctx, is224);
}
int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen ) {
    mbedtls_sha256_update(ctx, input, ilen);
}
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32]) {
    mbedtls_sha256_finish(ctx, output);
}*/
#endif

static void luat_full_ota_finish(luat_full_ota_ctrl_t *fota)
{
    CoreUpgrade_HeadCalMD5Struct Head = {0};
    PV_Union uPV;
    Head.MaigcNum = __APP_START_MAGIC__;
    Head.DataStartAddress = __SOC_OTA_COMMON_DATA_LOAD_ADDRESS__;
    uPV.u8[0] = CORE_OTA_MODE_FULL;
    uPV.u8[1] = CORE_OTA_IN_FLASH;
    Head.Param1 = uPV.u32;
    Head.DataLen = fota->fota_file_head.CommonDataLen;
    memcpy(Head.MainVersion, fota->fota_file_head.MainVersion, sizeof(Head.MainVersion));
    memcpy(Head.CommonMD5, fota->fota_file_head.CommonMD5, 16);
    Head.CRC32 = CRC32_Cal(fota->crc32_table, (uint8_t *)&Head.Param1, sizeof(Head) - 8, 0xffffffff);
    BSP_QSPI_Erase_Safe(__SOC_OTA_INFO_DATA_SAVE_ADDRESS__, __FLASH_SECTOR_SIZE__);
    BSP_QSPI_Write_Safe((uint8_t *)&Head, __SOC_OTA_INFO_DATA_SAVE_ADDRESS__, sizeof(Head));
    fota->ota_state = OTA_STATE_OK;
    OS_DeInitBuffer(&fota->data_buffer);
    DBG("full ota save ok!, wait reboot");
}

luat_full_ota_ctrl_t *luat_full_ota_init(uint32_t start_address, uint32_t len, void* spi_device, const char *path, uint32_t pathlen)
{
    luat_full_ota_ctrl_t *fota = calloc(1, sizeof(luat_full_ota_ctrl_t));
    if (fota)
    {
        CRC32_CreateTable(fota->crc32_table, CRC32_GEN);
    }
    else
    {
        return NULL;
    }
    fota->ota_state = OTA_STATE_IDLE;
    BSP_QSPI_Erase_Safe(__SOC_OTA_INFO_DATA_SAVE_ADDRESS__, __FLASH_SECTOR_SIZE__);
    OS_ReInitBuffer(&fota->data_buffer, __FLASH_SECTOR_SIZE__ * 4);
    return fota;
}

int luat_full_ota_write(luat_full_ota_ctrl_t *fota, uint8_t *data, uint32_t len)
{
    uint32_t save_len;
    OS_BufferWrite(&fota->data_buffer, data, len);
REPEAT:
    switch(fota->ota_state)
    {
    case OTA_STATE_IDLE:
        if (fota->data_buffer.Pos > sizeof(CoreUpgrade_FileHeadCalMD5Struct))
        {
            memcpy(&fota->fota_file_head, fota->data_buffer.Data, sizeof(CoreUpgrade_FileHeadCalMD5Struct));
            OS_BufferRemove(&fota->data_buffer, sizeof(CoreUpgrade_FileHeadCalMD5Struct));
            if (fota->fota_file_head.MaigcNum != __APP_START_MAGIC__)
            {
                DBG("magic num error %x", fota->fota_file_head.MaigcNum);
                fota->data_buffer.Pos = 0;
                return -1;
            }
            uint32_t crc32 = CRC32_Cal(fota->crc32_table, (uint8_t*)fota->fota_file_head.MainVersion, sizeof(CoreUpgrade_FileHeadCalMD5Struct) - 8, 0xffffffff);
            if (crc32 != fota->fota_file_head.CRC32)
            {
                DBG("file head crc32 error %x,%x", crc32, fota->fota_file_head.CRC32);
                fota->data_buffer.Pos = 0;
                return -1;
            }
            fota->ota_done_len = 0;
            mbedtls_md5_init(&fota->md5_ctx);
            mbedtls_md5_starts_ret(&fota->md5_ctx);
            if (fota->fota_file_head.CommonDataLen)
            {
                fota->ota_state = OTA_STATE_WRITE_COMMON_DATA;
                fotaNvmNfsPeInit(1);
                DBG("write full ota data");
                goto REPEAT;
            }
            else
            {
                fota->ota_state = OTA_STATE_ERROR;
                return -1;
            }
        }
        break;
    case OTA_STATE_WRITE_COMMON_DATA:
        save_len = ((fota->ota_done_len + __FLASH_SECTOR_SIZE__) < fota->fota_file_head.CommonDataLen)?__FLASH_SECTOR_SIZE__:(fota->fota_file_head.CommonDataLen - fota->ota_done_len);
        if (fota->data_buffer.Pos >= save_len)
        {
            BSP_QSPI_Erase_Safe(__SOC_OTA_COMMON_DATA_SAVE_ADDRESS__ + fota->ota_done_len, __FLASH_SECTOR_SIZE__);
            BSP_QSPI_Write_Safe(fota->data_buffer.Data, __SOC_OTA_COMMON_DATA_SAVE_ADDRESS__ + fota->ota_done_len, save_len);
            size_t loadaddr = __SOC_OTA_COMMON_DATA_LOAD_ADDRESS__;
            mbedtls_md5_update_ret(&fota->md5_ctx, (uint8_t *)(loadaddr + fota->ota_done_len), save_len );
            OS_BufferRemove(&fota->data_buffer, save_len);
        }
        else
        {
            break;
        }
        fota->ota_done_len += save_len;
        if (fota->ota_done_len >= fota->fota_file_head.CommonDataLen)
        {
            DBG("full ota data done %ubyte, now checking", fota->ota_done_len);
            fotaNvmNfsPeInit(0);
            uint8_t md5[16];
            mbedtls_md5_finish_ret(&fota->md5_ctx, md5);
            if (memcmp(md5, fota->fota_file_head.CommonMD5, 16))
            {
                DBG("full ota data md5 check failed");
                fota->ota_state = OTA_STATE_ERROR;
                fota->data_buffer.Pos = 0;
                return -1;
            }
            else
            {
                DBG("full ota data md5 ok");
                luat_full_ota_finish(fota);
                return 0;
            }
        }
        else
        {
            goto REPEAT;
        }
        break;
    case OTA_STATE_OK:
        return 0;
    default:
        return -1;
        break;
    }
    return 1;
}

int luat_full_ota_is_done(luat_full_ota_ctrl_t *fota)
{
    switch(fota->ota_state)
    {
    case OTA_STATE_IDLE:
        return -1;
    case OTA_STATE_OK:
        return 0;
    default:
        return 1;
    }
    return 1;
}

void luat_full_ota_end(luat_full_ota_ctrl_t *fota, uint8_t is_ok)
{
    if (fota->ota_state != OTA_STATE_OK)
    {
        OS_DeInitBuffer(&fota->data_buffer);
        BSP_QSPI_Erase_Safe(__SOC_OTA_INFO_DATA_SAVE_ADDRESS__, __FLASH_SECTOR_SIZE__);
        DBG("full ota failed");
    }
}
