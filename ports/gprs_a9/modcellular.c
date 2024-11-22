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

#include "modcellular.h"
#include "mphalport.h"
#include "timeout.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objexcept.h"
#include "py/objarray.h"

#include "api_info.h"
#include "api_sim.h"
#include "api_sms.h"
#include "api_ss.h"
#include "api_call.h"
#include "api_os.h"
#include "api_network.h"
#include "api_inc_network.h"
#include "api_charset.h"
#include "api_hal_pm.h"
#include "api_fs.h"

#include "time.h"
#include "py/mperrno.h"
#include "cJSON.h"

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

#define NTW_EXC_CALL_NO_DIAL_TONE 0x6E10
#define NTW_EXC_CALL_BUSY 0x6E11
#define NTW_EXC_CALL_NO_ANSWER 0x6E12
#define NTW_EXC_CALL_NO_CARRIER 0x6E13
#define NTW_EXC_CALL_TIMEOUT 0x6E14
#define NTW_EXC_CALL_INPROGRESS 0x6E15
#define NTW_EXC_CALL_UNKNOWN 0x6E16

#define SMS_SENT 1

#define MAX_NUMBER_LEN 16
#define MAX_CALLS_MISSED 15
#define MAX_CELLS 8

#define BANDS_ALL (NETWORK_FREQ_BAND_GSM_900P | NETWORK_FREQ_BAND_GSM_900E | NETWORK_FREQ_BAND_GSM_850 | NETWORK_FREQ_BAND_DCS_1800 | NETWORK_FREQ_BAND_PCS_1900)

#define MCC(x) ((x)[0] * 100 + (x)[1] * 10 + (x)[2])
#define MNC(x) ((x)[0] * 100 + (x)[1] * 10 + (x)[2])

// --------------
// Vars: statuses
// --------------

// Tracks the status on the network
uint8_t network_status = 0;
uint16_t network_exception = NTW_NO_EXC;
uint8_t network_signal_quality = 0;
uint8_t network_signal_rx_level = 0;
mp_obj_t network_status_callback = mp_const_none;

// SMS send flag
uint8_t sms_send_flag = 0;
uint8_t ussd_send_flag = 0;
mp_obj_t ussd_response = mp_const_none;

typedef struct _sms_obj_t {
    mp_obj_base_t base;
    mp_obj_t phone_number;
    mp_obj_t message;
    uint8_t pn_type;
    uint8_t index;
    uint8_t purpose;
} sms_obj_t;

// -------------------
// Vars: SMS retrieval
// -------------------

// A buffer used for listing messages
mp_obj_list_t *sms_list_buffer = NULL;
uint8_t sms_list_buffer_count = 0;

// SMS received
mp_obj_t sms_callback = mp_const_none;


// USSD received
mp_obj_t ussd_callback = mp_const_none;

// ---------------------------
// Vars: Network operator list
// ---------------------------

uint8_t network_list_buffer_len = 0;
Network_Operator_Info_t *network_list_buffer = NULL;

// -------------------
// Vars: base stations
// -------------------

Network_Location_t cells[MAX_CELLS];
int8_t cells_n = 0;

// -----------
// Vars: Calls
// -----------

// SMS parsing
STATIC mp_obj_t modcellular_sms_from_record(SMS_Message_Info_t* record);
STATIC mp_obj_t modcellular_sms_from_raw(uint8_t* header, uint32_t header_length, uint8_t* content, uint32_t content_length);

// Incoming call
mp_obj_t call_callback = mp_const_none;

// ----
// Init
// ----

void modcellular_init0(void) {
    // Reset callbacks
    network_status_callback = mp_const_none;
    sms_callback = mp_const_none;
    call_callback = mp_const_none;
    ussd_callback = mp_const_none;

    // Reset statuses
    network_exception = NTW_NO_EXC;
    cells_n = 0;

    // Set bands to default
    Network_SetFrequencyBand(NETWORK_FREQ_BAND_GSM_900P | NETWORK_FREQ_BAND_GSM_900E | NETWORK_FREQ_BAND_GSM_850 | NETWORK_FREQ_BAND_DCS_1800 | NETWORK_FREQ_BAND_PCS_1900);

    // Turn off flight mode
    Network_SetFlightMode(0);

    // Set SMS storage
    if (!SMS_SetFormat(SMS_FORMAT_TEXT, SIM0))
        mp_printf(&mp_plat_print, "Warning: modcellular_init0 failed to reset SMS format\n");

    SMS_Parameter_t smsParam = {
        .fo = 17 , // stadard values
        .vp = 167,
        .pid= 0  ,
        .dcs= 8  , // 0:English 7bit, 4:English 8 bit, 8:Unicode 2 Bytes
    };

    if (!SMS_SetParameter(&smsParam, SIM0))
        mp_printf(&mp_plat_print, "Warning: modcellular_init0 failed to reset SMS parameters\n");

    if (!SMS_SetNewMessageStorage(SMS_STORAGE_SIM_CARD))
        mp_printf(&mp_plat_print, "Warning: modcellular_init0 failed to reset SMS storage\n");
}

// ----------
// Exceptions
// ----------

NORETURN void mp_raise_RuntimeError(const char *msg) {
    mp_raise_msg(&mp_type_RuntimeError, msg);
}

// ------
// Notify
// ------

void modcellular_network_status_update(uint8_t new_status, uint16_t new_exception) {
    if (new_exception) network_exception = new_exception;
    network_status = new_status;
    if (network_status_callback && network_status_callback != mp_const_none) mp_sched_schedule(network_status_callback, mp_obj_new_int(network_status));
}

void modcellular_notify_no_sim(API_Event_t* event) {
    modcellular_network_status_update(0, NTW_EXC_NOSIM);
}

void modcellular_notify_sim_drop(API_Event_t* event) {
    modcellular_network_status_update(0, NTW_EXC_SIM_DROP);
}

// Register

void modcellular_notify_reg_home(API_Event_t* event) {
    modcellular_network_status_update(NTW_REG_BIT, 0);
}

void modcellular_notify_reg_roaming(API_Event_t* event) {
    modcellular_network_status_update(NTW_REG_BIT | NTW_ROAM_BIT, 0);
}

void modcellular_notify_reg_searching(API_Event_t* event) {
    modcellular_network_status_update(NTW_REG_PROGRESS_BIT, 0);
}

void modcellular_notify_reg_denied(API_Event_t* event) {
    modcellular_network_status_update(0, NTW_EXC_REG_DENIED);
}

void modcellular_notify_dereg(API_Event_t* event) {
    modcellular_network_status_update(0, 0);
}

// Attach

void modcellular_notify_det(API_Event_t* event) {
    modcellular_network_status_update(network_status & ~NTW_ATT_BIT, 0);
}

void modcellular_notify_att_failed(API_Event_t* event) {
    modcellular_network_status_update(network_status & ~NTW_ATT_BIT, NTW_EXC_ATT_FAILED);
}

void modcellular_notify_att(API_Event_t* event) {
    modcellular_network_status_update(network_status | NTW_ATT_BIT, 0);
}

// Activate

void modcellular_notify_deact(API_Event_t* event) {
    modcellular_network_status_update(network_status & ~NTW_ACT_BIT, 0);
}

void modcellular_notify_act_failed(API_Event_t* event) {
    modcellular_network_status_update(network_status & ~NTW_ACT_BIT, NTW_EXC_ACT_FAILED);
}

void modcellular_notify_act(API_Event_t* event) {
    modcellular_network_status_update(network_status | NTW_ACT_BIT, 0);
}

// Networks

void modcellular_notify_ntwlist(API_Event_t* event) {
    network_list_buffer_len = event->param1;

    if (network_list_buffer != NULL)
        free(network_list_buffer);
    network_list_buffer = malloc(sizeof(Network_Operator_Info_t) * (network_list_buffer_len + 1));  // One more item added for empty outputs
    if (network_list_buffer != NULL)
        memcpy(network_list_buffer, event->pParam1, sizeof(Network_Operator_Info_t) * network_list_buffer_len);
}

// SMS

void modcellular_notify_sms_list(API_Event_t* event) {
    SMS_Message_Info_t* messageInfo = (SMS_Message_Info_t*)event->pParam1;
    if (sms_list_buffer && (sms_list_buffer->len > sms_list_buffer_count)) {
        sms_list_buffer->items[sms_list_buffer_count] = modcellular_sms_from_record(messageInfo);
        sms_list_buffer_count ++;
    } else {
        network_exception = NTW_EXC_SMS_DROP;
    }
    OS_Free(messageInfo->data);
}

void modcellular_notify_sms_sent(API_Event_t* event) {
    sms_send_flag = 1;
    if (sms_callback && sms_callback != mp_const_none)
        mp_sched_schedule(sms_callback, mp_obj_new_int(SMS_SENT));
}

void modcellular_notify_sms_error(API_Event_t* event) {
    network_exception = NTW_EXC_SMS_SEND;
}

int modcellular_endswith(const char *str, const char *suffix) {
    if (!str || !suffix) return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void modcellular_remove_files(char *suffix) {
    Dir_t* dir = API_FS_OpenDir("/");
    const Dirent_t* entry = NULL;
    while ((entry = API_FS_ReadDir(dir))) {
        if(modcellular_endswith(entry->d_name, suffix)) {
            mp_printf(&mp_plat_print, "Remove %s ... ", entry->d_name);
            int res = API_FS_Delete(entry->d_name);
            if(res == 0) mp_printf(&mp_plat_print, "success\n");
            else mp_printf(&mp_plat_print, "failed\n");
        }
    }
    API_FS_CloseDir(dir);
}

int modcellular_new_settings(char *file, const char* sname, const char* ssub) {
    int result = 0;
    Dir_t* dir = API_FS_OpenDir("/");
    const Dirent_t* entry = NULL;
    while ((entry = API_FS_ReadDir(dir))) {
        if(strcmp(entry->d_name, file) == 0) {
            mp_printf(&mp_plat_print, "Open %s ... ", file);
            int fd  = API_FS_Open(file, FS_O_RDWR, 0);
            if(fd >= 0) {
                mp_printf(&mp_plat_print, "success\n");
                char buff[255];
                memset(buff, 0, sizeof(buff));
                int res = API_FS_Read(fd, (uint8_t*)buff, sizeof(buff));
                API_FS_Close(fd);
                if(res > 0 && res < sizeof(buff) - 1) {
                    cJSON *json = cJSON_Parse(buff);
                    if(json != NULL) {
                        if(cJSON_HasObjectItem(json, "sname") && cJSON_HasObjectItem(json, "ssub") &&
                           sname != NULL && ssub != NULL && strlen(sname) <= 20 && strlen(ssub) <= 20) {
                            cJSON_ReplaceItemInObjectCaseSensitive(json, "sname", cJSON_CreateString(sname));
                            cJSON_ReplaceItemInObjectCaseSensitive(json, "ssub", cJSON_CreateString(ssub));
                            char *mess = cJSON_PrintUnformatted(json);
                            if(mess != NULL) {
                                mp_printf(&mp_plat_print, "Write to %s -> %s ... ", file, mess);
                                if(API_FS_Delete(file) == 0) {
                                    fd = API_FS_Open(file, FS_O_RDWR | FS_O_CREAT, 0);
                                    if(fd >= 0) {
                                        res = API_FS_Write(fd, (uint8_t*)mess, strlen(mess));
                                        if(res == strlen(mess)) {
                                            mp_printf(&mp_plat_print, "success\n");
                                            result = 1;
                                        }
                                        else mp_printf(&mp_plat_print, "failed\n");
                                        API_FS_Flush(fd);
                                        API_FS_Close(fd);
                                        OS_Sleep(1000);
                                    }
                                }
                                free(mess);
                            }
                        } else mp_printf(&mp_plat_print, "sname or ssub not found or too long\n");
                        cJSON_Delete(json);
                    }
                }
            } else mp_printf(&mp_plat_print, "failed\n");
        }
    }
    API_FS_CloseDir(dir);
    return result;
}


void modcellular_notify_sms_receipt(API_Event_t* event) {

    SMS_Encode_Type_t encodeType = event->param1;
    uint32_t content_length = event->param2;
    uint8_t* header = event->pParam1;
    uint8_t* content = event->pParam2;

    bool to_reset = false;
    char *pfiles = ".py";
    char *tfiles = ".txt";
    char *vfiles = ".version";

    if(strcmp((char*)content, "rmcode") == 0) {
      mp_printf(&mp_plat_print, "Remove versions...\n");
      modcellular_remove_files(vfiles);
      mp_printf(&mp_plat_print, "Remove code...\n");
      modcellular_remove_files(pfiles);
      to_reset = true;
    }
    if(strcmp((char*)content, "rmconfig") == 0) {
      mp_printf(&mp_plat_print, "Remove config...\n");
      modcellular_remove_files(tfiles);
      to_reset = true;
    }
    if(strcmp((char*)content, "rmall") == 0) {
      mp_printf(&mp_plat_print, "Remove all...\n");
      modcellular_remove_files(pfiles);
      modcellular_remove_files(tfiles);
      modcellular_remove_files(vfiles);
      to_reset = true;
    }
    if(strcmp((char*)content, "reset") == 0) {
      mp_printf(&mp_plat_print, "Reset device...\n");
      to_reset = true;
    }
    if(strncmp((char*)content, "set ", 4) == 0) {
      char sname [52];
      char ssub [52];
      memset(sname, 0, sizeof(sname));
      memset(ssub, 0, sizeof(ssub));
      int len = strlen((char*)content);
      char * last = (char*)content + len;
      char * firstSpace = strstr((char*)content, " ");
      if(firstSpace != NULL && ((int)(firstSpace + 2 - (char*)content)) < len) {
          char * secondSpace = strstr(firstSpace + 2, " ");
          if(secondSpace != NULL && ((int)(secondSpace + 2 - (char*)content)) < len) {
              if(firstSpace < secondSpace && ((int)(secondSpace - firstSpace)) > 0 && ((int)(secondSpace - firstSpace)) <= 50 &&
                 secondSpace < last && ((int)(last - secondSpace)) > 0 && ((int)(last - secondSpace)) <= 50) {
                  strncpy(sname, firstSpace + 1, ((int)(secondSpace - firstSpace)) - 1);
                  strncpy(ssub, secondSpace + 1, ((int)(last - secondSpace)));
                  mp_printf(&mp_plat_print, "Set device sname = '%s' ssub = '%s'\n", sname, ssub);
                  if(modcellular_new_settings("settings.txt", sname, ssub) == 1) to_reset = true;
              } else mp_printf(&mp_plat_print, "Incorrect format (too long)\n");
          } else mp_printf(&mp_plat_print, "Incorrect format (SS)\n");
      } else mp_printf(&mp_plat_print, "Incorrect format (FS)\n");
    }
    if(to_reset) {

        // SMS sender phone number in form \0"+XXXXXXXXXXXX",...
        uint32_t i;
        for (i = 1; i < strlen((char*)(header + 1)); i++) {
            if (header[i] == '"') break;
        }

        if (header[i] == '"' && i < 20) {
            uint8_t phone[20];
            memset(phone, 0, sizeof(phone));
            strncpy((char*)phone, (char*)(header + 1), i - 1);
            const char* message = "Done";
            uint8_t* unicode = NULL;
            uint32_t unicodeLen;
            if (SMS_LocalLanguage2Unicode((uint8_t*)message, strlen(message), CHARSET_UTF_8, &unicode, &unicodeLen)) {
                sms_send_flag = 0;
                mp_printf(&mp_plat_print, "Send '%s' message to %s...\n", message, phone);
                SMS_SendMessage((char*)phone, unicode, unicodeLen, SIM0);
                OS_Free(unicode);
                WAIT_UNTIL(sms_send_flag, TIMEOUT_SMS_SEND, 100, mp_warning(NULL, "The module will continue attempts sending SMS"));
                sms_send_flag = 0;
            }
        }

        // restart module
        mp_printf(&mp_plat_print, "Restarting module...\n");
        PM_Restart();
        while(1) OS_Sleep(1000);
    }

    if (sms_callback && sms_callback != mp_const_none) {
       uint8_t* gbk = NULL;
       uint32_t gbkLen = 0;

        if (encodeType == SMS_ENCODE_TYPE_UNICODE) {
            if (!SMS_Unicode2LocalLanguage(content,content_length,CHARSET_UTF_8,&gbk,&gbkLen)) {
                 mp_raise_ValueError("Failed to convert sms to Unicode");
            }
        }
        else {
            gbk = content;
            gbkLen = content_length;
        }

        mp_obj_t sms = modcellular_sms_from_raw(header, strlen((char*) header), gbk, gbkLen);

        if (encodeType == SMS_ENCODE_TYPE_UNICODE) {
            OS_Free(gbk);
        }

        mp_sched_schedule(sms_callback, sms);
    }
}

// Signal level

void modcellular_notify_signal(API_Event_t* event) {
    network_signal_quality = event->param1;
    network_signal_rx_level = event->param2;
}

// Calls

void modcellular_notify_call_incoming(API_Event_t* event) {
    if (call_callback && call_callback != mp_const_none)
        mp_sched_schedule(call_callback, mp_obj_new_str((char*) event->pParam1, strlen((char*) event->pParam1)));
}

void modcellular_notify_call_hangup(API_Event_t* event) {
    if (event->param2)
        network_exception = (uint8_t) event->param2 + 0x0F;
    if (call_callback && call_callback != mp_const_none)
        mp_sched_schedule(call_callback, mp_obj_new_bool(event->param1));
}

// USSD
static unsigned char utf16_to_utf8(const uint8_t * content, uint8_t * output, unsigned int clen)
{
    if (clen < 2) return 0;  // input ends unexpectedly

    int c = 0;
    unsigned int code;

    for (int i = 0; i < clen; i += 2) {
        code = (*(content + i) << 8) + *(content + i + 1); // get the first utf16 sequence in BE
        if (code >= 0xD800 && code <= 0xDFFF) break; // Unsupported surrogate pair \uXXXX\uXXXX

        int shift = 0;
        if(code == 0x00AB || code == 0x00BB) { shift = 0xC200; }
        else if(code <= 0x00FF) { *(output + c) = (uint8_t)(code & 0x00FF); c++;  } // ASCII
        else if(code >= 0x0410 && code <= 0x043F) shift = 0xCC80;  // Russian А-Яа-п
        else if(code >= 0x0440 && code <= 0x044F) shift = 0xCD40; // Russian р-я
        else if(code == 0x0401) shift = 0xCC80; // Russian Ё
        else if(code == 0x0451) shift = 0xCD40; // Russian ё
        else break;

        if(shift > 0) {
             *(output + c) = (uint8_t)(((code + shift) & 0xFF00) >> 8);
             *(output + c + 1) = (uint8_t)((code + shift) & 0x00FF);
             c += 2;
        }
    }
    return c;
}

mp_obj_t decode_ussd(API_Event_t* event) {
    USSD_Type_t* incoming = (USSD_Type_t*) event->pParam1;
    uint8_t dcs = incoming->dcs;
    uint8_t clen = incoming->usdStringSize;
    uint8_t *content = incoming->usdString;

    //mp_printf(&mp_plat_print, "DCS = 0x%02x\n", dcs);
    //mp_printf(&mp_plat_print, "Content length = 0x%02x\n", clen);
    //mp_printf(&mp_plat_print, "Content = {");
    //for(int i = 0; i < incoming->usdStringSize; i++) mp_printf(&mp_plat_print, "0x%02x, ", content[i]);
    //mp_printf(&mp_plat_print, "};\n");

    mp_obj_t tuple[2];
    uint8_t code[clen + 1];
    int len;

    if((dcs & 0x80) == 0x00) {
        switch (dcs & 0x0C) {
            case 0x00:
            case 0x0C:
              //decode 7bitASCII
              len = GSM_7BitTo8Bit(content, code, clen);
              tuple[0] = mp_obj_new_int(incoming->option);
              tuple[1] = mp_obj_new_bytes(code, len);
              break;
            case 0x04:
              //decode UTF8
              tuple[0] = mp_obj_new_int(incoming->option);
              tuple[1] = mp_obj_new_bytes(content, clen); // mp_obj_new_str((char*)content, clen);
              break;
            case 0x08:
              //decode UTF16-BE to UTF-8
              len = utf16_to_utf8(content, code, clen);
              tuple[0] = mp_obj_new_int(incoming->option);
              tuple[1] = mp_obj_new_bytes(code, len);
              break;
            default:
              mp_raise_RuntimeError("Unsupported data coding scheme\n");
              tuple[0] = mp_const_none;
              tuple[1] = mp_const_none;
              break;
        }
    } else mp_raise_RuntimeError("Unsupported data coding scheme\n");

    return mp_obj_new_tuple(2, tuple);
}


void modcellular_notify_ussd_sent(API_Event_t* event) {
    ussd_send_flag = 1;
    ussd_response = decode_ussd(event);
    if (ussd_callback && ussd_callback != mp_const_none)
        mp_sched_schedule(ussd_callback, ussd_response);
}

void modcellular_notify_ussd_failed(API_Event_t* event) {
    network_exception = NTW_EXC_USSD_SEND;
}

void modcellular_notify_incoming_ussd(API_Event_t* event) {
    if (ussd_callback && ussd_callback != mp_const_none)
        mp_sched_schedule(ussd_callback, decode_ussd(event));
}

// Base stations

void modcellular_notify_cell_info(API_Event_t* event) {
    cells_n = event->param1;
    memcpy(cells, event->pParam1, MIN(cells_n * sizeof(Network_Location_t), sizeof(cells)));
}

// -------
// Classes
// -------

mp_obj_t modcellular_sms_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // ========================================
    // SMS class.
    // Args:
    //     phone_number (str): the phone number, source or destination;
    //     message (str): message contents;
    // ========================================

    enum { ARG_phone_number, ARG_message, ARG_pn_type, ARG_index, ARG_purpose };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_phone_number, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_message, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_pn_type, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, 
        { MP_QSTR_index, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, 
        { MP_QSTR_purpose, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, 
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    sms_obj_t *self = m_new_obj(sms_obj_t);
    self->base.type = type;
    self->phone_number = args[ARG_phone_number].u_obj;
    self->message = args[ARG_message].u_obj;
    self->pn_type = args[ARG_pn_type].u_int;
    self->index = args[ARG_index].u_int;
    self->purpose = args[ARG_purpose].u_int;
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t modcellular_sms_send(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Sends an SMS messsage.
    // Args:
    //     timeout (int): optional timeout in ms;
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    mp_int_t timeout = TIMEOUT_SMS_SEND;
    if (n_args == 2)
        timeout = mp_obj_get_int(args[1]);

    sms_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (self->purpose != 0)
        mp_raise_ValueError("A message with non-zero purpose cannot be sent");

    const char* destination_c = mp_obj_str_get_str(self->phone_number);
    const char* message_c = mp_obj_str_get_str(self->message);

    uint8_t* unicode = NULL;
    uint32_t unicodeLen;

    if (!SMS_LocalLanguage2Unicode((uint8_t*)message_c, strlen(message_c), CHARSET_UTF_8, &unicode, &unicodeLen))
        mp_raise_ValueError("Failed to convert to Unicode before sending SMS");

    sms_send_flag = 0;
    if (!SMS_SendMessage(destination_c, unicode, unicodeLen, SIM0)) {
        OS_Free(unicode);
        mp_raise_ValueError("Failed to submit SMS message for sending");
    }
    OS_Free(unicode);

    WAIT_UNTIL(sms_send_flag, timeout, 100, mp_warning(NULL, "Failed to send SMS immidiately. The module will continue attempts sending it"));

    sms_send_flag = 0;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_sms_send_obj, 1, 2, modcellular_sms_send);

STATIC mp_obj_t modcellular_sms_delete(mp_obj_t self_in) {
    // ========================================
    // Deletes an SMS message from the SIM card.
    // ========================================

    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->index == 0 || self->purpose == 0) {
        mp_raise_ValueError("Cannot delete SMS with zero index/purpose");
        return mp_const_none;
    }

    if (!SMS_DeleteMessage(self->index, SMS_STATUS_ALL, SMS_STORAGE_SIM_CARD)) {
        mp_raise_ValueError("Failed to delete SMS");
        return mp_const_none;
    }

    self->purpose = 0;
    self->index = 0;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modcellular_sms_delete_obj, &modcellular_sms_delete);

STATIC void modcellular_sms_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // ========================================
    // SMS.[attr]
    // ========================================
    if (dest[0] != MP_OBJ_NULL) {
    } else {
        sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
        // .phone_number
        if (attr == MP_QSTR_phone_number) {
            dest[0] = self->phone_number;
        // .message
        } else if (attr == MP_QSTR_message) {
            dest[0] = self->message;
        // .pn_type
        } else if (attr == MP_QSTR_pn_type) {
            dest[0] = mp_obj_new_int(self->pn_type);
        // .index
        } else if (attr == MP_QSTR_index) {
            dest[0] = mp_obj_new_int(self->index);
        // .purpose
        } else if (attr == MP_QSTR_purpose) {
            dest[0] = mp_obj_new_int(self->purpose);
        // .is_inbox
        } else if (attr == MP_QSTR_is_inbox) {
            dest[0] = mp_obj_new_bool(self->purpose & (SMS_STATUS_READ | SMS_STATUS_UNREAD));
        // .is_read
        } else if (attr == MP_QSTR_is_read) {
            dest[0] = mp_obj_new_bool(self->purpose & SMS_STATUS_READ);
        // .is_unread
        } else if (attr == MP_QSTR_is_unread) {
            dest[0] = mp_obj_new_bool(self->purpose & SMS_STATUS_UNREAD);
        // .is_unsent
        } else if (attr == MP_QSTR_is_unsent) {
            dest[0] = mp_obj_new_bool(self->purpose & SMS_STATUS_UNSENT);
        // .send
        } else if (attr == MP_QSTR_send) {
            mp_convert_member_lookup(self_in, mp_obj_get_type(self_in), (mp_obj_t)MP_ROM_PTR(&modcellular_sms_send_obj), dest);
        // .delete
        } else if (attr == MP_QSTR_delete) {
            mp_convert_member_lookup(self_in, mp_obj_get_type(self_in), (mp_obj_t)MP_ROM_PTR(&modcellular_sms_delete_obj), dest);
        }
    }
}

STATIC void modcellular_sms_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    // ========================================
    // SMS.__str__()
    // ========================================
    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SMS(\"%s\", \"%s\", pn_type=%d, index=%d, purpose=%d)\n",
            mp_obj_str_get_str(self->phone_number),
            mp_obj_str_get_str(self->message),
            self->pn_type,
            self->index,
            self->purpose
    );
}

STATIC const mp_rom_map_elem_t sms_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&modcellular_sms_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&modcellular_sms_delete_obj) },
};

STATIC MP_DEFINE_CONST_DICT(sms_locals_dict, sms_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    modcellular_sms_type,
    MP_QSTR_SMS,
    MP_TYPE_FLAG_NONE,
    make_new, modcellular_sms_make_new,
    print, modcellular_sms_print,
    attr, modcellular_sms_attr,
    locals_dict, &sms_locals_dict
    );

// -------
// Private
// -------

STATIC mp_obj_t modcellular_sms_from_record(SMS_Message_Info_t* record) {
    // ========================================
    // Prepares an SMS object from the record.
    // Returns:
    //     A new SMS object.
    // ========================================

    sms_obj_t *self = m_new_obj_with_finaliser(sms_obj_t);
    self->base.type = &modcellular_sms_type;
    self->index = record->index;
    self->purpose = (uint8_t)record->status;
    self->pn_type = (uint8_t)record->phoneNumberType;
    self->phone_number = mp_obj_new_str((char*)record->phoneNumber + 1, SMS_PHONE_NUMBER_MAX_LEN - 1);

    /*
    // TODO: make correct decoding for 0xA7
    // to translate Russian symbols
    // workaround now: #define MICROPY_PY_BUILTINS_STR_UNICODE_CHECK (0) in mpcondifport.h
    for(int i = 0; i < record->dataLen; i++) {
       uint8_t ch = *(record->data + i);
       Trace(1, "sms decode char 0x%02x ('%c')", ch, (char)ch);
       if(ch == 0xA7) {
          Trace(1, "sms decode 1");
          *(record->data + i) = 0xD0;
          if(i + 1 < record->dataLen) {
              uint8_t ch2 = *(record->data + i + 1);
              uint8_t ch3 = 0;
              Trace(1, "sms decode 2 char2 0x%02x ('%c')", ch2, (char)ch2);
              if(ch2 >= 0xA1 && ch2 <= 0xA6) ch3 = ch2 - 0x11; // Russian А-Е
              else if(ch2 >= 0xA8 && ch2 <= 0xC1) ch3 = ch2 - 0x12; // Russian Ж-Я
              else if(ch2 >= 0xD1 && ch2 <= 0xD6) ch3 = ch2 - 0x21; // Russian а-е
              else if(ch2 >= 0xD8 && ch2 <= 0xE1) ch3 = ch2 - 0x22; // Russian ж-п
              else if(ch2 >= 0xE2 && ch2 <= 0xF1) { *(record->data + i) = 0xD1;  ch3 = ch2 - 0x62; } // Russian р-я
              else if(ch2 == 0xA7) ch3 = 0x81; // Russian Ё
              else if(ch2 == 0xD7) {  *(record->data + i) = 0xD1; ch3 = 0x91; } // Russian ё
              else { *(record->data + i) = 0x2A; ch3 = 0x2A; } // replace with "*"
              *(record->data + i + 1) = ch3;
              //else if(ch == 0xAB || ch == 0xBB) *(record->data + i) = 0x22;
              Trace(1, "sms decode 2 char3 0x%02x ('%c')", ch3, (char)ch3);
              Trace(1, "sms decode 2  i = %i", i);
              mp_obj_new_str((char*)record->data, i + 1);

          } else {
                Trace(1, "sms decode 3!!!");
                mp_obj_new_str((char*)record->data, i);
          }
       } else {
            Trace(1, "sms decode 4  i = %i", i); //else if(ch == 0xAB || ch == 0xBB) *(record->data + i) = 0x22;
            mp_obj_new_str((char*)record->data, i);
       }
    }*/
    self->message = mp_obj_new_bytes(record->data, record->dataLen);
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t modcellular_sms_from_raw(uint8_t* header, uint32_t header_length, uint8_t* content, uint32_t content_length) {
    // ========================================
    // Prepares an SMS object from raw header and contents.
    // Returns:
    //     A new SMS object.
    // ========================================

    // TODO: This function is not tested / not used. It parses
    // raw event data and assembles a message.
    if (header[0] != '"') {
        return mp_const_none;
    }

    uint32_t i;
    for (i=1; i<header_length; i++) {
        if (header[i] == '"') {
            break;
        }
    }

    if (header[i] != '"') {
        return mp_const_none;
    }

    sms_obj_t *self = m_new_obj_with_finaliser(sms_obj_t);
    self->base.type = &modcellular_sms_type;
    self->index = 0;
    self->purpose = 0;
    self->pn_type = 0;
    self->phone_number = mp_obj_new_str((char*)header + 1, i - 1);
    self->message = mp_obj_new_bytes(content, content_length);
    return MP_OBJ_FROM_PTR(self);
}

// -------
// Methods
// -------

STATIC mp_obj_t modcellular_get_signal_quality(void) {
    // ========================================
    // Retrieves the network signal quality.
    // Returns:
    //     Two integers: quality
    // ========================================
    mp_obj_t tuple[2] = {
        network_signal_quality == 99 ? mp_const_none : mp_obj_new_int(network_signal_quality),
        network_signal_rx_level == 99 ? mp_const_none : mp_obj_new_int(network_signal_rx_level),
    };
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_get_signal_quality_obj, modcellular_get_signal_quality);

STATIC mp_obj_t modcellular_get_imei(void) {
    // ========================================
    // Retrieves IMEI number.
    // Returns:
    //     IMEI number as a string.
    // ========================================
    char imei[16];
    memset(imei,0,sizeof(imei));
    INFO_GetIMEI((uint8_t*)imei);
    return mp_obj_new_str(imei, strlen(imei));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_get_imei_obj, modcellular_get_imei);

STATIC mp_obj_t modcellular_is_sim_present(void) {
    // ========================================
    // Checks whether the SIM card is inserted and ICCID can be retrieved.
    // Returns:
    //     True if SIM present.
    // ========================================
    char iccid[21];
    memset(iccid, 0, sizeof(iccid));
    return mp_obj_new_bool(SIM_GetICCID((uint8_t*)iccid));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_is_sim_present_obj, modcellular_is_sim_present);

STATIC mp_obj_t modcellular_poll_network_exception(void) {
    // ========================================
    // Raises a last network exception.
    // ========================================
    uint8_t e = network_exception;
    network_exception = NTW_NO_EXC;

    switch (e) {
        case NTW_NO_EXC:
            break;

        default:
            mp_raise_OSError(e);
            break;

    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_poll_network_exception_obj, modcellular_poll_network_exception);

STATIC mp_obj_t modcellular_get_network_status(void) {
    // ========================================
    // Retrieves the network status.
    // Returns:
    //     Network status as an integer.
    // ========================================
    return mp_obj_new_int(network_status);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_get_network_status_obj, modcellular_get_network_status);

STATIC mp_obj_t modcellular_is_network_registered(void) {
    // ========================================
    // Checks whether registered on the cellular network.
    // Returns:
    //     True if registered.
    // ========================================
    return mp_obj_new_bool(network_status & NTW_REG_BIT);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_is_network_registered_obj, modcellular_is_network_registered);

STATIC mp_obj_t modcellular_is_roaming(void) {
    // ========================================
    // Checks whether registered on the roaming network.
    // Returns:
    //     True if roaming.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    return mp_obj_new_bool(network_status & NTW_ROAM_BIT);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_is_roaming_obj, modcellular_is_roaming);

STATIC mp_obj_t modcellular_get_iccid(void) {
    // ========================================
    // Retrieves ICCID number.
    // Returns:
    //     ICCID number as a string.
    // ========================================
    char iccid[21];
    memset(iccid, 0, sizeof(iccid));
    if (SIM_GetICCID((uint8_t*)iccid))
        return mp_obj_new_str(iccid, strlen(iccid));
    else {
        mp_raise_RuntimeError("No ICCID data available");
        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_get_iccid_obj, modcellular_get_iccid);

STATIC mp_obj_t modcellular_get_imsi(void) {
    // ========================================
    // Retrieves IMSI number.
    // Returns:
    //     IMSI number as a string.
    // ========================================
    char imsi[21];
    memset(imsi, 0, sizeof(imsi));
    if (SIM_GetIMSI((uint8_t*)imsi))
        return mp_obj_new_str(imsi, strlen(imsi));
    else {
        mp_raise_RuntimeError("No IMSI data available");
        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_get_imsi_obj, modcellular_get_imsi);

bool get_flight_mode(void) {
    // Polls flight mode
    bool flag;
    if (!Network_GetFlightMode(&flag)) {
        mp_raise_RuntimeError("Failed to retrieve flight mode status");
    }
    return !flag;  // By fact, the meaning of the output is inverse
}

STATIC mp_obj_t modcellular_flight_mode(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Retrieves and switches the flight mode
    // status.
    // Returns:
    //     The new flight mode status.
    // ========================================
    if (n_args == 1) {
        mp_int_t set_flag = mp_obj_get_int(args[0]);
        if (!Network_SetFlightMode(set_flag)) {
            mp_raise_RuntimeError("Failed to set flight mode status");
            return mp_const_none;
        }
        WAIT_UNTIL(set_flag == get_flight_mode(), TIMEOUT_FLIGHT_MODE, 100, mp_raise_OSError(MP_ETIMEDOUT));
    }
    return mp_obj_new_bool(get_flight_mode());
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_flight_mode_obj, 0, 1, modcellular_flight_mode);

STATIC mp_obj_t modcellular_set_bands(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Sets 2G bands the module operates at.
    // Args:
    //     bands (int): a mask specifying
    //     bands;
    // ========================================
    if (n_args == 0) {
        if (!Network_SetFrequencyBand(BANDS_ALL)) {
            mp_raise_RuntimeError("Failed to reset 2G GSM bands");
            return mp_const_none;
        }
    } else if (n_args == 1) {
        if (!Network_SetFrequencyBand(mp_obj_get_int(args[0]))) {
            mp_raise_RuntimeError("Failed to set 2G GSM bands");
            return mp_const_none;
        }
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_set_bands_obj, 0, 1, modcellular_set_bands);

bool __is_attached(void) {
    uint8_t status;
    if (Network_GetAttachStatus(&status)) {
        return status;
    }
    return false;
}

STATIC mp_obj_t modcellular_gprs(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Polls and switches GPRS status.
    // Args:
    //     apn (str, bool): access point name
    //     or False if GPRS shutdown requested.
    //     user (str): username;
    //     pass (str): password;
    //     timeout (int): time to wait until
    //     connected;
    // Returns:
    //     True if GPRS is active, False
    //     otherwise.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    if (n_args == 1 || n_args == 2) {
        mp_int_t flag = mp_obj_get_int(args[0]);
        if (flag != 0) {
            mp_raise_ValueError("Unkown integer argument supplied, zero (or False) expected");
            return mp_const_none;
        }
        mp_int_t timeout = TIMEOUT_GPRS_ACTIVATION;
        if (n_args == 2) timeout = mp_obj_get_int(args[1]);

        if (network_status & NTW_ACT_BIT) {
            if (!Network_StartDeactive(1)) {
                mp_raise_RuntimeError("Cannot initiate context deactivation");
                return mp_const_none;
            }
            WAIT_UNTIL(!(network_status & NTW_ACT_BIT), timeout, 100, mp_raise_OSError(MP_ETIMEDOUT));
        }

        if (network_status & NTW_ATT_BIT)
            if (Network_StartDetach())
                WAIT_UNTIL(!(network_status & NTW_ATT_BIT), TIMEOUT_GPRS_ATTACHMENT, 100, break);

    } else if (n_args == 3 || n_args == 4) {
        const char* c_apn = mp_obj_str_get_str(args[0]);
        const char* c_user = mp_obj_str_get_str(args[1]);
        const char* c_pass = mp_obj_str_get_str(args[2]);
        mp_int_t timeout = TIMEOUT_GPRS_ACTIVATION;
        if (n_args == 4) timeout = mp_obj_get_int(args[3]);

        if (network_status & NTW_ACT_BIT) {
            mp_raise_ValueError("GPRS is already on");
            return mp_const_none;
        }

        uint8_t ret;
        //mp_printf(&mp_plat_print, "Deactivate... ");
        ret = Network_StartDeactive(1);
        WAIT_UNTIL(!(network_status & NTW_ACT_BIT), timeout, 100, mp_raise_RuntimeError("Not detactivated: try resetting"));
        //mp_printf(&mp_plat_print, " ret=%d attached=%d active=%d\n", ret, (network_status & NTW_ATT_BIT) != 0, (network_status & NTW_ACT_BIT) != 0);

        //mp_printf(&mp_plat_print, "Detach ... ");
        ret = Network_StartDetach();
        WAIT_UNTIL(!(network_status & NTW_ATT_BIT), TIMEOUT_GPRS_ATTACHMENT, 100, mp_raise_RuntimeError("Not detached: try resetting"));
        //mp_printf(&mp_plat_print, " ret=%d attached=%d active=%d\n", ret, (network_status & NTW_ATT_BIT) != 0, (network_status & NTW_ACT_BIT) != 0);

        //mp_printf(&mp_plat_print, "Attach... ");
        ret = Network_StartAttach();
        WAIT_UNTIL((network_status & NTW_ATT_BIT), TIMEOUT_GPRS_ATTACHMENT, 100, mp_raise_RuntimeError("Not attached: try resetting"));
        //mp_printf(&mp_plat_print, " ret=%d attached=%d active=%d\n", ret, (network_status & NTW_ATT_BIT) != 0, (network_status & NTW_ACT_BIT) != 0);

        Network_PDP_Context_t context;
        memcpy(context.apn, c_apn, MIN(strlen(c_apn) + 1, sizeof(context.apn)));
        memcpy(context.userName, c_user, MIN(strlen(c_user) + 1, sizeof(context.userName)));
        memcpy(context.userPasswd, c_pass, MIN(strlen(c_pass) + 1, sizeof(context.userPasswd)));

        //mp_printf(&mp_plat_print, "Activate... ");
        ret = Network_StartActive(context);
        WAIT_UNTIL((network_status & NTW_ACT_BIT), timeout, 100, mp_raise_RuntimeError("Not activated: try resetting"));
        //mp_printf(&mp_plat_print, " ret=%d attached=%d active=%d\n", ret, (network_status & NTW_ATT_BIT) != 0, (network_status & NTW_ACT_BIT) != 0);
    } else if (n_args != 0) {
        mp_raise_ValueError("Unexpected number of argument: 0, 1 or 3 required");
    }

    return mp_obj_new_bool(network_status & NTW_ACT_BIT);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_gprs_obj, 0, 4, modcellular_gprs);

STATIC mp_obj_t modcellular_scan(void) {
    // ========================================
    // Lists network operators.
    // ========================================
    network_list_buffer = NULL;
    if (!Network_GetAvailableOperatorReq()) {
        mp_raise_RuntimeError("Failed to poll available operators");
        return mp_const_none;
    }
    WAIT_UNTIL(network_list_buffer != NULL, TIMEOUT_LIST_OPERATORS, 100, mp_raise_OSError(MP_ETIMEDOUT));

    mp_obj_t items[network_list_buffer_len];
    for (int i=0; i < network_list_buffer_len; i++) {

        // Name
        uint8_t *op_name;
        if (!Network_GetOperatorNameById(network_list_buffer[i].operatorId, &op_name)) {
            mp_raise_RuntimeError("Failed to poll operator name");
            return mp_const_none;
        }

        mp_obj_t tuple[3] = {
            mp_obj_new_bytearray(sizeof(network_list_buffer[i].operatorId), network_list_buffer[i].operatorId),
            mp_obj_new_str((char*) op_name, strlen((char*) op_name)),
            mp_obj_new_int(network_list_buffer[i].status),
        };
        items[i] = mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
    }

    free(network_list_buffer);

    return mp_obj_new_list(sizeof(items) / sizeof(mp_obj_t), items);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_scan_obj, modcellular_scan);

STATIC mp_obj_t modcellular_register(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Lists network operators.
    // ========================================
    if (n_args == 1) {
        mp_int_t flag = mp_obj_get_int(args[0]);
        if (flag != 0) {
            mp_raise_ValueError("Unkown integer argument supplied, zero (or False) expected");
            return mp_const_none;
        }

        if (!Network_DeRegister()) {
            mp_raise_RuntimeError("Failed to request deregistration");
            return mp_const_none;
        }
        WAIT_UNTIL(!(network_status & NTW_REG_BIT), TIMEOUT_REG, 100, mp_raise_OSError(MP_ETIMEDOUT));

    } else if (n_args == 2) {
        mp_obj_array_t *op_id = MP_OBJ_TO_PTR(args[0]);
        mp_int_t op_mode = mp_obj_get_int(args[1]);

        if (op_id->base.type != &mp_type_bytearray) {
            mp_raise_ValueError("A bytearray expected");
            return mp_const_none;
        }

        if (op_id->len != 6) {
            mp_raise_ValueError("The length of the input bytearray should be 6");
            return mp_const_none;
        }

        if (op_mode != NETWORK_REGISTER_MODE_MANUAL && op_mode != NETWORK_REGISTER_MODE_AUTO && op_mode != NETWORK_REGISTER_MODE_MANUAL_AUTO) {
            mp_raise_ValueError("The mode should be one of NETWORK_REGISTER_MODE_*");
            return mp_const_none;
        }

        if (!Network_Register((uint8_t*) op_id->items, (Network_Register_Mode_t) op_mode)) {
            mp_raise_RuntimeError("Failed to request network registration");
            return mp_const_none;
        }
        WAIT_UNTIL(network_status & NTW_REG_BIT, TIMEOUT_REG, 100, mp_raise_OSError(MP_ETIMEDOUT));
    }

    uint8_t op_id[6];
    Network_Register_Mode_t op_mode;

    if (!Network_GetCurrentOperator(op_id, &op_mode)) {
        mp_raise_RuntimeError("Failed to poll current operator");
        return mp_const_none;
    }

    uint8_t *op_name;
    if (!Network_GetOperatorNameById(op_id, &op_name)) {
        mp_raise_RuntimeError("Failed to poll operator name");
        return mp_const_none;
    }

    mp_obj_t tuple[3] = {
        mp_obj_new_bytearray(sizeof(op_id), op_id),
        mp_obj_new_str((char*) op_name, strlen((char*) op_name)),
        mp_obj_new_int((uint8_t) op_mode),
    };

    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_register_obj, 0, 2, modcellular_register);

STATIC mp_obj_t modcellular_dial(mp_obj_t tn_in) {
    // ========================================
    // Dial a number.
    // Args:
    //     tn (str, bool): the telephone number
    //     or False if hangup requested;
    // ========================================
    if (mp_obj_is_str(tn_in)) {
        const char* tn = mp_obj_str_get_str(tn_in);
        if (!CALL_Dial(tn)) {
            mp_raise_RuntimeError("Failed to initiate an outgoing call");
        }
        return mp_const_none;
    } else {
        if (!mp_obj_is_true(tn_in)) {
            if (!CALL_HangUp()) {
                mp_raise_RuntimeError("Failed to hangup call");
            }
        } else {
            mp_raise_ValueError("The argument must be a string or False");
        }
        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modcellular_dial_obj, modcellular_dial);

STATIC mp_obj_t modcellular_ussd(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // USSD request.
    // Args:
    //     code (str): the request;
    //     timeout (int, optional): timeout in ms;
    // Returns:
    //     USSD response for non-zero timeouts.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    mp_int_t timeout = TIMEOUT_USSD_RESPONSE;
    if (n_args == 2)
        timeout = mp_obj_get_int(args[1]);

    const char* code = mp_obj_str_get_str(args[0]);
    if(strlen(code) == 0) {
        mp_raise_RuntimeError("USSD string should not be empty");
        return mp_const_none;
    }
    uint8_t code7[2 * strlen(code) + 1];
    int len = GSM_8BitTo7Bit((const uint8_t*) code, code7, strlen(code));

    USSD_Type_t ussd;
    ussd.usdString = (uint8_t*) code7;
    ussd.usdStringSize = len;
    ussd.option = 3;
    ussd.dcs = 0x0F;

    ussd_send_flag = 0;
    ussd_response = mp_const_none;

    int result = SS_SendUSSD(ussd);
    if (result) {
        mp_printf(&mp_plat_print, "Failed to submit USSD: 0x%02x\n", result);
        mp_raise_RuntimeError("Failed to submit USSD");
    }

    if (timeout == 0)
        return mp_const_none;

    WAIT_UNTIL(ussd_send_flag, timeout, 100, mp_raise_OSError(MP_ETIMEDOUT));
    mp_obj_t rtn = ussd_response;
    ussd_send_flag = 0;
    ussd_response = 0;
    return rtn;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_ussd_obj, 1, 2, modcellular_ussd);

STATIC mp_obj_t modcellular_stations(void) {
    // ========================================
    // Polls base stations.
    // ========================================
    cells_n = -1;
    if (!Network_GetCellInfoRequst()) {
        mp_raise_RuntimeError("Failed to poll base stations");
        return mp_const_none;
    }
    WAIT_UNTIL(cells_n >= 0, TIMEOUT_STATIONS, 100, mp_raise_OSError(MP_ETIMEDOUT));

    mp_obj_t stations[cells_n];
    for (int i=0; i<cells_n; i++) {
        mp_obj_t tuple[8] = {
            mp_obj_new_int(MCC(cells[i].sMcc)),
            mp_obj_new_int(MNC(cells[i].sMnc)),
            mp_obj_new_int(cells[i].sLac),
            mp_obj_new_int(cells[i].sCellID),
            mp_obj_new_int(cells[i].iBsic),
            mp_obj_new_int(cells[i].iRxLev),
            mp_obj_new_int(cells[i].iRxLevSub),
            mp_obj_new_int(cells[i].nArfcn),
        };
        stations[i] = mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
    }
    return mp_obj_new_tuple(sizeof(stations) / sizeof(mp_obj_t), stations);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_stations_obj, modcellular_stations);

STATIC mp_obj_t modcellular_agps_station_data(void) {
    // ========================================
    // Polls base stations and returns mcc, mnc
    // and a list of lac, cell_id, signal level
    // for each base station.
    // ========================================
    cells_n = -1;
    if (!Network_GetCellInfoRequst()) {
        mp_raise_RuntimeError("Failed to poll base stations");
        return mp_const_none;
    }
    WAIT_UNTIL(cells_n >= 0, TIMEOUT_STATIONS, 100, mp_raise_OSError(MP_ETIMEDOUT));

    if (cells_n == 0) {
        mp_raise_RuntimeError("No station data available");
        return mp_const_none;
    }

    // just return the first mcc/mnc in the list
    int mcc = MCC(cells[0].sMcc);
    int mnc = MNC(cells[0].sMnc);
    mp_obj_t list = mp_obj_new_list(0, NULL);
    for (int i=0; i<cells_n; i++) {
        if (MCC(cells[i].sMcc) == mcc && MNC(cells[i].sMnc) == mnc) {
            mp_obj_t tuple[3] = {
                mp_obj_new_int(cells[i].sLac),
                mp_obj_new_int(cells[i].sCellID),
                mp_obj_new_int(cells[i].iRxLev),
            };
            mp_obj_list_append(list, mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple));
        }
    }
    mp_obj_t tuple[3] = {
        mp_obj_new_int(mcc),
        mp_obj_new_int(mnc),
        list,
    };
    return mp_obj_new_tuple(3, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_agps_station_data_obj, modcellular_agps_station_data);

STATIC mp_obj_t modcellular_reset(void) {
    // ========================================
    // Resets network settings to defaults.
    // ========================================
    modcellular_init0();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_reset_obj, modcellular_reset);

STATIC mp_obj_t modcellular_on_status_event(mp_obj_t callable) {
    // ========================================
    // Sets a callback on status event.
    // Args:
    //     callback (Callable): a callback to
    //     execute on status event.
    // ========================================
    network_status_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modcellular_on_status_event_obj, modcellular_on_status_event);

STATIC mp_obj_t modcellular_on_sms(mp_obj_t callable) {
    // ========================================
    // Sets a callback on SMS (receive).
    // Args:
    //     callback (Callable): a callback to
    //     execute on SMS receive.
    // ========================================
    sms_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modcellular_on_sms_obj, modcellular_on_sms);

STATIC mp_obj_t modcellular_on_call(mp_obj_t callable) {
    // ========================================
    // Sets a callback on incoming calls.
    // Args:
    //     callback (Callable): a callback to
    //     execute on incoming call.
    // ========================================
    call_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modcellular_on_call_obj, modcellular_on_call);

STATIC mp_obj_t modcellular_on_ussd(mp_obj_t callable) {
    // ========================================
    // Sets a callback on USSD.
    // Args:
    //     callback (Callable): a callback to
    //     execute on incoming USSD.
    // ========================================
    ussd_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modcellular_on_ussd_obj, modcellular_on_ussd);


STATIC mp_obj_t modcellular_sms_delete_by_index(mp_obj_t index) {
    // ========================================
    // Delete an sms message by its index.
    // Args:
    //     index (int): the index of the SMS;
    // ========================================

    mp_int_t int_index = mp_obj_get_int(index);

    if (!int_index) {
        mp_raise_ValueError("Cannot delete SMS with zero index");
        return mp_const_none;
    }

    if (!SMS_DeleteMessage(int_index, SMS_STATUS_ALL, SMS_STORAGE_SIM_CARD)) {
        mp_raise_ValueError("Failed to delete SMS");
        return mp_const_none;
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modcellular_sms_delete_by_index_obj, modcellular_sms_delete_by_index);

STATIC mp_obj_t modcellular_sms_delete_all_read() {
    // ========================================
    // Deletes all sms messages
    // Args:
    //     index (int): the index of the SMS;
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    SMS_Storage_Info_t storage;
    SMS_GetStorageInfo(&storage, SMS_STORAGE_SIM_CARD);

    // Read SMSs
    sms_list_buffer = mp_obj_new_list(storage.readRecords, NULL);
    sms_list_buffer_count = 0;

    SMS_ListMessageRequst(SMS_STATUS_READ, SMS_STORAGE_SIM_CARD);
    WAIT_UNTIL(sms_list_buffer_count == storage.readRecords, TIMEOUT_SMS_LIST, 100, mp_warning(NULL, "Failed to poll all SMS: the list may be incomplete"));

    mp_obj_list_t *result = sms_list_buffer;
    //mp_printf(&mp_plat_print, "Read list len = %d\n", result->len);

    uint16_t i;
    for (i = 0; i < result->len; i++) {
        if(result->items[i] != NULL) {
            sms_obj_t* sms = MP_OBJ_FROM_PTR(result->items[i]);
            // mp_printf(&mp_plat_print, "Read index[%d] = %d\n", i, sms->index);
            if(sms->index != 0) SMS_DeleteMessage(sms->index, SMS_STATUS_ALL, SMS_STORAGE_SIM_CARD);
        }
        result->items[i] = mp_const_none;
    }
    sms_list_buffer = NULL;
    sms_list_buffer_count = 0;

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_delete_all_read_obj, modcellular_sms_delete_all_read);

// reads SMS
STATIC mp_obj_t modcellular_sms_read_all(void) {
    // ========================================
    // Read all SMS messages with "unRead" state.
    // Returns:
    //     A list of SMS messages.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    SMS_Storage_Info_t storage;
    SMS_GetStorageInfo(&storage, SMS_STORAGE_SIM_CARD);
    bool res = true;

    while(storage.unReadRecords > 0) {
        sms_list_buffer = mp_obj_new_list(1, NULL);
        sms_list_buffer_count = 0;

        // read message one by one
        SMS_ListMessageRequst(SMS_STATUS_ALL, SMS_STORAGE_SIM_CARD);
        WAIT_UNTIL(sms_list_buffer_count == 1, TIMEOUT_SMS_LIST, 100, mp_warning(NULL, "Failed to read SMS from SIM"));

        if(sms_list_buffer_count == 0) res = false;
        mp_hal_delay_ms(500); // wait for storage info update

        SMS_GetStorageInfo(&storage, SMS_STORAGE_SIM_CARD);

        sms_list_buffer = NULL;
        sms_list_buffer_count = 0;
    }
    return mp_obj_new_bool(res);
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_read_all_obj, modcellular_sms_read_all);

STATIC mp_obj_t modcellular_sms_list_read(void) {
    // ========================================
    // Lists READ SMS messages.
    // Returns:
    //     A list of SMS messages.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    SMS_Storage_Info_t storage;
    SMS_GetStorageInfo(&storage, SMS_STORAGE_SIM_CARD);

    sms_list_buffer = mp_obj_new_list(storage.readRecords, NULL);
    sms_list_buffer_count = 0;

    SMS_ListMessageRequst(SMS_STATUS_READ, SMS_STORAGE_SIM_CARD);
    WAIT_UNTIL(sms_list_buffer_count == storage.readRecords, TIMEOUT_SMS_LIST, 100, mp_warning(NULL, "Failed to poll all READ SMS: the list may be incomplete"));

    mp_obj_list_t *result = sms_list_buffer;
    sms_list_buffer = NULL;
    for (uint16_t i = sms_list_buffer_count; i < result->len; i++) result->items[i] = mp_const_none;
    sms_list_buffer_count = 0;

    return (mp_obj_t)result;
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_list_read_obj, modcellular_sms_list_read);

STATIC mp_obj_t modcellular_sms_get_storage_size(void) {
    // ========================================
    // Retrieves SMS storage size.
    // Returns:
    //     Storage used and total size as ints.
    // ========================================
    SMS_Storage_Info_t storage;

    SMS_GetStorageInfo(&storage, SMS_STORAGE_SIM_CARD);

    mp_obj_t tuple[8] = {
        mp_obj_new_int(storage.used),
        mp_obj_new_int(storage.total),
        mp_obj_new_int(storage.unReadRecords),
        mp_obj_new_int(storage.readRecords),
        mp_obj_new_int(storage.sentRecords),
        mp_obj_new_int(storage.unsentRecords),
        mp_obj_new_int(storage.unknownRecords),
        mp_obj_new_int(storage.storageId),
    };
    return mp_obj_new_tuple(8, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_get_storage_size_obj, modcellular_sms_get_storage_size);




STATIC const mp_map_elem_t mp_module_cellular_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_cellular) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_SMS), (mp_obj_t)MP_ROM_PTR(&modcellular_sms_type) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_get_imei), (mp_obj_t)&modcellular_get_imei_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_signal_quality), (mp_obj_t)&modcellular_get_signal_quality_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_poll_network_exception), (mp_obj_t)&modcellular_poll_network_exception_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_network_status), (mp_obj_t)&modcellular_get_network_status_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_sim_present), (mp_obj_t)&modcellular_is_sim_present_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_network_registered), (mp_obj_t)&modcellular_is_network_registered_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_roaming), (mp_obj_t)&modcellular_is_roaming_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_iccid), (mp_obj_t)&modcellular_get_iccid_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_imsi), (mp_obj_t)&modcellular_get_imsi_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_flight_mode), (mp_obj_t)&modcellular_flight_mode_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_bands), (mp_obj_t)&modcellular_set_bands_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_gprs), (mp_obj_t)&modcellular_gprs_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_scan), (mp_obj_t)&modcellular_scan_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_register), (mp_obj_t)&modcellular_register_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_dial), (mp_obj_t)&modcellular_dial_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ussd), (mp_obj_t)&modcellular_ussd_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_stations), (mp_obj_t)&modcellular_stations_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_agps_station_data), (mp_obj_t)&modcellular_agps_station_data_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset), (mp_obj_t)&modcellular_reset_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_status_event), (mp_obj_t)&modcellular_on_status_event_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_sms), (mp_obj_t)&modcellular_on_sms_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_call), (mp_obj_t)&modcellular_on_call_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_ussd), (mp_obj_t)&modcellular_on_ussd_obj },

    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_GSM_900P), MP_ROM_INT(NETWORK_FREQ_BAND_GSM_900P) },
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_GSM_900E), MP_ROM_INT(NETWORK_FREQ_BAND_GSM_900E) },
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_GSM_850),  MP_ROM_INT(NETWORK_FREQ_BAND_GSM_850)  },
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_DCS_1800), MP_ROM_INT(NETWORK_FREQ_BAND_DCS_1800) },
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_PCS_1900), MP_ROM_INT(NETWORK_FREQ_BAND_PCS_1900) },
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BANDS_ALL), MP_ROM_INT(BANDS_ALL) },

    { MP_ROM_QSTR(MP_QSTR_OPERATOR_STATUS_UNKNOWN), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_OPERATOR_STATUS_AVAILABLE), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_OPERATOR_STATUS_CURRENT), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_OPERATOR_STATUS_DISABLED), MP_ROM_INT(3) },

    { MP_ROM_QSTR(MP_QSTR_NETWORK_MODE_MANUAL),      MP_ROM_INT(NETWORK_REGISTER_MODE_MANUAL)      },
    { MP_ROM_QSTR(MP_QSTR_NETWORK_MODE_AUTO),        MP_ROM_INT(NETWORK_REGISTER_MODE_AUTO)        },
    { MP_ROM_QSTR(MP_QSTR_NETWORK_MODE_MANUAL_AUTO), MP_ROM_INT(NETWORK_REGISTER_MODE_MANUAL_AUTO) },

    { MP_ROM_QSTR(MP_QSTR_SMS_SENT), MP_ROM_INT(SMS_SENT) },

    { MP_ROM_QSTR(MP_QSTR_ENOSIM), MP_ROM_INT(NTW_EXC_NOSIM) },
    { MP_ROM_QSTR(MP_QSTR_EREGD), MP_ROM_INT(NTW_EXC_REG_DENIED) },
    { MP_ROM_QSTR(MP_QSTR_ESMSSEND), MP_ROM_INT(NTW_EXC_SMS_SEND) },
    { MP_ROM_QSTR(MP_QSTR_ESMSDROP), MP_ROM_INT(NTW_EXC_SMS_DROP) },
    { MP_ROM_QSTR(MP_QSTR_ESIMDROP), MP_ROM_INT(NTW_EXC_SIM_DROP) },
    { MP_ROM_QSTR(MP_QSTR_EATTACHMENT), MP_ROM_INT(NTW_EXC_ATT_FAILED) },
    { MP_ROM_QSTR(MP_QSTR_EACTIVATION), MP_ROM_INT(NTW_EXC_ACT_FAILED) },
    { MP_ROM_QSTR(MP_QSTR_ENODIALTONE), MP_ROM_INT(NTW_EXC_CALL_NO_DIAL_TONE) },
    { MP_ROM_QSTR(MP_QSTR_EBUSY), MP_ROM_INT(NTW_EXC_CALL_BUSY) },
    { MP_ROM_QSTR(MP_QSTR_ENOANSWER), MP_ROM_INT(NTW_EXC_CALL_NO_ANSWER) },
    { MP_ROM_QSTR(MP_QSTR_ENOCARRIER), MP_ROM_INT(NTW_EXC_CALL_NO_CARRIER) },
    { MP_ROM_QSTR(MP_QSTR_ECALLTIMEOUT), MP_ROM_INT(NTW_EXC_CALL_TIMEOUT) },
    { MP_ROM_QSTR(MP_QSTR_ECALLINPROGRESS), MP_ROM_INT(NTW_EXC_CALL_INPROGRESS) },
    { MP_ROM_QSTR(MP_QSTR_ECALLUNKNOWN), MP_ROM_INT(NTW_EXC_CALL_UNKNOWN) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_delete_by_index), (mp_obj_t)&modcellular_sms_delete_by_index_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_delete_all_read), (mp_obj_t)&modcellular_sms_delete_all_read_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_read_all), (mp_obj_t)&modcellular_sms_read_all_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_list_read), (mp_obj_t)&modcellular_sms_list_read_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_get_storage_size), (mp_obj_t)&modcellular_sms_get_storage_size_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_cellular_globals, mp_module_cellular_globals_table);

const mp_obj_module_t cellular_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_cellular_globals,
};

MP_REGISTER_MODULE(MP_QSTR_cellular, cellular_module);