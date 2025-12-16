/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Josef Gajdusek
 * Copyright (c) 2016 Paul Sokolovsky
 * Copyright (c) 2024 freshev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// This file is never compiled standalone, it's included directly from
// extmod/modos.c via MICROPY_PY_OS_INCLUDEFILE.

#include "extmod/modmachine.h"
#include "iot_os.h"

static uint8_t trng_wait;
static uint8_t trng_pool[24];


void rngGenRandom(uint8_t Rand[24]) {
     iot_os_srand(iot_os_get_system_tick()); // Set random number seed interface
     for(int i = 0; i < 6; i++) {
        uint32_t rv = iot_os_rand(); // Get random number interface
     	Rand[i * 4] = rv & 0xFF; 
     	Rand[i * 4 + 1] = (rv >> 8) & 0xFF; 
     	Rand[i * 4 + 2] = (rv >> 16) & 0xFF; 
     	Rand[i * 4 + 3] = (rv >> 24) & 0xFF; 
     }
}

int iot_crypto_trng(char* buff, size_t len) {
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

static const char *mp_os_uname_release(void) {
    return FW_VERSION;
}

static mp_obj_t mp_os_urandom(mp_obj_t num) {
    mp_int_t n = mp_obj_get_int(num);
    vstr_t vstr;
    vstr_init_len(&vstr, n);
    iot_crypto_trng(vstr.buf, n);
    return mp_obj_new_bytes_from_vstr(&vstr);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_os_urandom_obj, mp_os_urandom);


