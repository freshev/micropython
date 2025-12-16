/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
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
// extmod/machine_wdt.c via MICROPY_PY_MACHINE_WDT_INCLUDEFILE.

#include "luat_wdt.h"

// https://doc.openluat.com/wiki/37?wiki_page_id=4586

typedef struct _machine_wdt_obj_t {
    mp_obj_base_t base;
    uint8_t id;
} machine_wdt_obj_t;

static machine_wdt_obj_t machine_wdt_default = {{&machine_wdt_type}};

void modmachine_wdt_init0(void) {
    machine_wdt_obj_t *wdt;
    wdt = MP_OBJ_TO_PTR(&machine_wdt_default);
    wdt->id = 0;
}

extern int replTimeoutTaskFlag; // see mphalport.c
static machine_wdt_obj_t *mp_machine_wdt_make_new_instance(mp_int_t id, mp_int_t timeout_ms) {
    // timeout for software watchdog should be in interval 0 - 60000 ms
    // value = 0 turns off watchdog (SW and HW)
    // value < 0 simply returns watchdog instance
    switch (id) {
        case 0: // software watchdog
            if(timeout_ms > 0) {
                if(timeout_ms > 60000) timeout_ms = 60000;
                luat_wdt_close();
                luat_wdt_setup(timeout_ms / 1000);
                replTimeoutTaskFlag = 1;
            }
            else if(timeout_ms == 0) {                
                luat_wdt_close();
                replTimeoutTaskFlag = 0;
            }
            return &machine_wdt_default;
        default: mp_raise_ValueError(MP_ERROR_TEXT("Watchdog id should be 0"));
    }
}

static void mp_machine_wdt_feed(machine_wdt_obj_t *self) {
    luat_wdt_feed();
}


static void mp_machine_wdt_timeout_ms_set(machine_wdt_obj_t *self_in, mp_int_t timeout_ms) {
    switch (self_in->id) {
        case 0: // software watchdog
            if(timeout_ms > 0) {
                if(timeout_ms > 60000) timeout_ms = 60000;
                luat_wdt_close();
                luat_wdt_setup(timeout_ms / 1000);
            }
            else if(timeout_ms == 0) luat_wdt_close();
            else mp_raise_ValueError(MP_ERROR_TEXT("Watchdog timeout should be >= 0"));
            break;
        default: mp_raise_ValueError(MP_ERROR_TEXT("Watchdog id should be 0"));
            break;
    }
}

/*
# Test case
import machine
wdt = machine.WDT(0, 10000) # timeout in milliseconds
machine.wdt_test() # freeze micropython thread
*/