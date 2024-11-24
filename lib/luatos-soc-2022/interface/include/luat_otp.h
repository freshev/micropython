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


#ifndef LUAT_OTP_H
#define LUAT_OTP_H
#include "luat_base.h"
/**
 * @defgroup luatos_otp OTP operation
 * @{*/

/**
 * @brief Read the OTP data of the specified area
 *
 * @param zone OTP zone, usually an integer such as 1, 2, 3, etc., related to the device
 * @param buff[OUT] read data
 * @param offset offset, usually 0
 * @param len read length, offset+len cannot exceed the length of the OTP area
 * @return int <= 0 error >0 actual outgoing size*/
int luat_otp_read(int zone, char* buff, size_t offset, size_t len);

/**
 * @brief Write OTP data in the specified area
 *
 * @param zone OTP zone, usually an integer such as 1, 2, 3, etc., related to the device
 * @param buff[IN] written data
 * @param offset offset, usually 0
 * @param len read length, offset+len cannot exceed the length of the OTP area
 * @return int <= 0 error >0 actual written size*/
int luat_otp_write(int zone, char* buff, size_t offset, size_t len);
/**
 * @brief Erase OTP data in the specified area
 *
 * @param zone OTP zone, usually an integer such as 1, 2, 3, etc., related to the device
 * @param offset offset, usually 0, most devices only support erasing the entire area, this parameter is invalid
 * @param len The erasure length, usually the area size. If the device only supports normal erasure, this parameter is invalid
 * @return int != 0 error =0 normal*/
int luat_otp_erase(int zone, size_t offset, size_t len);
/**
 * @brief The OTP data in the specified area is locked and cannot be written or erased.
 *
 * @param zone OTP zone, usually an integer such as 1, 2, 3, etc., related to the device
 * @return int != 0 error =0 normal*/
int luat_otp_lock(int zone);
/**
 * @brief OTP area size of the specified area
 *
 * @param zone OTP zone, usually an integer such as 1, 2, 3, etc., related to the device
 * @return int == 0 no such area, > 0 area size, for example 256, 512*/
size_t luat_otp_size(int zone);


/**
 * @}
 */
#endif
