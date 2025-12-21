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

#ifndef LUAT_ICONV_H
#define LUAT_ICONV_H

#include "luat_base.h"

/**
 * @defgroup luatos_iconv character encoding conversion interface
 * @{*/
typedef void *luat_iconv_t;

/**
 * @brief Open iconv conversion stream and allocate memory
 * @param to_code target encoding format, gb2312/ucs2/ucs2be/utf8
 * @param from_code source encoding format, gb2312/ucs2/ucs2be/utf8
 * @return cionv conversion stream*/
luat_iconv_t luat_iconv_open (const char *to_code, const char *from_code);

/**
 * @brief Convert encoding format
 * @param cd cionv conversion stream
 * @param inbuf input buffer
 * @param in_bytes_left input buffer length
 * @param outbuf output buffer
 * @param out_bytes_left output buffer length
 * @return 0 for success, -1 for failure*/
size_t luat_iconv_convert (luat_iconv_t cd, char ** inbuf, size_t * in_bytes_left, char ** outbuf, size_t * out_bytes_left);

/**
 * @brief Close iconv conversion stream and release memory
 * @param cd cionv conversion stream
 * @return 1 success, 0 failure*/
int luat_iconv_close (luat_iconv_t cd);

/**@}*/

#endif
