
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
 * @brief Get the starting address and length of kv
 * @param len kv size, related to the specific device
 * @return size_t = 0 error !=0 normal*/
size_t luat_flash_get_fskv_addr(size_t *len);

/**
 * @}
 */
#endif
