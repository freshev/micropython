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

#ifndef Luat_CRYPTO_H
#define Luat_CRYPTO_H

#include "luat_base.h"

/**
 * @defgroup luatos_crypto crypto data encryption
 * @{*/

/**
 * \brief perform MD5 check
 * \param input input data
 * \param ilen input data length
 * \param output MD5 test value of output*/
void luat_crypto_md5( unsigned char *input, int ilen, unsigned char output[16] );


/**
 * @brief BASE64 encryption
 * @param dst buffer
 * @param dlen buffer length
 * @param olen The number of bytes written
 * @param src encryption key
 * @param slen encryption key length
 * @return 0 success*/
int luat_crypto_base64_encode( unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen );

/**
 * @brief BASE64 decryption
 * @param dst buffer
 * @param dlen buffer length
 * @param olen The number of bytes written
 * @param src key
 * @param slen key length
 * @return 0 success*/
int luat_crypto_base64_decode( unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen );

/**
 * @brief Perform SHA1 verification
 * @param input input data
 * @param ilen input data length
 * @param output SHA1 verification value of the output*/
void luat_crypto_sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);


/**
 * @brief perform SHA256 verification
 * @param input input data
 * @param ilen input data length
 * @param output SHA1 verification value of the output
 * @param is_224 Whether it is 224 check*/
void luat_crypto_sha256(const unsigned char *input, size_t ilen, unsigned char output[20], int is_224);

/**
 * @brief generate random numbers
 * @param buff random number storage address
 * @param ilen random number length*/
int luat_crypto_trng(char* buff, size_t len);

/**@}*/ 
#endif
