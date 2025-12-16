/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Paul Sokolovsky
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

// This file is never compiled standalone, it's included directly from
// extmod/machine_wdt.c via MICROPY_PY_MACHINE_WDT_INCLUDEFILE.

#include "modmachine.h"
#include "api_os.h"
#include "api_hal_watchdog.h"
#include "api_hal_gpio.h"

typedef struct _machine_wdt_obj_t {
    mp_obj_base_t base;
    uint8_t id;
} machine_wdt_obj_t;

static machine_wdt_obj_t machine_wdt_default = {{&machine_wdt_type}};
static machine_wdt_obj_t machine_hw_wdt_default = {{&machine_wdt_type}};

static machine_wdt_obj_t *mp_machine_wdt_make_new_instance(mp_int_t id, mp_int_t timeout_ms) {
    // timeout for software watchdog should be in interval 0 - 511999 ms
    // value > 512000 overflows WatchDog_Open() and hangs A9
    // value = 0 turns off watchdog (SW and HW)
    // value < 0 simply returns watchdog instance
    // exact timeout for hw watchdog not used i.e. it relies on HW only
    switch (id) {
        case 0: // A9 soft watchdog
            if(timeout_ms > 0 && timeout_ms < 512000) {
                 WatchDog_Close();
                 WatchDog_Open(WATCHDOG_SECOND_TO_TICK(timeout_ms / 1000));
            }
            else if(timeout_ms == 0) WatchDog_Close();
            else if(timeout_ms >= 512000) mp_raise_ValueError(MP_ERROR_TEXT("Watchdog timeout should be in interval 0-512000"));
            return &machine_wdt_default;
        case 1: // A9 hardware watchdog
            machine_hw_watchdog_active = (timeout_ms > 0);
            return &machine_hw_wdt_default;
        default: mp_raise_ValueError(MP_ERROR_TEXT("Watchdog id should be 0 (software wd) or 1 (hardware wd)"));
    }
}

static void mp_machine_wdt_feed(machine_wdt_obj_t *self) {
    WatchDog_KeepAlive();
}


static void mp_machine_wdt_timeout_ms_set(machine_wdt_obj_t *self_in, mp_int_t timeout_ms) {
    switch (self_in->id) {
        case 0: // A9 soft watchdog
            if(timeout_ms > 0 && timeout_ms < 512000) {
                WatchDog_Close();
                WatchDog_Open(WATCHDOG_SECOND_TO_TICK(timeout_ms / 1000));
            }
            else if(timeout_ms == 0) WatchDog_Close();
            else mp_raise_ValueError(MP_ERROR_TEXT("Watchdog timeout should be in interval 0-512000"));
            break;
        case 1: // A9 hardware watchdog
            machine_hw_watchdog_active = (timeout_ms > 0);
            break;
        default: mp_raise_ValueError(MP_ERROR_TEXT("Watchdog id should be 0 (software wd) or 1 (hardware wd)"));
            break;
    }
}

void modmachine_wdt_init0(void) {
    machine_wdt_obj_t *wdt;
    wdt = MP_OBJ_TO_PTR(&machine_wdt_default);
    wdt->id = 0;
    wdt = MP_OBJ_TO_PTR(&machine_hw_wdt_default);
    wdt->id = 1;
    //mp_machine_wdt_timeout_ms_set(&machine_wdt_default, 0); // leads to cycle reboot
}


// Hardware watchdog functions
// Change WD pin state during boot (patched hal_HstSendEvent())
typedef bool TGPIO_Init(GPIO_config_t config);
void ChangeWDPinStateInBoot() {
    // This called from "Critical Section" code in hal_host.c -> hal_HstSendEvent(), do not use Trace(...), etc
    // do not uncomment Trace(...) here
    // Trace(0, "WD pin state change, time:%dms, pin %d: %d, GPIO_Init: %p, ChangeWDPinState: %p", (uint32_t)(clock()/CLOCKS_PER_MSEC), MACHINE_HW_WATCHDOG_PIN, machine_hw_watchdog_pin_level, GPIO_Init, ChangeWDPinState);
    // GPRS C SDK is patched by "patch-lod.py"

    machine_hw_watchdog_pin_level = machine_hw_watchdog_pin_level == GPIO_LEVEL_LOW ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
    GPIO_config_t config_wdi = {
        .pin = MACHINE_HW_WATCHDOG_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .defaultLevel = machine_hw_watchdog_pin_level,
    };
    TGPIO_Init* BootGPIO_Init = (TGPIO_Init*)0x82005C45; // for SDK SW_V2131
    (*BootGPIO_Init)(config_wdi);
}

// Change WD pin state perioducally (not restartable - WDT should reset the board)
void ChangeWDPinState(HANDLE* mainTaskHandle) {
    if(machine_hw_watchdog_active) {
        //Trace(0, "WD pin state change, time:%dms, pin %d: %d", (uint32_t)(clock()/CLOCKS_PER_MSEC), MACHINE_HW_WATCHDOG_PIN, machine_hw_watchdog_pin_level);
        machine_hw_watchdog_pin_level = machine_hw_watchdog_pin_level == GPIO_LEVEL_LOW ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
        GPIO_config_t config_wdi = {
            .pin = MACHINE_HW_WATCHDOG_PIN,
            .mode = GPIO_MODE_OUTPUT,
            .defaultLevel = machine_hw_watchdog_pin_level,
        };
        GPIO_Init(config_wdi);
        OS_StartCallbackTimer(*mainTaskHandle, MACHINE_HW_WATCHDOG_TIMEOUT, (void *)ChangeWDPinState, mainTaskHandle); // rearm timer
    }
}

