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

#include <stdio.h>

#include "modmachine.h"
#include "api_os.h"
#include "api_fota.h"
#include "api_debug.h"
#include "api_event.h"
#include "api_hal_pm.h"
#include "api_hal_adc.h"
#include "api_fs.h"

#include "mpconfigport.h"
#include "stdint.h"
#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"

uint8_t machine_hw_watchdog_active = 1;
uint8_t machine_hw_watchdog_pin_level = GPIO_LEVEL_LOW;

// This file is never compiled standalone, it's included directly from
// extmod/modmachine.c via MICROPY_PY_MACHINE_INCLUDEFILE.

// refactoring need
//{ MP_OBJ_NEW_QSTR(MP_QSTR_RTC), (mp_obj_t)&modmachine_rtc_obj },

#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) },                            \
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_power_key), (mp_obj_t)&modmachine_on_power_key_obj },      \
    { MP_OBJ_NEW_QSTR(MP_QSTR_OTA), (mp_obj_t)&modmachine_ota_obj },                        \
    { MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&machine_rtc_type) },                            \
                                                                                            \
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_KEY),       MP_ROM_INT(POWER_ON_CAUSE_KEY) },      \
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_CHARGE),    MP_ROM_INT(POWER_ON_CAUSE_CHARGE) },   \
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_ALARM),     MP_ROM_INT(POWER_ON_CAUSE_ALARM) },    \
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_EXCEPTION), MP_ROM_INT(POWER_ON_CAUSE_EXCEPTION) },\
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_RESET),     MP_ROM_INT(POWER_ON_CAUSE_RESET) },    \
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_MAX),       MP_ROM_INT(POWER_ON_CAUSE_MAX) },      \
                                                                                            \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_32K),          MP_ROM_INT(PM_SYS_FREQ_32K) },         \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_13M),          MP_ROM_INT(PM_SYS_FREQ_13M) },         \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_26M),          MP_ROM_INT(PM_SYS_FREQ_26M) },         \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_39M),          MP_ROM_INT(PM_SYS_FREQ_39M) },         \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_52M),          MP_ROM_INT(PM_SYS_FREQ_52M) },         \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_78M),          MP_ROM_INT(PM_SYS_FREQ_78M) },         \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_89M),          MP_ROM_INT(PM_SYS_FREQ_89M) },         \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_104M),         MP_ROM_INT(PM_SYS_FREQ_104M) },        \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_113M),         MP_ROM_INT(PM_SYS_FREQ_113M) },        \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_125M),         MP_ROM_INT(PM_SYS_FREQ_125M) },        \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_139M),         MP_ROM_INT(PM_SYS_FREQ_139M) },        \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_156M),         MP_ROM_INT(PM_SYS_FREQ_156M) },        \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_178M),         MP_ROM_INT(PM_SYS_FREQ_178M) },        \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_208M),         MP_ROM_INT(PM_SYS_FREQ_208M) },        \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_250M),         MP_ROM_INT(PM_SYS_FREQ_250M) },        \
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_312M),         MP_ROM_INT(PM_SYS_FREQ_312M) },        \
                                                                                            \
    { MP_ROM_QSTR(MP_QSTR_ADC_CHANNEL_0),            MP_ROM_INT(ADC_CHANNEL_0) },           \
    { MP_ROM_QSTR(MP_QSTR_ADC_CHANNEL_1),            MP_ROM_INT(ADC_CHANNEL_1) },           \
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_122US),  MP_ROM_INT(ADC_SAMPLE_PERIOD_122US) }, \
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_1MS),    MP_ROM_INT(ADC_SAMPLE_PERIOD_1MS) },   \
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_10MS),   MP_ROM_INT(ADC_SAMPLE_PERIOD_10MS) },  \
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_100MS),  MP_ROM_INT(ADC_SAMPLE_PERIOD_100MS) }, \
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_250MS),  MP_ROM_INT(ADC_SAMPLE_PERIOD_250MS) }, \
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_500MS),  MP_ROM_INT(ADC_SAMPLE_PERIOD_500MS) }, \
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_1S),     MP_ROM_INT(ADC_SAMPLE_PERIOD_1S) },    \
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_2S),     MP_ROM_INT(ADC_SAMPLE_PERIOD_2S) },    \

static void mp_machine_idle(void) {
    //OS_Sleep(1);
    mp_event_handle_nowait(); // handle any events after possibly a long wait (eg feed WDT)
}

static mp_obj_t power_key_callback = mp_const_none;

void modmachine_init0(void) {
    PM_SetSysMinFreq(PM_SYS_FREQ_312M);
    modmachine_wdt_init0();
    modmachine_rtc_init0();
    modmachine_pin_init0();
    modmachine_uart_init0();
    power_key_callback = mp_const_none;
}

// ------
// Notify
// ------

Power_On_Cause_t powerOnCause = POWER_ON_CAUSE_MAX;

void modmachine_notify_power_on(API_Event_t* event) {
    powerOnCause = event->param1;
}

void modmachine_notify_power_key_down(API_Event_t* event) {
    if (power_key_callback && power_key_callback != mp_const_none) {
        mp_sched_schedule(power_key_callback, mp_obj_new_bool(1));
    }
}

void modmachine_notify_power_key_up(API_Event_t* event) {
    if (power_key_callback && power_key_callback != mp_const_none) {
        mp_sched_schedule(power_key_callback, mp_obj_new_bool(0));
    }
}

// -------
// Methods
// -------

static mp_obj_t mp_machine_unique_id(void) {
    // ========================================
    // Returns FW version
    // ========================================
    return mp_obj_new_str(FW_VERSION, strlen(FW_VERSION));
}

NORETURN static void mp_machine_reset(void) {
    // ========================================
    // Resets the module.
    // Prevents cycle reboot due to main.py early machine.reset()
    // ========================================
    if(mp_hal_ticks_ms_64() < 10 * 1000) {
        mp_printf(&mp_plat_print, "Remove faulty main.py\n");
        API_FS_Delete("main.py");
        OS_Sleep(100);
    }
    PM_Restart();
    OS_Sleep(10000);
    while(1) {}
}

static mp_int_t mp_machine_reset_cause(void) {
    // ========================================
    // Retrieves the last reason for the power on.
    // Returns:
    //     An integer with the reason encoded.
    // ========================================
    switch (powerOnCause) {
        case POWER_ON_CAUSE_KEY:
            return POWER_ON_CAUSE_KEY;
            break;
        case POWER_ON_CAUSE_CHARGE:
            return POWER_ON_CAUSE_CHARGE;
            break;
        case POWER_ON_CAUSE_ALARM:
            return POWER_ON_CAUSE_ALARM;
            break;
        case POWER_ON_CAUSE_EXCEPTION:
            return POWER_ON_CAUSE_EXCEPTION;
            break;
        case POWER_ON_CAUSE_RESET:
            return POWER_ON_CAUSE_RESET;
            break;
        case POWER_ON_CAUSE_MAX:
            return POWER_ON_CAUSE_MAX;
            break;
        default:
            return powerOnCause;
            break;
    }
}


static void mp_machine_set_freq(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Sets the minimal CPU frequency.
    // Args:
    //     freq (int): a constant specifying
    //     the frequency.
    // ========================================
    mp_int_t i = mp_obj_get_int(args[0]);
    if (i != PM_SYS_FREQ_32K && i != PM_SYS_FREQ_13M && i != PM_SYS_FREQ_26M && i != PM_SYS_FREQ_39M &&
            i != PM_SYS_FREQ_52M && i != PM_SYS_FREQ_78M && i != PM_SYS_FREQ_89M && i != PM_SYS_FREQ_104M &&
            i != PM_SYS_FREQ_113M && i != PM_SYS_FREQ_125M && i != PM_SYS_FREQ_139M && i != PM_SYS_FREQ_156M &&
            i != PM_SYS_FREQ_178M && i != PM_SYS_FREQ_208M && i != PM_SYS_FREQ_250M && i != PM_SYS_FREQ_312M)
        mp_raise_ValueError(MP_ERROR_TEXT("Unknown frequency"));
    PM_SetSysMinFreq(i);
}

static mp_obj_t mp_machine_get_freq(void) {
    return 0; // TODO
}

static mp_obj_t modmachine_on_power_key(mp_obj_t callable) {
    // ========================================
    // Sets a callback on power key press.
    // Args:
    //     callback (Callable): a callback to
    //     execute on power key press.
    // ========================================
    power_key_callback = callable;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(modmachine_on_power_key_obj, modmachine_on_power_key);

static void mp_machine_lightsleep(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Puts the module into low-power mode by
    // tuning down frequencies and powering
    // down certain peripherials.
    // The execution continues.
    // Args:
    //     flag (bool): if True, triggers low-
    //     power mode. If False, switches low-
    //     power mode off.
    // ========================================
    mp_int_t i = mp_obj_get_int(args[0]);
    PM_SleepMode(i);
}

NORETURN static void mp_machine_deepsleep(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Puts the module into low-power mode by
    // tuning down frequencies and powering
    // down certain peripherials.
    // The execution continues.
    // Args:
    //     flag (bool): if True, triggers low-
    //     power mode. If False, switches low-
    //     power mode off.
    // ========================================
    mp_machine_reset();
}

// ------
//  FOTA
// ------
int modmachine_endswith(const char *str, const char *suffix) {
    if (!str || !suffix) return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void modmachine_remove_files(char *suffix) {
    Dir_t* dir = API_FS_OpenDir("/");
    const Dirent_t* entry = NULL;
    while ((entry = API_FS_ReadDir(dir))) {
        if(modmachine_endswith(entry->d_name, suffix)) {
            mp_printf(&mp_plat_print, "Remove %s ... ", entry->d_name);
            int res = API_FS_Delete(entry->d_name);
            if(res == 0) mp_printf(&mp_plat_print, "success\n");
            else mp_printf(&mp_plat_print, "failed\n");
        }
    }
    API_FS_CloseDir(dir);
}

static uint8_t _fota_result = 0;
static void processFota(const unsigned char *data, int len) {
    Trace(1,"FOTA total length:%d, data:%s", len, data);
    if(len > 0 && data != NULL) {
        // MEMBLOCK_Trace(1, (uint8_t*)data, (uint16_t)len, 16);
        if(API_FotaInit(len)) {
            Trace(1, "FOTA inited");
            //mp_printf(&mp_plat_print, "FOTA inited\n");            
            int res_len = API_FotaReceiveData((unsigned char*)data, (int)len);
            if(res_len != 0) {
                Trace(1, "FOTA received data %d", res_len);
                //mp_printf(&mp_plat_print, "FOTA received data %d\n" , res_len);
                _fota_result = 1;
#ifdef FOTA_REMOVE_PY
                modmachine_remove_files(".py");
#endif
            } else {
                Trace(1, "FOTA NOT received data");
                //mp_printf(&mp_plat_print, "FOTA NOT received data\n");
            }
            API_FotaClean();
            return;
        }
    } // else mp_printf(&mp_plat_print, "FOTA len = 0\n");
    Trace(1,"FOTA failed");
    // mp_printf(&mp_plat_print, "FOTA failed\n");    
    API_FotaClean();
}


extern void* heap;

static mp_obj_t modmachine_ota(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // ========================================
    // Firmware over the air (FOTA)
    // ========================================
    enum { ARG_newversion, ARG_query };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_newversion, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        { MP_QSTR_query, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    mp_obj_t new_version = args[ARG_newversion].u_obj;
    mp_obj_t query = args[ARG_query].u_obj;    

#ifdef FOTA_USE
    _fota_result = 0;
    if (new_version != NULL && mp_obj_is_str(new_version)) {
        const char* newv = mp_obj_str_get_str(new_version);
        if(strcmp(newv, FW_VERSION) != 0) {
            char url[150];
            memset(url, 0, sizeof(url));
            sprintf(url, FOTA_URL, FW_VERSION, newv);
            if(query != NULL && mp_obj_is_str(query)) strcat(url, mp_obj_str_get_str(query));
            Trace(1,"FOTA URL %s", url);
            if(API_FotaByServer(url, processFota) == 0) {
            	return mp_obj_new_int(_fota_result);
            } else mp_printf(&mp_plat_print, "FOTA failed. Check internet connection.\n");
        } else mp_printf(&mp_plat_print, "FOTA versions equals. Skip updating.\n");
    } else mp_raise_ValueError(MP_ERROR_TEXT("FOTA requested version should be string."));
    return mp_obj_new_int(0);
#else
    mp_printf(&mp_plat_print, "FOTA disabled.\n");
    return mp_obj_new_int(0);
#endif
}
static MP_DEFINE_CONST_FUN_OBJ_KW(modmachine_ota_obj, 1, modmachine_ota);

