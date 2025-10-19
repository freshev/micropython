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
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_mobile.h"
#include "luat_debug.h"
#include "luat_mem.h"
#include "luat_pm.h"
#include "luat_uart.h"
#include "luat_network_adapter.h"
#include "plat_config.h"
#include "mw_nvm_sms.h"

// priority in %
#define MICROPYTHON_TASK_PRIORITY   (20) 

#define MICROPY_HEAP_MAX_SIZE       (1024 * 300)
#define MICROPY_HEAP_MIN_SIZE       (1024 * 10) 

#define MP_TASK_STACK_LIMIT_MARGIN  (2048)
#define MICROPY_TASK_STACK_SIZE     (16 * 1024)

#define MP_FATAL_REASON_NLR_JUMP_FAIL   (1)
#define MP_FATAL_REASON_HEAP_INIT       (2)


luat_rtos_task_handle microPyTaskHandle = NULL;

void NORETURN mp_fatal_error(uint8_t reason, void* ptr1);
void NORETURN nlr_jump_fail(void *val) {
    mp_fatal_error(MP_FATAL_REASON_NLR_JUMP_FAIL, val);
    while(1);
}

void* mp_allocate_heap(uint32_t* size) {
    uint32_t h_size = MICROPY_HEAP_MAX_SIZE;
    void* ptr = NULL;
    int counter = 0;
    while (!ptr) {
        // LUAT_DEBUG_PRINT("Try %d heap size", h_size);
        if (h_size < MICROPY_HEAP_MIN_SIZE) mp_fatal_error(MP_FATAL_REASON_HEAP_INIT, NULL);
        ptr = luat_heap_malloc(h_size);
        if (!ptr) {
             h_size = h_size - MICROPY_HEAP_MIN_SIZE;
             counter++;
        }
    }

    
    #ifdef FOTA_USE
    { 
    luat_heap_free(ptr);
    h_size -=  2 * MBEDTLS_SSL_MAX_CONTENT_LEN; // leave 32K buffer for MBEDTLS, HTTP deflate/inflate
    ptr = luat_heap_malloc(h_size);
    if (!ptr) mp_fatal_error(MP_FATAL_REASON_HEAP_INIT, NULL);
    }
    #endif

    size[0] = h_size;
    if(counter == 0) LUAT_DEBUG_PRINT("MICROPY_HEAP_MAX_SIZE can be increased!");
    LUAT_DEBUG_PRINT("Final heap size: %d ", h_size);
    return ptr;
}

extern void soc_uart0_set_log_off(uint8_t is_off);

void mp_task(void *param) {

#ifdef HALTONEXC
    // If the system crashes, it will print information instead of restarting.
    BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_FAULT_ACTION, 0); 
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

    
    soc_uart0_set_log_off(1);
    luat_uart_pre_setup(0, 1);


    mp_init();           // LUAT_DEBUG_PRINT("micropython inited");
    modmachine_init0();  // LUAT_DEBUG_PRINT("machine inited");
    readline_init0();    // LUAT_DEBUG_PRINT("readline inited");
    modcellular_init0(); // LUAT_DEBUG_PRINT("cellular inited");
#ifdef GPS_MODULE
    //modgps_init0(); LUAT_DEBUG_PRINT("GPS inited");
#endif

    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_));
    mp_obj_list_init(mp_sys_argv, 0);
    // LUAT_DEBUG_PRINT("sys path inited");

    // Startup scripts
    pyexec_frozen_module("_boot.py", false);
    pyexec_file_if_exists("boot.py");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
#ifdef MAINRUN
        // LUAT_DEBUG_PRINT("execute main.py");
        pyexec_file_if_exists("main.py");
#endif
    }
    // LUAT_DEBUG_PRINT("start REPL");
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
    luat_heap_free(mp_task_heap);
    mp_hal_stdout_tx_str("PYB: soft reboot\r\n");
    mp_hal_delay_ms(10);// allow UART to flush output
    // LUAT_DEBUG_PRINT("soft reset");

    goto soft_reset;
}


MP_REGISTER_ROOT_POINTER(char *readline_hist[8]); // mp history buffer

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
                LUAT_DEBUG_PRINT("%s", trace_debug_buffer);
                memset(trace_debug_buffer, 0, sizeof(trace_debug_buffer));
                trace_debug_buffer_len = 0;
            }
            if (str[i] != '\n') trace_debug_buffer[trace_debug_buffer_len++] = str[i];
        }
    }
}
const mp_print_t mp_debug_print = {NULL, debug_print_strn};
#endif


static const char msg1[] =
    "\r\n========================================================="
    "\r\nMicropython experienced a fatal error and will be halted."
    "\r\nReason: ";
static const char msg2[] = "\r\n  ptr1: ";

static const char msg_nlr_jump_fail[] = "nlr jump fail";
static const char msg_heap_init[] = "heap init fail";
static const char msg_unknown[] = "unknown";
static const char msg_reboot[] = "Rebooting...\r\n";


char mp_fatal_buffer[16];
void NORETURN mp_fatal_error(uint8_t reason, void* ptr1) {
    // ========================================
    // A fatal error. Avoid using stack.
    // ========================================
    LUAT_DEBUG_PRINT("Fatal error reason: %d", reason);
    #if MICROPY_HW_ENABLE_UART_REPL

    luat_uart_write(HW_UART_REPL, (uint8_t*)msg1, sizeof(msg1));
    switch (reason) {
        case MP_FATAL_REASON_NLR_JUMP_FAIL:
            luat_uart_write(HW_UART_REPL, (uint8_t*)msg_nlr_jump_fail, sizeof(msg_nlr_jump_fail));
            break;
        case MP_FATAL_REASON_HEAP_INIT:
            luat_uart_write(HW_UART_REPL, (uint8_t*)msg_heap_init, sizeof(msg_heap_init));
            break;
        default:
            luat_uart_write(HW_UART_REPL, (uint8_t*)msg_unknown, sizeof(msg_unknown));
    }
    luat_uart_write(HW_UART_REPL, (uint8_t*)msg2, sizeof(msg2));
    snprintf(mp_fatal_buffer, sizeof(mp_fatal_buffer), "%p\r\n", ptr1);
    luat_uart_write(HW_UART_REPL, (uint8_t*)mp_fatal_buffer, strlen(mp_fatal_buffer)+1);
    luat_uart_write(HW_UART_REPL, (uint8_t*)msg_reboot, sizeof(msg_reboot));
    #endif
    
    luat_rtos_task_sleep(1000); 
    
    luat_pm_reboot();
    while(1);
}

void task_init(void) {  
    luat_rtos_task_create(&microPyTaskHandle, MICROPY_TASK_STACK_SIZE, MICROPYTHON_TASK_PRIORITY, "mp_task", mp_task, NULL, NULL);
}

INIT_TASK_EXPORT(task_init, "0");
