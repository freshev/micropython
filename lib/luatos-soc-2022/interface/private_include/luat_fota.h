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

#ifndef LUAT_FOTA_H
#define LUAT_FOTA_H
/**
 * @defgroup luatos_fota remote upgrade interface
 * @{*/


/**
 * @brief context structure of upgrade package data
 **/
typedef void* luat_fota_img_proc_ctx_ptr;


/**
 * @brief is used to initialize fota and create a context structure for writing upgrade package data.
 *
 * @param
 * @return luat_fota_img_proc_ctx_ptr*/
luat_fota_img_proc_ctx_ptr luat_fota_init(void);

/**
 * @brief is used to write upgrade package data to local Flash
 *
 * @param context - context structure pointer
 * @param data - upgrade package data address
 * @param len - Upgrade packet data length. Unit: Bytes
 * @return int =0 success, others failure;*/
int luat_fota_write(luat_fota_img_proc_ctx_ptr context, void * data, int len);

/**
 * @brief is used to end the upgrade package download
 *
 * @param context - context structure pointer
 * @return int =0 success, others failure*/
int luat_fota_done(luat_fota_img_proc_ctx_ptr context);
/** @}*/
#endif
