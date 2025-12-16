/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2023 Damien P. George
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

#include <sys/time.h>

#include "py/obj.h"
#include "shared/timeutils/timeutils.h"
#include "luat_debug.h"

// Return the localtime as an 8-tuple.
static mp_obj_t mp_time_localtime_get(void) {
    struct tm tb1 = {0};
    struct tm *tb2 = NULL;
    luat_rtc_get(&tb1);
    time_t nowtime;
    time(&nowtime);
    tb2 = localtime(&nowtime);
    // LUAT_DEBUG_PRINT("time ISO %d-%d-%dT%d:%d:%d", tb1.tm_year, tb1.tm_mon, tb1.tm_mday, tb1.tm_hour, tb1.tm_min, tb1.tm_sec);
    mp_obj_t tuple[8] = {
        tuple[0] = mp_obj_new_int(1900 + tb2->tm_year),
        tuple[1] = mp_obj_new_int(tb2->tm_mon),
        tuple[2] = mp_obj_new_int(tb2->tm_mday),
        tuple[3] = mp_obj_new_int(tb2->tm_hour),
        tuple[4] = mp_obj_new_int(tb2->tm_min),
        tuple[5] = mp_obj_new_int(tb2->tm_sec),
        tuple[6] = mp_obj_new_int(tb2->tm_wday),
        tuple[7] = mp_obj_new_int(tb2->tm_yday),
    };
    return mp_obj_new_tuple(8, tuple);
}

// Return the number of seconds since the Epoch.
static mp_obj_t mp_time_time_get(void) {
    struct tm tb1 = {0};
    luat_rtc_get(&tb1);
    time_t utctime = mktime(&tb1);
    return mp_obj_new_int(utctime);
}
