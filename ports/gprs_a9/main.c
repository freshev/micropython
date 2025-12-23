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
#include "shared/readline/readline.h"

#include "stdbool.h"
#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"
#include "api_hal_pm.h"
#include "api_hal_uart.h"
#include "buffer.h"
#include "api_network.h"
#include "time.h"
#include "api_fs.h"
#include "fatal.h"

#include "modos.h"
#include "mphalport.h"
#include "mpconfigport.h"
#include "modcellular.h"
#include "modgps.h"
#include "modmachine.h"

#define AppMain_TASK_STACK_SIZE      (2048 * 2)
#define AppMain_TASK_PRIORITY        (0)
#define MICROPYTHON_TASK_STACK_SIZE  (2048 * 4)
#define MICROPYTHON_TASK_PRIORITY    (1)
//#define MICROPYTHON_HEAP_MAX_SIZE    (2048 * 1024)
#define MICROPYTHON_HEAP_MAX_SIZE    (880  * 1024)
#define MICROPYTHON_HEAP_MIN_SIZE    (16   * 1024)
#define MICROPYTHON_FOTA_MAX_SIZE    (45   * 1024)

void* heap;
HANDLE mainTaskHandle  = NULL;
HANDLE microPyTaskHandle = NULL;
static void *stack_top;

extern mp_uint_t gc_helper_get_regs_and_sp(mp_uint_t* regs);
void gc_collect(void) {
    //ESP8266-style
    gc_collect_start();
    mp_uint_t regs[8];
    mp_uint_t sp = gc_helper_get_regs_and_sp(regs);
    gc_collect_root((void**)sp, (mp_uint_t)(stack_top - sp) / sizeof(mp_uint_t));
    gc_collect_end();
}

void NORETURN nlr_jump_fail(void *val) {
    mp_fatal_error(MP_FATAL_REASON_NLR_JUMP_FAIL, val);
}


void* mp_allocate_heap(uint32_t* size) {
    uint32_t h_size = MICROPYTHON_HEAP_MAX_SIZE;
    void* ptr = NULL;
    int counter = 0;
    while (!ptr) {
        // Trace(1, "FOTA Try %d heap size", h_size);
        if (h_size < MICROPYTHON_HEAP_MIN_SIZE) {
            mp_fatal_error(MP_FATAL_REASON_HEAP_INIT, NULL);
        }
        ptr = OS_Malloc(h_size);
        if (!ptr) {
            h_size = h_size - MICROPYTHON_HEAP_MIN_SIZE;
            counter++;
        }
    }

    #ifdef FOTA_USE
    { 
    // leave buffer for SDK FOTA subroutines
    // MICROPYTHON_FOTA_MAX_SIZE * 4 is the minimum memory size
    // because of SDK reallocations during FOTA pack HTTP "GET" subroutine
    OS_Free(ptr);
    h_size -=  4 * MICROPYTHON_FOTA_MAX_SIZE; 
    ptr = OS_Malloc(h_size);
    if (!ptr) mp_fatal_error(MP_FATAL_REASON_HEAP_INIT, NULL);
    }
    #endif

    size[0] = h_size;
    if(counter == 0) Trace(1, "MICROPYTHON_HEAP_MAX_SIZE can be increased!");
    Trace(1, "Final heap size: %d ", h_size);
    return ptr;
}

void MicroPyTask(void *pData) {

    OS_Task_Info_t info;
    OS_GetTaskInfo(microPyTaskHandle, &info);

soft_reset:
    mp_stack_ctrl_init();
    stack_top = (void*) info.stackTop + info.stackSize * 4;
    mp_stack_set_top((void *) stack_top);
    mp_stack_set_limit(MICROPYTHON_TASK_STACK_SIZE * 4 - 1024);

    uint32_t heap_size;
    heap = mp_allocate_heap(&heap_size);
    gc_init(heap, heap + heap_size);

    mp_init();
    modos_init0();
    modcellular_init0();
    modgps_init0();
    modmachine_init0();
    readline_init0();
    
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_));
    mp_obj_list_init(mp_sys_argv, 0);

    // Startup scripts
    OS_Sleep(3000); // Magically increases stability on "old" flashes, preventing cycle reboot. 3000 ms is the minimum.

    pyexec_frozen_module("_boot.py", false);
    pyexec_file_if_exists("boot.py");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
#ifdef MAINRUN
        pyexec_file_if_exists("main.py");
#endif
    }

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
    mp_deinit();

    OS_Free(heap);
    mp_hal_stdout_tx_str("PYB: soft reboot\r\n");
    mp_hal_delay_us(10000); // allow UART to flush output

    goto soft_reset;
}

void EventDispatch(API_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case API_EVENT_ID_POWER_ON:
            modmachine_notify_power_on(pEvent);
            break;

        case API_EVENT_ID_KEY_DOWN:
            modmachine_notify_power_key_down(pEvent);
            break;

        case API_EVENT_ID_KEY_UP:
            modmachine_notify_power_key_up(pEvent);
            break;

        // Network
        // =======

        case API_EVENT_ID_NO_SIMCARD:
            modcellular_notify_no_sim(pEvent);
            break;

        case API_EVENT_ID_SIMCARD_DROP:
            modcellular_notify_sim_drop(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTERED_HOME:
            modcellular_notify_reg_home(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
            modcellular_notify_reg_roaming(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTER_SEARCHING:
            modcellular_notify_reg_searching(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTER_DENIED:
            modcellular_notify_reg_denied(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTER_NO:
            // TODO: WTF is this?
            modcellular_notify_reg_denied(pEvent);
            break;

        case API_EVENT_ID_NETWORK_DEREGISTER:
            modcellular_notify_dereg(pEvent);
            break;

        case API_EVENT_ID_NETWORK_DETACHED:
            modcellular_notify_det(pEvent);
            break;

        case API_EVENT_ID_NETWORK_ATTACH_FAILED:
            modcellular_notify_att_failed(pEvent);
            break;

        case API_EVENT_ID_NETWORK_ATTACHED:
            modcellular_notify_att(pEvent);
            break;

        case API_EVENT_ID_NETWORK_DEACTIVED:
            modcellular_notify_deact(pEvent);
            break;

        case API_EVENT_ID_NETWORK_ACTIVATE_FAILED:
            modcellular_notify_act_failed(pEvent);
            break;

        case API_EVENT_ID_NETWORK_ACTIVATED:
            modcellular_notify_act(pEvent);
            break;

        case API_EVENT_ID_NETWORK_AVAILABEL_OPERATOR:
            modcellular_notify_ntwlist(pEvent);
            break;

        // SMS
        // ===

        case API_EVENT_ID_SMS_SENT:
            modcellular_notify_sms_sent(pEvent);
            break;

        case API_EVENT_ID_SMS_ERROR:
            modcellular_notify_sms_error(pEvent);
            break;

        case API_EVENT_ID_SMS_LIST_MESSAGE:
            modcellular_notify_sms_list(pEvent);
            break;

        case API_EVENT_ID_SMS_RECEIVED:
            modcellular_notify_sms_receipt(pEvent);
            break;

        // Signal
        // ======

        case API_EVENT_ID_SIGNAL_QUALITY:
            modcellular_notify_signal(pEvent);
            break;

        case API_EVENT_ID_NETWORK_CELL_INFO:
            modcellular_notify_cell_info(pEvent);
            break;

        // Call
        // ====

        case API_EVENT_ID_CALL_INCOMING:
            modcellular_notify_call_incoming(pEvent);
            break;

        case API_EVENT_ID_CALL_HANGUP:
            modcellular_notify_call_hangup(pEvent);
            break;

        // USSD
        // ====
        case API_EVENT_ID_USSD_SEND_SUCCESS:
            modcellular_notify_ussd_sent(pEvent);
            break;

        case API_EVENT_ID_USSD_SEND_FAIL:
            modcellular_notify_ussd_failed(pEvent);
            break;

        case API_EVENT_ID_USSD_IND:
            modcellular_notify_incoming_ussd(pEvent);
            break;

        // GPS
        // ===
        case API_EVENT_ID_GPS_UART_RECEIVED:
            modgps_notify_gps_update(pEvent);
            break;

        default:
            break;
    }
}

void AppMainTask(void *pData)
{
    API_Event_t* event=NULL;
    TIME_SetIsAutoUpdateRtcTime(true);
    microPyTaskHandle = OS_CreateTask(MicroPyTask, NULL, NULL, MICROPYTHON_TASK_STACK_SIZE, MICROPYTHON_TASK_PRIORITY, 0, 0, "mpy Task");
    while(1) {
        if(OS_WaitEvent(mainTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER)) {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}

void _Main(void)
{
    mainTaskHandle = OS_CreateTask(AppMainTask, NULL, NULL, AppMain_TASK_STACK_SIZE, AppMain_TASK_PRIORITY, 0, 0, "main Task");
    OS_SetUserMainHandle(&mainTaskHandle);
    ChangeWDPinStateInBoot(); // do not remove this!
    ChangeWDPinState(&mainTaskHandle);
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

static void debug_print_strn(void *env, const char *str, size_t len) {
    (void) env;
    if(microPyTaskHandle != NULL) {
        for (size_t i=0; i<len; i++) {
            if (trace_debug_buffer_len == sizeof(trace_debug_buffer) - 1 || str[i] == '\n') {
                // flush
                Trace(2, trace_debug_buffer);
                memset(trace_debug_buffer, 0, sizeof(trace_debug_buffer));
                trace_debug_buffer_len = 0;
            }
            if (str[i] != '\n') trace_debug_buffer[trace_debug_buffer_len++] = str[i];
        }
    }
}
const mp_print_t mp_debug_print = {NULL, debug_print_strn};

#endif


