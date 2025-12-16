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


#include "common_api.h"
#include "FreeRTOS.h"
#include "task.h"

#include "luat_base.h"
#include "rng.h"
#include "luat_crypto.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"
#include "mbedtls/des.h"
#include "mbedtls/aes.h"
#include "mbedtls/rsa.h"


void luat_crypto_md5( unsigned char *input, int ilen, unsigned char output[16] )
{
    mbedtls_md5_ret(input, ilen, output);
}

#ifndef __LUATOS__
/**
 * @brief BASE64 encryption
 * @param dst buffer
 * @param dlen buffer length
 * @param olen The number of bytes written
 * @param src encryption key
 * @param slen encryption key length
 * @return 0 success*/
int luat_crypto_base64_encode( unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen )
{
    mbedtls_base64_encode(dst, dlen, olen, src, slen);
}

/**
 * @brief BASE64 decryption
 * @param dst buffer
 * @param dlen buffer length
 * @param olen The number of bytes written
 * @param src key
 * @param slen key length
 * @return 0 success*/
int luat_crypto_base64_decode( unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen )
{
    mbedtls_base64_decode(dst, dlen, olen, src, slen);
}

#endif

/**
 * @brief Perform SHA1 verification
 * @param input input data
 * @param ilen input data length
 * @param output SHA1 verification value of the output*/
void luat_crypto_sha1(const unsigned char *input, size_t ilen, unsigned char output[20])
{
    mbedtls_sha1_ret(input, ilen, output);
}


/**
 * @brief perform SHA256 verification
 * @param input input data
 * @param ilen input data length
 * @param output SHA1 verification value of the output
 * @param is_224 Whether it is 224 check*/
void luat_crypto_sha256(const unsigned char *input, size_t ilen, unsigned char output[20], int is_224)
{
    mbedtls_sha256_ret(input, ilen, output, is_224);
}

#ifdef __LUATOS__
#define LUAT_LOG_TAG "crypto"
#include "luat_log.h"
#include "luat_mcu.h"
#endif

static uint8_t trng_wait;
static uint8_t trng_pool[24];

int luat_crypto_trng(char* buff, size_t len) {
    char* dst = buff;
    while (len > 0) {
        // No random values   left in the pool? Generate once
        if (trng_wait == 0) {
            // LLOGD("Generate a random number of 24 bytes and put it into the pool");
            rngGenRandom(trng_pool);
            trng_wait = 24;
        }
        // The remaining random values   are enough, copy them directly
        if (len <= trng_wait) {
            memcpy(dst, trng_pool + (24 - trng_wait), len);
            trng_wait -= len;
            return 0;
        }
        // If there is not enough, use up the existing ones first, and then cycle next
        memcpy(dst, trng_pool + (24 - trng_wait), trng_wait);
        dst += trng_wait;
        len -= trng_wait;
        trng_wait = 0;
    }
    return 0;
}
