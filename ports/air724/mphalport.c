/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
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

#include "py/runtime.h"
#include "py/stream.h"
#include "py/ringbuf.h"
#include "extmod/misc.h"
#include "mphalport.h"


uint8_t stdin_ringbuf_array[1024 * 2];
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


//luat_rtos_task_handle replTimeoutTaskHandle;
//void mp_repl_timeout_task(void *param) {
//    while(1);
//}
//int replTimeoutTaskFlag = 0;

int mp_hal_stdin_rx_chr(void) {

    //if(replTimeoutTaskFlag) {
    //    // LUAT_DEBUG_PRINT("Start repl timeout task");
    //    luat_rtos_task_create(&replTimeoutTaskHandle, 1024, 10, "repl_timeout_task", mp_repl_timeout_task, NULL, NULL);
    //}
    for (;;) {
        int c = ringbuf_get(&stdin_ringbuf);
        if (c != -1) {
            //if(replTimeoutTaskFlag)  {
            //    if(replTimeoutTaskHandle) {
            //        // LUAT_DEBUG_PRINT("Stop repl timeout task");
            //        luat_rtos_task_delete(replTimeoutTaskHandle);
            //    }
            //}
            return c;
        }
        iot_os_sleep(1); // this is for HW_REPL UART callback normal firing. Do not comment! 
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


uint32_t mp_hal_ticks_ms(void)    { return (uint32_t)(iot_os_get_system_tick()); }
uint32_t mp_hal_ticks_us(void)    { return (uint32_t)(iot_os_get_system_tick()) * 1000; } 
uint64_t mp_hal_ticks_ms_64(void) { return (uint64_t)(iot_os_get_system_tick()); }
uint64_t mp_hal_ticks_us_64(void) { return (uint64_t)(iot_os_get_system_tick()) * 1000; }
//uint64_t mp_hal_time_ns(void)     { return (uint64_t)(luat_mcu_ticks() * 1000.0 / luat_mcu_us_period()); }

void mp_hal_delay_ms(uint32_t ms) {
    uint64_t start = iot_os_get_system_tick();
    while ((uint64_t)iot_os_get_system_tick() - start < ms) {
        osiThreadSleep(1);
        MICROPY_EVENT_POLL_HOOK
    }
}

void mp_hal_delay_us(uint32_t us) {
    osiThreadSleepUS(us);
}

void mp_hal_delay_us_fast(uint32_t us) {
    osiThreadSleepUS(us);
}

int mp_hal_pin_read(mp_hal_pin_obj_t pin) {
    return iot_gpio_read(pin);
}

// Wake up the main task if it is sleeping, to be called from an ISR.
void mp_hal_wake_main_task_from_isr(void) {
}

//#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)
/*int luat_crypto_trng(char* buff, size_t len);
int mbedtls_hardware_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen ){

    if (data != NULL)
        data = NULL;
    *olen = 0;
    int rnd = luat_crypto_trng((char *)output, len);
    if (rnd != 0)
    {
        return -1;
    }
    *olen = len;   
    return( 0 );
}
*/
//#endif
