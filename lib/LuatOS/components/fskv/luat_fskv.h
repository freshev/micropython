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

#ifndef LUAT_FSKV_H
#define LUAT_FSKV_H

#include "lfs.h"



/**
 * @defgroup luatos_fskv persistent data storage interface
 * @{*/

/**
 * @brief Initialize kv data storage
 *
 * @return int == 0 normal != 0 failed*/
int luat_fskv_init(void);

/**
 * @brief Delete the specified key
 * @param key[IN] key value to be deleted
 * @return int == 0 normal != 0 failed*/
int luat_fskv_del(const char* key);

/**
 * @brief Write the data of the specified key
 * @param key[IN] The key value to be written, cannot be NULL, must end with \0, and the maximum length is 64 bytes
 * @param data[IN] The data to be written, does not need to end with \0
 * @param len[IN] The length of the data to be written, excluding \0, currently supports a maximum length of 255 bytes
 * @return actual written length*/
int luat_fskv_set(const char* key, void* data, size_t len);

/**
 * @brief Read key size
 * @param key key value, cannot be NULL, must end with \0, maximum length 64 bytes
 * @param buff buffer
 * @return length,<=0 failure*/
int luat_fskv_size(const char* key, char buff[4]);

/**
 * @brief Read the data of the specified key
 * @param key[IN] The key value to be read cannot be NULL and must end with \0
 * @param data[IN] The data to be read, the writable space must be greater than or equal to the len value
 * @param len[IN] The maximum length of the data to be read, excluding \0
 * @return int > 0 actual read length, <=0 failure*/
int luat_fskv_get(const char* key, void* data, size_t len);

/**
 * @brief Clear all data
 * @return int == 0 normal != 0 failed*/
int luat_fskv_clear(void);
/**
 * @brief Get kv database status
 * @param using_sz used space, unit bytes
 * @param max_sz total available space, in bytes
 * @param kv_count Total number of kv key-value pairs, unit
 * @return 0 success, others failure*/
int luat_fskv_stat(size_t *using_sz, size_t *max_sz, size_t *kv_count);
/**
 * @brief Read the data of the next offset
 * @param buff read data
 * @param offset offset
 * @return 0 success, others failure*/
int luat_fskv_next(char* buff, size_t offset);

/**
 * @}
 */

#endif
