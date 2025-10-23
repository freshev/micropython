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

#include <stdio.h>
#include "stdint.h"
#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "luat_mobile.h"
#include "luat_fs.h"
#include "luat_pm.h"
#include "luat_debug.h"
#include "luat_adc.h"
#include "luat_fota.h"
#include "luat_network_adapter.h"
#include "httpclient.h"

#include "mpconfigport.h"

// This file is never compiled standalone, it's included directly from
// extmod/modmachine.c via MICROPY_PY_MACHINE_INCLUDEFILE.

// refactoring need
//{ MP_OBJ_NEW_QSTR(MP_QSTR_RTC), (mp_obj_t)&modmachine_rtc_obj },

extern luat_rtos_task_handle microPyTaskHandle;

typedef enum {
    MP_PWRON_RESET = 1,
    MP_HARD_RESET,
    MP_WDT_RESET,
    MP_DEEPSLEEP_RESET,
    MP_SOFT_RESET
} reset_reason_t;

static mp_obj_t machine_wdt_test(void) {
    while(1);
    return mp_const_none;
}                                
static MP_DEFINE_CONST_FUN_OBJ_0(machine_wdt_test_obj, machine_wdt_test);

#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \
    /* Reset reasons */                                                         \
    { MP_ROM_QSTR(MP_QSTR_HARD_RESET), MP_ROM_INT(MP_HARD_RESET) },             \
    { MP_ROM_QSTR(MP_QSTR_PWRON_RESET), MP_ROM_INT(MP_PWRON_RESET) },           \
    { MP_ROM_QSTR(MP_QSTR_WDT_RESET), MP_ROM_INT(MP_WDT_RESET) },               \
    { MP_ROM_QSTR(MP_QSTR_DEEPSLEEP_RESET), MP_ROM_INT(MP_DEEPSLEEP_RESET) },   \
    { MP_ROM_QSTR(MP_QSTR_SOFT_RESET), MP_ROM_INT(MP_SOFT_RESET) },             \
                                                                                \
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) },                \
                                                                                \
    { MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&machine_rtc_type) },                \
                                                                                \
    { MP_OBJ_NEW_QSTR(MP_QSTR_OTA), (mp_obj_t)&modmachine_ota_obj },            \
                                                                                \
    { MP_ROM_QSTR(MP_QSTR_wdt_test), MP_ROM_PTR(&machine_wdt_test_obj) }

void modmachine_init0(void) {
    modmachine_wdt_init0();
    modmachine_rtc_init0();
    modmachine_pin_init0();
    modmachine_uart_init0();
    modmachine_pwm_init0();
    modmachine_adc_init0();

    luat_fs_info_t fs_info = {0};
    luat_fs_info("/", &fs_info);
    LUAT_DEBUG_PRINT("fs_info filesystem:%s, type:%d, total_block:%d, block_used:%d, block_size:%d", fs_info.filesystem, fs_info.type, fs_info.total_block, fs_info.block_used, fs_info.block_size);

    int val1, val2;
    luat_adc_open(LUAT_ADC_CH_CPU, NULL); // -1
    luat_adc_open(LUAT_ADC_CH_VBAT, NULL); // -2
    luat_adc_read(LUAT_ADC_CH_CPU, &val1, &val2);
    LUAT_DEBUG_PRINT("CPU temperature: %dC (ADC=%d)",val2, val1);
    luat_adc_read(LUAT_ADC_CH_VBAT, &val1, &val2);
    LUAT_DEBUG_PRINT("Battery voltage: %.3fV (ADC=%d)", val2 / 1000.0, val1);
    luat_adc_close(LUAT_ADC_CH_CPU); 
    luat_adc_close(LUAT_ADC_CH_VBAT);
}
void modmachine_deinit0(void) {
    modmachine_adc_deinit0();
    modmachine_pwm_deinit0();
    modmachine_uart_deinit0();
    modmachine_pin_deinit0();
    modmachine_rtc_deinit0();
}


extern int soc_get_model_name(char *model, uint8_t is_full);

static mp_obj_t mp_machine_unique_id(void) {
    char model[40] = {0};
    char imei[22] = {0};    
    char final[64] = {0};
    soc_get_model_name(model, 1);
    luat_mobile_get_imei(0, imei, 22);
    strcat(final, model);
    strcat(final, "_");
    strcat(final, imei);
    return mp_obj_new_str(final, strlen(final));
}

NORETURN static void mp_machine_reset(void) {
    luat_pm_reboot();
    while(1) luat_rtos_task_sleep(1000);
}

static mp_int_t mp_machine_reset_cause(void) {
    switch (luat_pm_get_poweron_reason()) {
        case LUAT_PM_POWERON_REASON_SWRESET:
            return MP_SOFT_RESET;
        case LUAT_PM_POWERON_REASON_HWRESET:
            return MP_HARD_RESET;
        case LUAT_PM_POWERON_REASON_WDT:
            return MP_WDT_RESET;
        default:
            return MP_PWRON_RESET;
    }
}


static void mp_machine_set_freq(size_t n_args, const mp_obj_t *args) {
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Chip set freq operation not supported"));
}

static mp_obj_t mp_machine_get_freq(void) {
    return MP_OBJ_NEW_SMALL_INT(luat_mcu_get_clk());
}


static void mp_machine_idle(void) {
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_IDLE, NULL);
    mp_event_handle_nowait(); // handle any events after possibly a long wait (eg feed WDT)
}

static void mp_machine_lightsleep(size_t n_args, const mp_obj_t *args) {
    mp_int_t i = mp_obj_get_int(args[0]);
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, NULL);
}

NORETURN static void mp_machine_deepsleep(size_t n_args, const mp_obj_t *args) {
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_DEEP, NULL);
    while(1) luat_rtos_task_sleep(1000);
}

// ------
//  FOTA
// ------

#ifdef FOTA_REMOVE_PY
int modmachine_endswith(const char *str, const char *suffix) {
    if (!str || !suffix) return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void modmachine_remove_files(char *suffix) {
    luat_fs_dirent_t *fs_entry = (luat_fs_dirent_t*)luat_heap_malloc(sizeof(luat_fs_dirent_t));
    memset(fs_entry, 0, sizeof(luat_fs_dirent_t));
    int res = 1;
    int start = 2; // skip "." and ".." folders

    while(res > 0) {
        res = luat_fs_lsdir("", fs_entry, start, 1); // skip fs_index files, read 1 fs_entry                
        // LUAT_DEBUG_PRINT("res = %d, d_type = %d, d_name = %s", res, fs_entry->d_type, fs_entry->d_name);
        if(modmachine_endswith(fs_entry->d_name, suffix)) {
            mp_printf(&mp_plat_print, "Remove %s ... ", fs_entry->d_name);
            int res = luat_fs_remove(fs_entry->d_name);
            if(res == 0) mp_printf(&mp_plat_print, "success\n");
            else mp_printf(&mp_plat_print, "failed\n");
        } else start++;
    }
    luat_heap_free(fs_entry);
}
#endif


#ifdef FOTA_USE
#define HTTP_RECV_BUF_SIZE      (1501)
static luat_fota_img_proc_ctx_ptr test_luat_fota_handle = NULL;

int http_client_fota_recv_cb(char* buf, uint32_t len) {
   int result = 1;
   if(test_luat_fota_handle) {
        result = (len > 0) ? luat_fota_write(test_luat_fota_handle, buf, len) : 0;
        if (result == 0) {
            // LUAT_DEBUG_PRINT("FOTA update success");
        } else {
            LUAT_DEBUG_PRINT("FOTA update error");
        } 
   }
   return !result;
}

static int luatos_fota_http_task(char *url) {

    // net_lwip_init(); // should be run only once, see modsocket.c
    net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
    network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
    luat_socket_check_ready(NW_ADAPTER_INDEX_LWIP_GPRS, NULL); 
   
    test_luat_fota_handle = luat_fota_init();
    if(!test_luat_fota_handle) {
        LUAT_DEBUG_PRINT("FOTA init failed");
        return 0;
    }

    uint8_t retryTimes = 0;
    char *recvBuf = m_new(char, HTTP_RECV_BUF_SIZE);
    if(recvBuf == NULL) {
        LUAT_DEBUG_PRINT("FOTA can not alloc receive buffer");
        return 0;
    }

    HTTPResult result = HTTP_INTERNAL;
    HttpClientContext fota_http_client = {0};
    int stepLen = 0;
    int totalLen = 0;

    httpInit(&fota_http_client, http_client_fota_recv_cb);
    while(retryTimes < 5) {
        result = httpConnect(&fota_http_client, url);
        if (result == HTTP_OK) {
            result = httpGetData(&fota_http_client, url, recvBuf, HTTP_RECV_BUF_SIZE, &stepLen, &totalLen);
            switch(result) {
                case HTTP_OK: break;
                case HTTP_INFLATE: LUAT_DEBUG_PRINT("FOTA inflate error"); break;
                case HTTP_CALLBACK: LUAT_DEBUG_PRINT("FOTA callback error"); break;
                default: LUAT_DEBUG_PRINT("FOTA internal error: %d", result); break;
            }
        } else {
            LUAT_DEBUG_PRINT("FOTA client connect error");
        }
        httpClose(&fota_http_client);
        if (stepLen == totalLen) break;
        retryTimes++;
        luat_rtos_task_sleep(3000);
    }
    m_del(char, recvBuf, HTTP_RECV_BUF_SIZE);

    if(stepLen == totalLen) {
        if(fota_http_client.httpResponseCode == 404) {
            LUAT_DEBUG_PRINT("FOTA image not found (%d)", fota_http_client.httpResponseCode);
            return 0;
        }
        
        int verify = luat_fota_done(test_luat_fota_handle);
        if(verify != 0) {
            LUAT_DEBUG_PRINT("FOTA image verify error");
            return 0;
        }
        LUAT_DEBUG_PRINT("FOTA image verify ok");       
    } else {
        LUAT_DEBUG_PRINT("FOTA data not fully received");
        return 0;
    }
    return 1;
}
#endif

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
    if (new_version != NULL && mp_obj_is_str(new_version)) {
        const char* newv = mp_obj_str_get_str(new_version);
        if(strcmp(newv, FW_VERSION) != 0) {
            char url[150];
            memset(url, 0, sizeof(url));
            sprintf(url, FOTA_URL, FW_VERSION, newv);
            if(query != NULL && mp_obj_is_str(query)) strcat(url, mp_obj_str_get_str(query));
            LUAT_DEBUG_PRINT("FOTA URL %s", url);
            mp_printf(&mp_plat_print, "FOTA URL %s\n", url);
            int res = luatos_fota_http_task(url);
            if(res) {
#ifdef FOTA_REMOVE_PY
                modmachine_remove_files(".py");
#endif
                return mp_obj_new_int(1);
            }
            else mp_printf(&mp_plat_print, "FOTA failed. Check internet connection and URL.\n");
        } else mp_printf(&mp_plat_print, "FOTA versions equals. Skip updating.\n");
    } else mp_raise_ValueError(MP_ERROR_TEXT("FOTA requested version should be string."));
    return mp_obj_new_int(0);
#else
    mp_printf(&mp_plat_print, "FOTA disabled.\n");
    return mp_obj_new_int(0);
#endif
}
static MP_DEFINE_CONST_FUN_OBJ_KW(modmachine_ota_obj, 1, modmachine_ota);
