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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"
#include "shared/runtime/gchelper.h"
#include "shared/readline/readline.h"

#include "mpconfigport.h"
#include "iot_os.h"
#include "iot_debug.h"
#include "iot_uart.h"
#include "assert.h"

#define MICROPYTHON_TASK_PRIORITY   (20) 

// #define CONFIG_SRAM_SIZE 0x40000 - 260kB
//#define MICROPY_HEAP_MAX_SIZE       (1024 * 3800) // hangs under UART
#define MICROPY_HEAP_MAX_SIZE       (1024 * 2000)

#define MICROPY_HEAP_MIN_SIZE       (1024 * 10) 

#define MP_TASK_STACK_LIMIT_MARGIN  (2048)
#define MICROPY_TASK_STACK_SIZE     (16 * 1024)

#define MP_FATAL_REASON_NLR_JUMP_FAIL   (1)
#define MP_FATAL_REASON_HEAP_INIT       (2)


HANDLE microPyTaskHandle = NULL;

void NORETURN mp_fatal_error(uint8_t reason, void* ptr1);
void NORETURN nlr_jump_fail(void *val) {
    mp_fatal_error(MP_FATAL_REASON_NLR_JUMP_FAIL, val);
    while(1);
}
//extern void RPC_CFW_GetIMEIFromAP(void *in,void *out);
extern void CFW_EmodGetIMEI(uint8_t *pImei,uint8_t *pImeiLen, uint32_t nSimID);

void* mp_allocate_heap(uint32_t* size) {
    uint32_t h_size = MICROPY_HEAP_MAX_SIZE;
    void* ptr = NULL;
    int counter = 0;
    while (!ptr) {
        // iot_debug_print("Try %d heap size", h_size);
        if (h_size < MICROPY_HEAP_MIN_SIZE) mp_fatal_error(MP_FATAL_REASON_HEAP_INIT, NULL);
        ptr = iot_os_malloc(h_size);
        if (!ptr) {
             h_size = h_size - MICROPY_HEAP_MIN_SIZE;
             counter++;
        }
    }

    
    #ifdef FOTA_USE
    { 
    iot_os_free(ptr);
    // h_size -=  2 * MBEDTLS_SSL_MAX_CONTENT_LEN; // leave 32K buffer for MBEDTLS, HTTP deflate/inflate
    ptr = iot_os_malloc(h_size);
    if (!ptr) mp_fatal_error(MP_FATAL_REASON_HEAP_INIT, NULL);
    }
    #endif

    size[0] = h_size;
    if(counter == 0) iot_debug_print("MICROPY_HEAP_MAX_SIZE can be increased!");
    iot_debug_print("Final heap size: %d ", h_size);

    /*uint8_t IMEI[64] = {0};
    uint8_t len;
    iot_debug_print("Try to get imei");
    CFW_EmodGetIMEI(IMEI, &len, 0);
    iot_debug_print("IMEI: %s ", IMEI);
    */
   
    return ptr;
}

void mp_task(void *param) {

#ifdef HALTONEXC
    // If the system crashes, it will print information instead of restarting.
    iot_debug_set_fault_mode(OPENAT_FAULT_HANG); !!!
#endif
    gc_helper_regs_t regs;    
    uint32_t mp_task_heap_size;
    volatile uint32_t sp;
    sp = gc_helper_get_regs_and_sp(regs);
    
soft_reset:
    mp_stack_ctrl_init();
    mp_stack_set_top((void *)sp);
    mp_stack_set_limit(MICROPY_TASK_STACK_SIZE - MP_TASK_STACK_LIMIT_MARGIN);

    void *mp_task_heap = mp_allocate_heap(&mp_task_heap_size); // allocate maximum
    gc_init(mp_task_heap, mp_task_heap + mp_task_heap_size);
    
    mp_init();           // iot_debug_print("micropython inited");
    
    modmachine_init0();  // iot_debug_print("machine inited");
    readline_init0();    // iot_debug_print("readline inited");
    //modcellular_init0(); // iot_debug_print("cellular inited");
#ifdef GPS_MODULE
    //modgps_init0(); iot_debug_print("GPS inited");
#endif

    
    mp_obj_list_init(mp_sys_path, 0);  // iot_debug_print("mp_obj_list inited");
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_));
    mp_obj_list_init(mp_sys_argv, 0);
    // iot_debug_print("sys path inited");
    
    // Startup scripts
    pyexec_frozen_module("_boot.py", false); // iot_debug_print("_boot.py started");
    pyexec_file_if_exists("boot.py");        // iot_debug_print("boot.py started");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
#ifdef MAINRUN
        iot_debug_print("execute main.py");
        pyexec_file_if_exists("main.py"); 
#endif
    }
    // iot_debug_print("start REPL");
    while (1) {        
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) break;
        } else {
            if (pyexec_friendly_repl() != 0) break;
        }
    }
#if MICROPY_ENABLE_GC
    gc_sweep_all();
#endif

    modmachine_deinit0();
    mp_deinit();
    iot_os_free(mp_task_heap);
    mp_hal_stdout_tx_str("PYB: soft reboot\r\n");
    mp_hal_delay_ms(10);// allow UART to flush output

    goto soft_reset;
}


#if MICROPY_DEBUG_VERBOSE
char trace_debug_buffer[64];
size_t trace_debug_buffer_len = 0;

int DEBUG_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(MICROPY_DEBUG_PRINTER, fmt, ap);
    va_end(ap);
    return ret;
}

STATIC void debug_print_strn(void *env, const char *str, size_t len) {
    (void) env;
    if(microPyTaskHandle != NULL) {
        for (size_t i = 0; i < len; i++) {
            if (trace_debug_buffer_len == sizeof(trace_debug_buffer) - 1 || str[i] == '\n') {
                // flush
                iot_debug_print("%s", trace_debug_buffer);
                memset(trace_debug_buffer, 0, sizeof(trace_debug_buffer));
                trace_debug_buffer_len = 0;
            }
            if (str[i] != '\n') trace_debug_buffer[trace_debug_buffer_len++] = str[i];
        }
    }
}
const mp_print_t mp_debug_print = {NULL, debug_print_strn};
#endif


static char msg_nlr_jump_fail[] = "nlr jump fail";
static char msg_heap_init[] = "heap init fail";
static char msg_unknown[] = "unknown";
static char msg_reboot[] = "Rebooting...";

void NORETURN mp_fatal_error(uint8_t reason, void* ptr1) {
    // ========================================
    // A fatal error. Avoid using stack.
    // ========================================
    iot_debug_print("Fatal error reason: %d", reason);

    //iot_debug_print(msg1);
    switch (reason) {
        case MP_FATAL_REASON_NLR_JUMP_FAIL:
            iot_debug_print(msg_nlr_jump_fail);
            break;
        case MP_FATAL_REASON_HEAP_INIT:
            iot_debug_print(msg_heap_init);
            break;
        default:
            iot_debug_print(msg_unknown);
    }
    iot_debug_print("%p", ptr1);
    iot_debug_print(msg_reboot);
    iot_debug_assert(false, __FILE__, (short)__LINE__);
    iot_os_sleep(15000); 
    
    iot_os_restart();
    while(1);
}

int appimg_enter(void *param) {
    // wait to coolhost reconnects
    iot_os_sleep(1000); 

    // param3: port -  0(uart1), 1(uart2), 2(uart3 "AP Diag"), 3 (usb modem)
    //iot_os_set_trace_port(2);

    // param1: 0 - OSI Trace, 1 - Modem trace
    // param2: 0 - disable, 1 - enable
    // param3: port -  0("LUAT USB Device 0 Modem"), 1("LUAT USB Device 1 AT"?), 2(uart3 "LUAT USB Device 2 AP Diag"), 3("LUAT USB Device 1 AT"?)
       iot_vat_send_cmd((uint8_t*)"AT^TRACECTRL=0,1,2\r\n", sizeof("AT^TRACECTRL=0,1,2\r\n")); // enable OSI trace to AP Diag
    // iot_vat_send_cmd((uint8_t*)"AT^TRACECTRL=1,1,2\r\n", sizeof("AT^TRACECTRL=1,1,2\r\n")); // enable Modem trace to AP Diag
    // iot_vat_send_cmd((uint8_t*)"AT^TRACECTRL=0,0,2\r\n", sizeof("AT^TRACECTRL=0,1,2\r\n")); // enable OSI trace to AP Diag
    // iot_vat_send_cmd((uint8_t*)"AT^TRACECTRL=1,0,2\r\n", sizeof("AT^TRACECTRL=1,1,2\r\n")); // enable Modem trace to AP Diag

    microPyTaskHandle = iot_os_create_task(mp_task, NULL, MICROPY_TASK_STACK_SIZE, MICROPYTHON_TASK_PRIORITY, OPENAT_OS_CREATE_DEFAULT, "mp_task");
    return 0;
}

void appimg_exit(void) {

}

void _assert(void) {
    iot_debug_assert(false, __FILE__, (short)__LINE__);
    while(1);
}