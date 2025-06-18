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
#include "miniz.h"

#include "mpconfigport.h"
#include "modmachine.h"

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

STATIC mp_obj_t machine_wdt_test(void) {
    while(1);
    return mp_const_none;
}                                
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_wdt_test_obj, machine_wdt_test);

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
    { MP_OBJ_NEW_QSTR(MP_QSTR_OTA), (mp_obj_t)&modmachine_ota_obj },            \
                                                                                \
    { MP_ROM_QSTR(MP_QSTR_wdt_test), MP_ROM_PTR(&machine_wdt_test_obj) }

void modmachine_init0(void) {
    modmachine_wdt_init0();
    //modmachine_rtc_init0();
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
}


extern int soc_get_model_name(char *model, uint8_t is_full);

STATIC mp_obj_t mp_machine_unique_id(void) {
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

NORETURN STATIC void mp_machine_reset(void) {
    luat_pm_reboot();
    while(1) luat_rtos_task_sleep(1000);
}

STATIC mp_int_t mp_machine_reset_cause(void) {
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


STATIC void mp_machine_set_freq(size_t n_args, const mp_obj_t *args) {
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Chip set freq operation not supported"));
}

STATIC mp_obj_t mp_machine_get_freq(void) {
    return MP_OBJ_NEW_SMALL_INT(luat_mcu_get_clk());
}


STATIC void mp_machine_idle(void) {
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_IDLE, NULL);
    mp_event_handle_nowait(); // handle any events after possibly a long wait (eg feed WDT)
}

STATIC void mp_machine_lightsleep(size_t n_args, const mp_obj_t *args) {
    mp_int_t i = mp_obj_get_int(args[0]);
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, NULL);
}

NORETURN STATIC void mp_machine_deepsleep(size_t n_args, const mp_obj_t *args) {
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_DEEP, NULL);
    while(1) luat_rtos_task_sleep(1000);
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
/*    Dir_t* dir = API_FS_OpenDir("/");
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
*/
}

#define HTTP_RECV_BUF_SIZE      (1501)
static luat_fota_img_proc_ctx_ptr test_luat_fota_handle;

int http_client_fota_recv_cb(char* buf, uint32_t len) {
   int result = 0; // !!!!!!!
   if(test_luat_fota_handle) {
        result = luat_fota_write(test_luat_fota_handle, buf, len);
        if (result == 0) {
            LUAT_DEBUG_PRINT("fota update success");
        } else {
            LUAT_DEBUG_PRINT("fota update error");
        } 
   }
   return result;
}

STATIC int luatos_fota_http_task(char *url) {

    net_lwip_init();
    net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
    network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
    luat_socket_check_ready(NW_ADAPTER_INDEX_LWIP_GPRS, NULL); 
   
    luat_fota_img_proc_ctx_ptr test_luat_fota_handle = NULL;
    // test_luat_fota_handle = luat_fota_init();

    if(!test_luat_fota_handle) {
        LUAT_DEBUG_PRINT("FOTA init failed");
        // return 0; // !!!
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
            httpClose(&fota_http_client);            
            if (stepLen == totalLen) break;
        } else {
            LUAT_DEBUG_PRINT("http client connect error");
        }
        retryTimes++;
        luat_rtos_task_sleep(3000);
    }
    m_del(char, recvBuf, HTTP_RECV_BUF_SIZE);

    if(stepLen == totalLen) {
        if(fota_http_client.httpResponseCode == 404) {
            LUAT_DEBUG_PRINT("FOTA image not found (%d)", fota_http_client.httpResponseCode);
            return 0;
        }
        LUAT_DEBUG_PRINT("image_verify ok");
        if(test_luat_fota_handle != NULL) {
            int verify = luat_fota_done(test_luat_fota_handle);
            if(verify != 0) {
                LUAT_DEBUG_PRINT("image_verify error");
                return 0;
            }
            LUAT_DEBUG_PRINT("image_verify ok");       
        }
    } else {
        LUAT_DEBUG_PRINT("http client data not fully received");
        return 0;
    }
    return 1;
}


STATIC mp_obj_t modmachine_ota(mp_obj_t new_version) {
    // ========================================
    // Firmware over the air (FOTA)
    // ========================================
    if (mp_obj_is_str(new_version)) {
        const char* newv = mp_obj_str_get_str(new_version);
        if(strcmp(newv, FW_VERSION) != 0) {
            char url[512];
            memset(url, 0, sizeof(url));
            sprintf(url, FOTA_URL, FW_VERSION, newv);
            LUAT_DEBUG_PRINT("FOTA URL %s", url);
            mp_printf(&mp_plat_print, "FOTA URL %s\n", url);
            int res = luatos_fota_http_task(url);
            if(res) {
                // modmachine_remove_files(".py");
                return mp_obj_new_int(1);
            }
            else mp_printf(&mp_plat_print, "FOTA failed. Check internet connection and URL.\n");
        } else mp_printf(&mp_plat_print, "FOTA versions equals. Skip updating.\n");
    } else mp_raise_ValueError("FOTA requested version should be string.");
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modmachine_ota_obj, modmachine_ota);
