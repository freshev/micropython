#ifndef __LUAT_FULL_OTA_H__
#define __LUAT_FULL_OTA_H__
#include "bsp_common.h"
#include "platform_define.h"
#include "mbedtls/md5.h"

typedef struct
{
	Buffer_Struct data_buffer;
	mbedtls_md5_context md5_ctx;
	CoreUpgrade_FileHeadCalMD5Struct fota_file_head;
	uint32_t crc32_table[256];
	uint32_t ota_done_len;
	uint8_t ota_state;
}luat_full_ota_ctrl_t;


enum
{
	OTA_STATE_IDLE,
	OTA_STATE_WRITE_COMMON_DATA,
	OTA_STATE_OK,
	OTA_STATE_ERROR,
};

/**
 * @defgroup luatos_full_ota full package upgrade interface
 * @{*/

/**
 * @brief is used to initialize fota and create a context structure for writing upgrade package data.
 *
 * @param All params are invalid
 * @return void *fota handle*/
luat_full_ota_ctrl_t *luat_full_ota_init(uint32_t start_address, uint32_t len, void* spi_device, const char *path, uint32_t pathlen);
/**
 * @brief is used to write upgrade package data to local Flash
 *
 * @param handle - fota handle
 * @param data - upgrade package data address
 * @param len - Upgrade packet data length. Unit: Bytes
 * @return int =0 success, others failure;*/
int luat_full_ota_write(luat_full_ota_ctrl_t *handle, uint8_t *data, uint32_t len);

/**
 * @brief Check whether the upgrade package is written completely
 *
 * @param handle - fota handle
 * @return int =0 success, >0 more data needs to be written <0 exception occurred and failed*/
int luat_full_ota_is_done(luat_full_ota_ctrl_t *handle);

/**
 * @brief is used to end the upgrade package download
 *
 * @param handle - fota handle
 * @param is_ok obsolete and invalid
 * @return int =0 success, others failure*/
void luat_full_ota_end(luat_full_ota_ctrl_t *handle, uint8_t is_ok);
/** @}*/
#endif
