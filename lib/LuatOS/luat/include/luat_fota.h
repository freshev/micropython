#ifndef LUAT_FOTA_H
#define LUAT_FOTA_H

#include "luat_base.h"
#include "luat_spi.h"
/**
 * @defgroup luatos_fota remote upgrade interface
 * @{*/

/**
 * @brief is used to initialize fota and create a context structure for writing upgrade package data.
 *
 * @param start_address, start address 718/716 series, fill in 0
 * @param len length 718/716 series fill in 0
 * @param spi_device length 718/716 series fill in NULL
 * @param path length 718/716 series fill in NULL
 * @param pathlen length 718/716 series fill in 0
 * @return*/

int luat_fota_init(uint32_t start_address, uint32_t len, luat_spi_device_t* spi_device, const char *path, uint32_t pathlen);

/// @brief is used to write upgrade package data to local Flash
/// @param data upgrade package data
/// @param len upgrade packet data length
/// @return int =0 success, others failure
int luat_fota_write(uint8_t *data, uint32_t len);

/// @brief is used to end the upgrade package download
/// @return int =0 success, others failure
int luat_fota_done(void);

/**
 * @brief End the fota process
 *
 * @param is_ok whether to end the process
 * @return int =0 success, others failure*/
int luat_fota_end(uint8_t is_ok);

/// @brief Waiting for fota to be prepared, it has no effect currently
/// @param  
/// @return uint8_t =1 ready
uint8_t luat_fota_wait_ready(void);
/** @}*/
#endif
