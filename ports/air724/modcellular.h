/*                                                                                          
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 freshev
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

#ifndef __MODCELLULAR_H
#define __MODCELLULAR_H

#include "mphalport.h"
#include "math.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objexcept.h"
#include "py/objarray.h"
#include "py/mperrno.h"

#define IOT_MSG_MAX_ADDR_LEN 80
#define IOT_SMS_MAX_TXT_SIZE 640
#define IOT_SMS_MAX_PDU_SIZE 180
#define IOT_SMS_MAX_LENGTH_OF_ADDRESS_VALUE 40
#define IOT_SMS_MAX_ADDR_STR_MAX_LEN ((LUAT_SMS_MAX_LENGTH_OF_ADDRESS_VALUE + 1) * 4)

#define PLMN_STR_MAX_LENGTH 7

/*
typedef void (*LUAT_SMS_HANDLE_CB)(uint8_t event, void* param);
typedef void (*LUAT_SMS_HANDLE_SEND_CB)(int ret);
typedef struct {
    LUAT_SMS_HANDLE_CB cb;
    LUAT_SMS_HANDLE_SEND_CB send_cb;
} LUAT_SMS_MAIN_CFG_T;

typedef enum {
    SMS_SEND_OK = 0,
    SMS_ME_FAILURE = 300,
    SMS_SERVICE_OF_ME_RESV,
    SMS_OPERATION_NOT_ALLOWED,
    SMS_OPERATION_NOT_SUPPORTED,
    SMS_INVALID_PDU_MODE_PARAMETER,
    SMS_INVALID_TEXT_MODE_PARAMETER,
    SMS_USIM_NOT_INSERTED = 310,
    SMS_USIM_PIN_REQUIRED,
    SMS_PHSIM_PIN_REQUIRED,
    SMS_USIM_FAILURE,
    SMS_USIM_BUSY,
    SMS_USIM_WRONG,
    SMS_USIM_PUK_REQUIRED,
    SMS_USIM_PIN2_REQUIRED,
    SMS_USIM_PUK2_REQUIRED,
    SMS_MEMORY_FAILURE = 320,
    SMS_INVALID_MEM_INDEX,
    SMS_MEM_FULL,
    SMS_SMSC_ADDR_UNKNOWN = 330,
    SMS_NO_NETWORK_SERVICE,
    SMS_NETWORK_TIMEOUT,
    SMS_NO_CNMA_ACK_EXPECTED = 340,
    SMS_UNKNOWN_ERROR = 500,
    SMS_INVALID_DATA  = 550,
    SMS_UNSUPPORT_TEXT_WITH_CHINESE = 555,
    SMS_MAX_ERROR = 0xFFFF
} LUAT_SMS_SEND_RET_CODE_E;
*/


#define TIMEOUT_SMS_LIST 10000
#define TIMEOUT_SMS_SEND 10000
#define TIMEOUT_SMS_READ 10000
#define TIMEOUT_SMS_DELETE 10000
#define TIMEOUT_NETWORK_ACTIVATION 20000
#define TIMEOUT_FLIGHT_MODE 10000
#define TIMEOUT_LIST_OPERATORS 70000

#define NTW_REG_BIT 0x01
#define NTW_ROAM_BIT 0x02
#define NTW_REG_PROGRESS_BIT 0x04
#define NTW_ATT_BIT 0x08
#define NTW_ACT_BIT 0x10

#define NTW_NO_EXC 0
#define NTW_EXC_NOSIM 0x6E01
#define NTW_EXC_REG_DENIED 0x6E02
#define NTW_EXC_SMS_SEND 0x6E03
#define NTW_EXC_SIM_DROP 0x6E04
#define NTW_EXC_ATT_FAILED 0x6E05
#define NTW_EXC_ACT_FAILED 0x6E06
#define NTW_EXC_SMS_DROP 0x6E07
#define NTW_EXC_USSD_SEND 0x6E08
#define NTW_EXC_PLMN_LIST 0x6E09
#define NTW_EXC_REG_FAILED 0x6E10

#define BANDS_ALL (999)


typedef struct _sms_obj_t {
    mp_obj_base_t base;
    mp_obj_t phone_number;
    mp_obj_t message;
    uint8_t pn_type;
    uint8_t index;
    uint8_t type;
    uint8_t dcs;
    uint8_t usim_toolkit;
    uint8_t combined_sms;
    uint8_t combined_sms_reference;
    uint8_t combined_sms_parts;
    uint8_t combined_sms_part_number;
    uint16_t source_port;
    uint16_t destination_port;


    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t tz;         /* time zone */
    uint8_t tzSign;     /* '+'/'-' */
} sms_obj_t;

NORETURN void mp_raise_RuntimeError(const char *msg) { mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT((char*)msg)); }

#define REQUIRES_NETWORK_REGISTRATION do {if (!network_status) {mp_raise_RuntimeError("Network is not available: is SIM card inserted?"); return mp_const_none;}} while(0)

#endif