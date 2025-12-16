/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 pulkin
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

#include "mpconfigport.h"

#include "time.h"
#include "stdbool.h"
#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"
#include "api_hal_pm.h"
#include "api_hal_uart.h"
#include "buffer.h"
#include "uart.h"

#include "py/runtime.h"
#include "py/stream.h"
#include "extmod/misc.h"

int uart_attached_to_dupterm[UART_NPORTS];
static uint8_t stdin_ringbuf_array[2048];
ringbuf_t stdin_ringbuf = {stdin_ringbuf_array, sizeof(stdin_ringbuf_array), 0, 0};

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    if ((poll_flags & MP_STREAM_POLL_RD) && stdin_ringbuf.iget != stdin_ringbuf.iput) {
        ret |= MP_STREAM_POLL_RD;
    }
    if (poll_flags & MP_STREAM_POLL_WR) {
        ret |= mp_os_dupterm_poll(poll_flags);
    }
    return ret;
}

int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        int c = ringbuf_get(&stdin_ringbuf);
        if (c != -1) {
            return c;
        }
        // OS_Sleep(1); // need not comment ?
        MICROPY_EVENT_POLL_HOOK
    }
}


mp_uint_t mp_hal_stdout_tx_strn(const char *str, uint32_t len) {
    int dupterm_res = mp_os_dupterm_tx_strn(str, len);
    if (dupterm_res < 0) {
        // no outputs, nothing was written
        return 0;
    } else {
        return dupterm_res;
    }
}

uint32_t mp_hal_ticks_ms(void)    { return (uint32_t)(clock() / CLOCKS_PER_MSEC            ); }
uint32_t mp_hal_ticks_us(void)    { return (uint32_t)(clock() / CLOCKS_PER_MSEC * 1000.0   ); }
uint64_t mp_hal_ticks_ms_64(void) { return (uint64_t)(clock() / CLOCKS_PER_MSEC            ); }
uint64_t mp_hal_ticks_us_64(void) { return (uint64_t)(clock() / CLOCKS_PER_MSEC * 1000.0   ); }
uint64_t mp_hal_time_ns(void)     { return (uint64_t)(clock() / CLOCKS_PER_MSEC * 1000000.0); }


void mp_hal_delay_ms(uint32_t ms) {
    uint64_t start = clock();
    while ((uint64_t)clock() - start < ms * CLOCKS_PER_MSEC) {
        OS_Sleep(1);
        MICROPY_EVENT_POLL_HOOK
    }
}

void mp_hal_delay_us(uint32_t us) {
    /*uint64_t start = clock();
    while (((uint64_t)clock() - start) * 1000 < us * CLOCKS_PER_MSEC) {
        MICROPY_EVENT_POLL_HOOK
        OS_SleepUs(1);
    }*/
    OS_SleepUs(us);
}

void mp_hal_delay_us_fast(uint32_t us) {
    OS_SleepUs(us);
}
