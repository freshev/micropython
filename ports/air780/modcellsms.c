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
// 0041000B919740340616F500045E0B05040B84C0020003F001010B060403AE81EA02056A0045C60C036D6F62696C65746964696E67732E636F6D2F000AC30620090226162510C3062009030416250103436865636B206F7574204D6F62696C6520546964696E677321000101

#include "luat_iconv.h"
#include "modcellular.h"

#include "luat_rtos.h"
#include "luat_fs.h"
#include "luat_pm.h"
#include "cJSON.h"

#define LIST_MIN_ALLOC 4

// SMS send flag
mp_obj_t ussd_response = mp_const_none;
mp_obj_t sms_callback = mp_const_none;
mp_obj_t ussd_callback = mp_const_none;

CmiSmsGetStorageStatusCnf * storage = NULL;
mp_obj_list_t *sms_list_buffer = NULL;
uint8_t storage_flag = 0;
uint8_t sms_list_flag = 0;
uint8_t sms_delete_flag = 0;
uint8_t sms_read_flag = 0;
uint8_t sms_send_flag = 0;
uint8_t ussd_send_flag = 0;

static void modcellular_sms_recv_cb(uint8_t event,void *param) {
}

void modcellular_sms_process(sms_obj_t *sms);
void modcellular_sms_recv_custom_cb(sms_obj_t *sms) {
    LUAT_DEBUG_PRINT("sms_recv_cb: [%d]", sms->index);
    modcellular_sms_process(sms); // process internal commands
    if (sms_callback && sms_callback != mp_const_none) {
        mp_sched_schedule(sms_callback, sms);
    }
}
void modcellular_sms_send_cb(int ret) {
    LUAT_DEBUG_PRINT("sms_send_cb, send ret:[%d]", ret);
    if(ret == 0) sms_send_flag = 1;
}

// --------------------------------------------
//              SIM storage info
// --------------------------------------------
void modcellular_get_storage_info_cb(uint16_t param_size, void* p_param) { 
    smsGetPrefMemStorageInfoInSim(PS_DIAL_REQ_HANDLER, CMI_SMS_OPER_STORE_INFO_GETTING); 
}
BOOL modcellular_get_storage_info() {
    storage_flag = 0;
    storage = (CmiSmsGetStorageStatusCnf*)m_new(uint8_t, sizeof(CmiSmsGetStorageStatusCnf));
    if(storage != NULL) {
        memset(storage, 0, sizeof(CmiSmsGetStorageStatusCnf));
        cmsNonBlockApiCall(modcellular_get_storage_info_cb, 0, NULL);
        WAIT_UNTIL(storage_flag, TIMEOUT_SMS_LIST, 100, mp_raise_RuntimeError("Can not get SIM storage info");); // wait for CMI_SMS_GET_SMS_STORAGE_STATUS_CNF
        //LUAT_DEBUG_PRINT("SMS storage info:");
        //LUAT_DEBUG_PRINT("usedNumOfSim %d", storage->usedNumOfSim);
        //LUAT_DEBUG_PRINT("totalNumOfSim %d", storage->totalNumOfSim);
        //for(int i = 0; i < MIN(CI_SMS_RECORD_MAX_NUMBER,10); i++) LUAT_DEBUG_PRINT("usedIndexOfSim[%d] %d", i, storage->usedIndexOfSim[i]);
        return true;
    }
    return false;
}
void modcellular_clear_storage_info() {
    if(storage != NULL) m_del(uint8_t, storage, sizeof(CmiSmsGetStorageStatusCnf));
    storage = NULL;
}
// --------------------------------------------
//                   SMS list
// --------------------------------------------
/* typedef enum CmiSmsRecStorStatus_Enum {
        CMI_SMS_STOR_STATUS_REC_UNREAD = 0,  // Received unread message, i.e new message 
        CMI_SMS_STOR_STATUS_REC_READ   = 1,  // Received read message 
        CMI_SMS_STOR_STATUS_STO_UNSENT = 2,  // Stored unsent message only applicable to SMs 
        CMI_SMS_STOR_STATUS_STO_SENT   = 3,  // Stored sent message only applicable to SMs 
        CMI_SMS_STOR_STATUS_ALL        = 4,  // All message, only applicable to +CGML command 
        CMI_SMS_STOR_STATUS_END
} CmiSmsRecStorStatus; */

void modcellular_get_sms_list_cb(uint16_t param_size, void* p_param) { 
    smsListSmsStoredInSimSmsMsg(PS_DIAL_REQ_HANDLER, *((uint8_t*)p_param));
}
BOOL modcellular_get_sms_list(CmiSmsRecStorStatus status) {
    sms_list_flag = 0;
    if(sms_list_buffer == NULL) sms_list_buffer = mp_obj_new_list(0, NULL);
    if(sms_list_buffer != NULL) {
        cmsNonBlockApiCall(modcellular_get_sms_list_cb, sizeof(uint8_t), &status);
        WAIT_UNTIL(sms_list_flag, TIMEOUT_SMS_LIST, 100, mp_raise_RuntimeError("Can not get SMS list");); // wait for CMI_SMS_LIST_SMS_MSG_RECORD_CNF
        //LUAT_DEBUG_PRINT("SMS list with status %d:", status, sms_list_buffer->len);
        //for(int i = 0; i < sms_list_buffer->len; i++) LUAT_DEBUG_PRINT("%d: %p", i, sms_list_buffer->items[i]);
        return true;
    }
    return false;
}
void modcellular_clear_sms_list() {
    mp_obj_list_t *self = MP_OBJ_TO_PTR(sms_list_buffer);
    if(self != NULL) {
        self->len = 0;
        self->items = m_renew(mp_obj_t, self->items, self->alloc, LIST_MIN_ALLOC);
        self->alloc = LIST_MIN_ALLOC;
        mp_seq_clear(self->items, 0, self->alloc, sizeof(*self->items));
    }
}

// --------------------------------------------
//                   SMS delete
// --------------------------------------------
void modcellular_sms_delete_internal_cb(uint16_t param_size, void* p_param) { 
    smsDelStoredInSimSmsMsg(PS_DIAL_REQ_HANDLER, *((uint8_t*)p_param), 0);
}
BOOL modcellular_sms_delete_internal(uint8_t index) {
    sms_delete_flag = 0;
    cmsNonBlockApiCall(modcellular_sms_delete_internal_cb, sizeof(uint8_t), &index);
    WAIT_UNTIL(sms_delete_flag, TIMEOUT_SMS_DELETE, 100, mp_raise_RuntimeError("Can not delete SMS");); // wait for CMI_SMS_DEL_SMS_MSG_RECORD_CNF
    return true;
}


// --------------------------------------------
//               SMS event handler
// --------------------------------------------
extern void luat_sms_proc(uint32_t event, void *param);             // from "luat_sms_ec618.c"
// extern void luat_sms_nw_report_urc(CmiSmsNewMsgInd *p_cmi_msg_ind); // from "luat_sms_ec618.c"
mp_obj_t modcellular_sms_from_list_record(CmiSmsListSmsMsgRecCnf *record);

void sms_event_cb(uint32_t event, void *param) {

    void (*default_sms_proc)(uint32_t event, void *param) = luat_sms_proc;
    switch(event) {
        // call default handler from "luat_sms_ec618.c" only for sending
        case CMI_SMS_SEND_MSG_CNF: // 2
            default_sms_proc(event, param);
            break;

        // rewrite logic from "luat_sms_ec618.c" to store new message to U(SIM) storage
        case CMI_SMS_NEW_MSG_MEM_LOCATION_IND: { // 7
            CmiSmsNewMsgMemLocationInd * pSms_loc = (CmiSmsNewMsgMemLocationInd *)param;
            //LUAT_DEBUG_PRINT("New SIM loc: rc=%d, index=%d, opMode=%d", pSms_loc->rc, pSms_loc->index, pSms_loc->operatmode);
            }
            break;

        case CMI_SMS_NEW_MSG_IND: // 8
            switch (((CmiSmsNewMsgInd*)param)->smsType) {
                case CMI_SMS_TYPE_DELIVER:
                case CMI_SMS_TYPE_STATUS_REPORT:
                case CMI_SMS_TYPE_CB_ETWS_CMAS: {
                    // store to U(SIM)
                    CmiSmsNewMsgInd *p_cmi_msg_ind = (CmiSmsNewMsgInd*)param;                   
                    smsSetStoredInSimSmsMsg(PS_DIAL_REQ_HANDLER, CMI_SMS_OPER_STORE_MT_MSG, CMI_SMS_STOR_STATUS_REC_UNREAD, p_cmi_msg_ind->smscPresent, &(p_cmi_msg_ind->smscAddress), &(p_cmi_msg_ind->pdu));
                    // call luat default handler, which calls sms_recv_cb(p_cmi_msg_ind->smsType, &recv_msg_info) via "luat_sms_cfg.cb" delegate; 
                    // luat_sms_nw_report_urc(p_cmi_msg_ind); // rewritten
                    /* typedef struct CmiSmsNewMsgInd_Tag {
                        UINT8               smsType;       // CmiSmsMessageType 
                        UINT8               smsId;
                        UINT8               smsCoding;     // CmiSmsMsgCoding 
                        UINT8               smsClass;      // SmsMessageClass 
                        BOOL                smscPresent;
                        UINT8               reserved0;
                        UINT16              reserved1;
                        CmiSmsAddressInfo   smscAddress;
                        CmiSmsPdu           pdu;
                    } CmiSmsNewMsgInd; 
                    typedef struct CmiSmsListSmsMsgRecCnf_Tag {
                        CmiSmsErrorCode         errorCode;
                        UINT8                   endStatus;          // where or not is the last item 
                        BOOL                    scAddrPresent;      // SC Address info present or not 
                        UINT8                   index;              // the index of the SMS reocord in SIM 
                        UINT8                   smsStatus;          // SMS record status. CmiSmsRecStorStatus 
                        UINT8                   smsMsgType;         // SMS message type, defined type CmiSmsMessageType 
                        UINT8                   reserved0;
                        CmiSmsAddressInfo       scAddrInfo;         // SC address info 
                        CmiSmsPdu               smsPduData;         // SMS PDU contents 
                    } CmiSmsListSmsMsgRecCnf; */
                    // convert CmiSmsNewMsgInd to CmiSmsListSmsMsgRecCnf
                    CmiSmsListSmsMsgRecCnf record;
                    record.errorCode = 0;
                    record.endStatus = 1;
                    record.scAddrPresent = p_cmi_msg_ind->smscPresent;
                    record.index = 1;
                    record.smsStatus = CMI_SMS_STOR_STATUS_REC_UNREAD;
                    record.smsMsgType = p_cmi_msg_ind->smsType;
                    record.scAddrInfo = p_cmi_msg_ind->smscAddress;
                    record.smsPduData = p_cmi_msg_ind->pdu;
                    modcellular_sms_recv_custom_cb(modcellular_sms_from_list_record(&record));
                    }
                    break;
                default:
                    //LUAT_DEBUG_PRINT("unkown new_msg_ind->smsType %d", new_msg_ind->smsType);
                    break;
            }            
            //Confirm SMS
            CmiSmsNewMsgInd *p_cmi_msg_ind = (CmiSmsNewMsgInd*)param;
            CmiSmsNewMsgRsp new_msg_rsp;
            SignalBuf       *p_signal = PNULL;
            CamCmiRsp       *p_cac_cmi_rsp;            
            memset(&new_msg_rsp, 0, sizeof(CmiSmsNewMsgRsp));            
            new_msg_rsp.bRPAck = TRUE;
            new_msg_rsp.smsId = p_cmi_msg_ind->smsId;            
            OsaCreateZeroSignal(SIG_CAM_CMI_RSP, (sizeof(CamCmiRsp) + sizeof(CmiSmsNewMsgRsp)), &p_signal);
            
            p_cac_cmi_rsp = (CamCmiRsp *)(p_signal->sigBody);
            p_cac_cmi_rsp->header.rspId = CMS_SET_CMI_REQ_CNF_ID(CAM_SMS, CMI_SMS_NEW_MSG_RSP);
            p_cac_cmi_rsp->header.rspHandler = BROADCAST_IND_HANDLER;
            memcpy(p_cac_cmi_rsp->body, &new_msg_rsp, sizeof(CmiSmsNewMsgRsp));
            OsaSendSignal(CCM_TASK_ID, &p_signal);
            break;

        // custom handlers

        /* typedef struct CmiSmsGetSmsMsgRecCnf_Tag {
            CmiSmsErrorCode         errorCode;      // Response result 
            UINT8                   operatmode;     // CmiSmsOperationMode 
            UINT8                   smsStatus;      // SMS record status. CmiSmsRecStorStatus 
            UINT8                   smsMsgType;     // SMS message type, defined type CmiSmsMessageType 
            BOOL                    scAddrPresent;  / SC Address info present or not 
            UINT16                  reserved;
            CmiSmsAddressInfo       scAddrInfo;     // SC address info 
            CmiSmsPdu               smsPduData;     // SMS PDU contents 
        } CmiSmsGetSmsMsgRecCnf; */
        case CMI_SMS_GET_SMS_MSG_RECORD_CNF: { // 29
            CmiSmsGetSmsMsgRecCnf * sms_rec = (CmiSmsGetSmsMsgRecCnf *)param;
            //LUAT_DEBUG_PRINT("SMS record (GET_SMS_MSG): ");
            //LUAT_DEBUG_PRINT("smsStatus %d", sms_rec->smsStatus); // read|unread|unsent|sent
            //LUAT_DEBUG_PRINT("errorCode %d", sms_rec->errorCode);
            //LUAT_DEBUG_PRINT("pdu len %d", sms_rec->smsPduData.pduLength);
            }
            break;
        
        case CMI_SMS_SET_SMS_MSG_RECORD_CNF: { // 31
            CmiSmsSetSmsMsgRecCnf * pSms_res = (CmiSmsSetSmsMsgRecCnf *)param;
            //LUAT_DEBUG_PRINT("Set SIM mess: error=%d, index=%d, opMode=%d", pSms_res->errorCode, pSms_res->index, pSms_res->operatmode);
            if(pSms_res->errorCode != 0) LUAT_DEBUG_PRINT("Failed to store SMS into U(SIM)");
            }
            break;

        case CMI_SMS_DEL_SMS_MSG_RECORD_CNF: { // 33
            CmiSmsDelSmsMsgRecCnf * pSms_res = (CmiSmsDelSmsMsgRecCnf *)param;
            //LUAT_DEBUG_PRINT("Delete SIM: error=%d", pSms_res->errorCode);
            if(pSms_res->errorCode == 0) sms_delete_flag = 1;
            }
            break;

        case CMI_SMS_LIST_SMS_MSG_RECORD_CNF: { // 35
            CmiSmsListSmsMsgRecCnf * sms_rec = (CmiSmsListSmsMsgRecCnf *)param;
            //LUAT_DEBUG_PRINT("SMS record (LIST_SMS_MSG) %d: status=%d, end=%d, error=%d", sms_rec->index, sms_rec->smsStatus, sms_rec->endStatus, sms_rec->errorCode);
            if (sms_list_buffer && sms_rec->errorCode == 0) {
                if(sms_rec->smsStatus < CMI_SMS_STOR_STATUS_ALL) {
                    mp_obj_list_append(sms_list_buffer, modcellular_sms_from_list_record(sms_rec));
                }
            } else network_exception = NTW_EXC_SMS_DROP;    
            if(sms_rec->endStatus == 1) sms_list_flag = 1;
            }
            break;

        case CMI_SMS_GET_SMS_STORAGE_STATUS_CNF: { // 37
            CmiSmsGetStorageStatusCnf * storage_info = (CmiSmsGetStorageStatusCnf *)param;
            if(storage) { memcpy(storage, storage_info, sizeof(CmiSmsGetStorageStatusCnf)); storage_flag = 1; }
            }
            break;
        
        case CMI_SMS_MEM_CAP_IND: // 40, Report SMS Memory Capacity Exceeded flag
            LUAT_DEBUG_PRINT("SMS storage overflow");
            break;

        case CMI_SMS_IDLE_STATUS_IND: // 43, Report SMS Task IDLE state
            LUAT_DEBUG_PRINT("SMS task IDLE");              
            break;        
        
        default:
            LUAT_DEBUG_PRINT("SMS event%d,%x",event, param);
            break;
    }   
}


// --------------------------------------------
//                     SMS init
// --------------------------------------------
void modcellular_init_sms() {
    // AT+CPMS="SM","SM","SM"
    // AT+CPMS?
    // AT+CNMI=2,1 (not 1,2 !)
    // AT+CNMI?
    // Above commands can be used only in AT mode (AirM2M_780E_V1170_LTE_AT.binpkg firmware)

    MWNvmCfgCPMSParam cpmsConfig;
    memset(&cpmsConfig, 0, sizeof(MWNvmCfgCPMSParam));
    mwNvmCfgGetCpmsConfig(&cpmsConfig);
    if(cpmsConfig.mem1 != PSIL_SMS_STOR_MEM_TYPE_SM || cpmsConfig.mem2 != PSIL_SMS_STOR_MEM_TYPE_SM || cpmsConfig.mem3 != PSIL_SMS_STOR_MEM_TYPE_SM) {
        LUAT_DEBUG_PRINT("mem1: %d", cpmsConfig.mem1);
        LUAT_DEBUG_PRINT("mem2: %d", cpmsConfig.mem2);
        LUAT_DEBUG_PRINT("mem3: %d", cpmsConfig.mem3);
        // Formally set storage to U(SIM)
        MWNvmCfgSetCPMSParam pSetCpms;
        memset(&pSetCpms, 0, sizeof(MWNvmCfgSetCPMSParam));
        pSetCpms.mem1Present = pSetCpms.mem2Present = pSetCpms.mem3Present = true;
        pSetCpms.cpmsParam.mem1 = pSetCpms.cpmsParam.mem2 = pSetCpms.cpmsParam.mem3 = PSIL_SMS_STOR_MEM_TYPE_SM;  // 2 - (U)SIM, 1 - ME
        mwNvmCfgSetAndSaveCpmsConfig(&pSetCpms);
    }

    MWNvmCfgCNMIParam cnmiConfig;
    memset(&cnmiConfig, 0, sizeof(MWNvmCfgCNMIParam));
    mwNvmCfgGetCnmiConfig(&cnmiConfig);
    if(cnmiConfig.mode != 2 || cnmiConfig.mt != 1) {
        LUAT_DEBUG_PRINT("mode: %d", cnmiConfig.mode);  // the control mode for buffering URC, should be 2
        LUAT_DEBUG_PRINT("mt: %d", cnmiConfig.mt);      // SMS-DELIVER URC mode, should be 1
        // Formally set new incoming message store to U(SIM)
        MWNvmCfgSetCNMIParam pSetCnmi;
        pSetCnmi.modePresent = pSetCnmi.mtPresent = true;
        pSetCnmi.cnmiParam.mode = 2;
        pSetCnmi.cnmiParam.mt = 1;
        mwNvmCfgSetAndSaveCnmiConfig(&pSetCnmi);
    }
}

// --------------------------------------------
//                 SMS processing
// --------------------------------------------
int modcellular_endswith(const char *str, const char *suffix) {
    if (!str || !suffix) return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void modcellular_remove_files(char *suffix) {
    luat_fs_dirent_t *fs_dirent = (luat_fs_dirent_t*)m_new(uint8_t, sizeof(luat_fs_dirent_t) * 100);
    memset(fs_dirent, 0, sizeof(luat_fs_dirent_t)*100);
    int lsdir_cnt = luat_fs_lsdir("/", fs_dirent, 0, 100);
    if (lsdir_cnt > 0) {
        for (int i = 0; i < lsdir_cnt; i++) { // was size_t
            switch ((fs_dirent+i)->d_type) {
                case 0: // a file, not directory
                    if(modcellular_endswith((fs_dirent + i)->d_name, suffix)) {
                        mp_printf(&mp_plat_print, "Remove %s ... ", (fs_dirent + i)->d_name);
                        int res = luat_fs_remove((fs_dirent+i)->d_name);
                        if(res == 0) mp_printf(&mp_plat_print, "success\n");
                        else mp_printf(&mp_plat_print, "failed\n");
                    }
                    break;
                default: 
                break;
            }
        }        
    }
    m_del(uint8_t, fs_dirent, sizeof(luat_fs_dirent_t) * 100);
    fs_dirent = NULL;
}

int modcellular_new_settings(char *file, const char* sname, const char* ssub) {
    int result = 0;
    if(file != NULL && luat_fs_fexist(file)) {
        mp_printf(&mp_plat_print, "Open %s ... ", file);
        FILE* fd = luat_fs_fopen(file, "r");
        if(fd != NULL) {
            mp_printf(&mp_plat_print, "success\n");
            uint16_t buff_len = 255;
            char * buff = m_new(char, buff_len);
            memset(buff, 0, buff_len);
            int res = luat_fs_fread((uint8_t*)buff, buff_len, 1, fd);
            luat_fs_fclose(fd);
            if(res > 0 && res < (int)sizeof(buff) - 1) {
                cJSON *json = cJSON_Parse(buff);
                if(json != NULL) {
                    if(cJSON_HasObjectItem(json, "sname") && cJSON_HasObjectItem(json, "ssub") &&
                       sname != NULL && ssub != NULL && strlen(sname) <= 20 && strlen(ssub) <= 20) {
                        cJSON_ReplaceItemInObjectCaseSensitive(json, "sname", cJSON_CreateString(sname));
                        cJSON_ReplaceItemInObjectCaseSensitive(json, "ssub", cJSON_CreateString(ssub));
                        char *mess = cJSON_PrintUnformatted(json);
                        if(mess != NULL) {
                            mp_printf(&mp_plat_print, "Write to %s -> %s ... ", file, mess);
                            if(luat_fs_remove(file) == 0) {
                                fd = luat_fs_fopen(file, "w");
                                if(fd != NULL) {
                                    res = luat_fs_fwrite((uint8_t*)mess, strlen(mess), 1, fd);
                                    if(res == (int)strlen(mess)) {
                                        mp_printf(&mp_plat_print, "success\n");
                                        result = 1;
                                    }
                                    else mp_printf(&mp_plat_print, "failed\n");
                                    luat_fs_fclose(fd);
                                    luat_rtos_task_sleep(1000);
                                }
                            }
                            free(mess);
                        }
                    } else mp_printf(&mp_plat_print, "sname or ssub not found or too long\n");
                    cJSON_Delete(json);
                }
            }
            m_del(char, buff, buff_len);
        } else mp_printf(&mp_plat_print, "failed\n");
    }
    return result;
}


void modcellular_sms_process(sms_obj_t *sms) {

    const uint8_t* content = (const uint8_t*)mp_obj_str_get_str(sms->message);

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
      char snum  [52];
      memset(sname, 0, sizeof(sname));
      memset(snum, 0, sizeof(snum));
      int len = strlen((char*)content);
      char * last = (char*)content + len;
      char * firstSpace = strstr((char*)content, " ");
      if(firstSpace != NULL && ((int)(firstSpace + 2 - (char*)content)) < len) {
          char * secondSpace = strstr(firstSpace + 2, " ");
          if(secondSpace != NULL && ((int)(secondSpace + 2 - (char*)content)) < len) {
              if(firstSpace < secondSpace && ((int)(secondSpace - firstSpace)) > 0 && ((int)(secondSpace - firstSpace)) <= 50 &&
                 secondSpace < last && ((int)(last - secondSpace)) > 0 && ((int)(last - secondSpace)) <= 50) {
                  strncpy(sname, firstSpace + 1, ((int)(secondSpace - firstSpace)) - 1);
                  strncpy(snum, secondSpace + 1, ((int)(last - secondSpace)));
                  mp_printf(&mp_plat_print, "Set device sname = '%s' snum = '%s'\n", sname, snum);
                  if(modcellular_new_settings("settings.txt", sname, snum) == 1) to_reset = true;
              } else mp_printf(&mp_plat_print, "Incorrect format (too long)\n");
          } else mp_printf(&mp_plat_print, "Incorrect format (SS)\n");
      } else mp_printf(&mp_plat_print, "Incorrect format (FS)\n");
    }
    if(to_reset) {
        const char* message = "Done";
        mp_printf(&mp_plat_print, "Send '%s' message to %s...\n", message, mp_obj_str_get_str(sms->phone_number));
        //SMS_SendMessage((char*)phone, unicode, unicodeLen, SIM0);
        //WAIT_UNTIL(sms_send_flag, TIMEOUT_SMS_SEND, 100, mp_warning(NULL, "The module will continue attempts sending SMS"));

        // restart module
        mp_printf(&mp_plat_print, "Restarting module...\n");
        // luat_pm_reboot();
        // while(1) luat_rtos_task_sleep(1000);
    }
}


mp_obj_t modcellular_sms_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_phone_number, ARG_message, ARG_pn_type, ARG_index, ARG_type };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_phone_number, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_message, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_pn_type, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, 
        { MP_QSTR_index, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, 
        { MP_QSTR_type, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, 
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    sms_obj_t *self = m_new_obj(sms_obj_t);
    self->base.type = type;
    self->phone_number = args[ARG_phone_number].u_obj;
    self->message = args[ARG_message].u_obj;
    self->pn_type = args[ARG_pn_type].u_int;
    self->index = args[ARG_index].u_int;
    self->type = args[ARG_type].u_int;
    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t modcellular_sms_send(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Sends an SMS messsage.
    // Args:
    //     timeout (int): optional timeout in ms;
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    uint64_t timeout = TIMEOUT_SMS_SEND;
    if (n_args == 2)
        timeout = mp_obj_get_int(args[1]);

    sms_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (self->type!= 0)
        mp_raise_ValueError("A message with non-zero type cannot be sent");

    const char* destination = mp_obj_str_get_str(self->phone_number);
    const char* message = mp_obj_str_get_str(self->message);

    /*uint8_t* unicode = NULL;
    uint32_t unicodeLen;

    if (!SMS_LocalLanguage2Unicode((uint8_t*)message_c, strlen(message_c), CHARSET_UTF_8, &unicode, &unicodeLen))
        mp_raise_ValueError("Failed to convert to Unicode before sending SMS");

    sms_send_flag = 0;
    if (!SMS_SendMessage(destination_c, unicode, unicodeLen, SIM0)) {
        OS_Free(unicode);
        mp_raise_ValueError("Failed to submit SMS message for sending");
    }
    OS_Free(unicode);
    */
    sms_send_flag = 0;
    //int luat_sms_send_msg(uint8_t *p_input, char *p_des, bool is_pdu, int input_pdu_len)
    int res = luat_sms_send_msg((uint8_t*)message, (char*)destination, false, 0);

    if(res == 0) {
        WAIT_UNTIL(sms_send_flag, timeout, 100, mp_warning(NULL, "Failed to send SMS immidiately. The module will continue attempts sending it"));
        if(sms_send_flag == 1) {
            return MP_OBJ_NEW_SMALL_INT(1);
        }
    }
    return MP_OBJ_NEW_SMALL_INT(0);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_sms_send_obj, 1, 2, modcellular_sms_send);


STATIC mp_obj_t modcellular_sms_delete(mp_obj_t self_in) {
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t int_index = mp_obj_get_int(index);
    if(!modcellular_sms_delete_internal(int_index)) {
        modcellular_clear_sms_list();
        mp_raise_ValueError("Failed to delete SMS");
    }
    modcellular_clear_sms_list();
    self->type = 0;
    self->index = 0;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modcellular_sms_delete_obj, &modcellular_sms_delete);

STATIC void modcellular_sms_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
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
        // .ts
        } else if (attr == MP_QSTR_ts) {
            mp_obj_t tuple[7];
            tuple[0] = mp_obj_new_int(self->year);
            tuple[1] = mp_obj_new_int(self->month);
            tuple[2] = mp_obj_new_int(self->day);
            tuple[3] = mp_obj_new_int(self->hour);
            tuple[4] = mp_obj_new_int(self->minute);
            tuple[5] = mp_obj_new_int(self->second);
            tuple[6] = mp_obj_new_int(self->tz * ((self->tzSign) ? 1 : -1));
            dest[0] = mp_obj_new_tuple(7, tuple);
        // .index
        } else if (attr == MP_QSTR_index) {
            dest[0] = mp_obj_new_int(self->index);
        // .type
        } else if (attr == MP_QSTR_type) {
            dest[0] = mp_obj_new_int(self->type);
        // .is_inbox
        } else if (attr == MP_QSTR_is_inbox) {
            dest[0] = mp_obj_new_bool(self->type & (CMI_SMS_STOR_STATUS_REC_READ | CMI_SMS_STOR_STATUS_REC_UNREAD));
        // .is_read
        } else if (attr == MP_QSTR_is_read) {
            dest[0] = mp_obj_new_bool(self->type & CMI_SMS_STOR_STATUS_REC_READ);
        // .is_unread
        } else if (attr == MP_QSTR_is_unread) {
            dest[0] = mp_obj_new_bool(self->type & CMI_SMS_STOR_STATUS_REC_UNREAD);
        // .is_unsent
        } else if (attr == MP_QSTR_is_unsent) {
            dest[0] = mp_obj_new_bool(self->type & CMI_SMS_STOR_STATUS_STO_UNSENT);
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
    sms_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char *type_string = "UNKNOWN";
    switch(self->type) {
        case CMI_SMS_STOR_STATUS_REC_UNREAD: type_string = "UNREAD";break;
        case CMI_SMS_STOR_STATUS_REC_READ:   type_string = "READ";  break;
        case CMI_SMS_STOR_STATUS_STO_UNSENT: type_string = "UNSENT";break;
        case CMI_SMS_STOR_STATUS_STO_SENT:   type_string = "SENT";  break;
        case CMI_SMS_STOR_STATUS_ALL:        type_string = "ALL";break; // should not print ever
        case CMI_SMS_STOR_STATUS_END:        type_string = "END";break; // should not print ever
    }
    mp_printf(print, "SMS(\"%s\", \"%s\", ts:%d/%d/%d %d:%d:%d GMT%s%d, pn_type=%d, index=%d, type=%d(%s), dcs=0x%02x, parts=%d/%d/%d/%d, ports=%d/%d)\n",
            mp_obj_str_get_str(self->phone_number),
            mp_obj_str_get_str(self->message),
            self->day, self->month, self->year, self->hour, self->minute, self->second, (self->tzSign ? "+" : "-"), (int8_t)(self->tz / 4),
            self->pn_type, self->index, self->type, type_string, self->dcs,
            self->combined_sms, self->combined_sms_part_number, self->combined_sms_parts, self->combined_sms_reference,
            self->source_port, self->destination_port
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



uint8_t u_decodeMask [] = { 128, 192, 224, 240, 248, 252, 254 };
#define ASCIIENCODING 0x0
#define UTF7ENCODING 0x7
#define UTF8ENCODING 0x8

void sms_debug_print(char *type, uint8_t *data, uint16_t data_len) {
    char *mess = m_new(char, data_len * 3 + 1);
    if(mess != NULL) {
        memset(mess, 0, data_len * 3 + 1);
        for(int i = 0; i < data_len; i++) {
            sprintf(mess + i * 3 , "%02X ", *(uint8_t *)(data + i));
        }
        LUAT_DEBUG_PRINT("decode %s: %s (%d)", type, mess, data_len);
        m_del(char, mess, data_len * 3 + 1);
    }
}

uint8_t * util_unpack_bytes(uint8_t* packed_bytes, uint16_t packed_bytes_len, uint8_t* result_bytes, uint16_t* result_bytes_len) {
    uint16_t shifted_bytes_len = (packed_bytes_len * 8) / 7;
    uint8_t shifted_bytes[200];
    if(shifted_bytes_len >= sizeof(shifted_bytes)) return packed_bytes;

    int shift_offset = 0;
    int shift_index = 0;
    int i;

    // Shift the packed bytes to the left according to the offset (position of the byte)
    for(i = 0; i < packed_bytes_len; i++) {
        uint8_t b = packed_bytes[i];
        if (shift_offset == 7) {
            shifted_bytes[shift_index] = 0;
            shift_offset = 0;
            shift_index++;
        }

        shifted_bytes[shift_index] = (uint8_t)((b << shift_offset) & 0x7F);

        shift_offset++;
        shift_index++;
    }

    int move_offset = 0;
    int move_index = 0;
    int unpack_index = 1;
    uint8_t *unpacked_bytes = m_new(uint8_t, shifted_bytes_len);
    memset(unpacked_bytes, 0, shifted_bytes_len);

    if (shifted_bytes_len > 0) {
        unpacked_bytes[unpack_index - 1] = shifted_bytes[unpack_index - 1];
    }

    for(i= 0; i < packed_bytes_len; i++) {
    uint8_t b = packed_bytes[i];
        if (unpack_index != shifted_bytes_len) {
            if (move_offset == 7) {
                move_offset = 0;
                unpack_index++;
                unpacked_bytes[unpack_index - 1] = shifted_bytes[unpack_index - 1];
            }

            if (unpack_index != shifted_bytes_len) {
                uint8_t extracted_bits_byte = (packed_bytes[move_index] & u_decodeMask[move_offset]); //  uint16_t
                extracted_bits_byte = (extracted_bits_byte >> (7 - move_offset));
                uint8_t moved_bits_byte = (extracted_bits_byte | shifted_bytes[unpack_index]);         //  uint16_t
                unpacked_bytes[unpack_index] = (uint8_t)moved_bits_byte;                
                move_offset++;
                unpack_index++;
                move_index++;
            }
        }
    }
    if(*result_bytes_len >= shifted_bytes_len) {
       *result_bytes_len = (unpacked_bytes[shifted_bytes_len - 1] == 0) ? shifted_bytes_len - 1 : shifted_bytes_len;
       memcpy(result_bytes, unpacked_bytes, *result_bytes_len);
       m_del(uint8_t, unpacked_bytes, shifted_bytes_len);
       return result_bytes;
    } else {
       m_del(uint8_t, unpacked_bytes, shifted_bytes_len);
       return packed_bytes;
    }
}

void util_decode_7Bit_ASCII(uint8_t* packed_bytes, uint16_t packed_bytes_len, uint8_t* unpacked_bytes, uint16_t *unpacked_bytes_len) {    
    // sms_debug_print("decode_7Bit_ASCII packed    ", packed_bytes, packed_bytes_len);
    if (packed_bytes != NULL) {
        if(util_unpack_bytes(packed_bytes, packed_bytes_len, unpacked_bytes, unpacked_bytes_len)) {
            sms_debug_print("decode_7Bit_ASCII unpacked  ", unpacked_bytes, *unpacked_bytes_len);
        }
    } 
}

void util_decode_UCS(uint8_t* b, uint16_t len, uint8_t *pOutData, uint16_t *pSmsLength) {
    // sms_debug_print("util_decode_UCS", b, len);
    luat_iconv_t cd = luat_iconv_open("utf8", "ucs2be");
    size_t in_bytes_left = len;
    int res = luat_iconv_convert(cd, (char **)&b, &in_bytes_left, (char **)&pOutData, (size_t *)pSmsLength);    
    luat_iconv_close(cd);
}

void util_decode_UTF8(uint8_t* b, uint16_t len, uint8_t *pOutData, uint16_t *pSmsLength){
    // sms_debug_print("util_decode_UTF8", b, len);
    *pSmsLength = MIN(len, *pSmsLength);
    memcpy(pOutData, b, *pSmsLength);
}

void util_decode_mms(uint8_t* b, uint16_t len, uint8_t *pOutData, uint16_t *pSmsLength) {
    //sms_debug_print("util_decode_mms", b, len);
    strcpy((char*)pOutData, "decode MMS not implemented");
    *pSmsLength = strlen((char*)pOutData);
}

void util_decode_vcard(uint8_t* b, uint16_t len, uint8_t *pOutData, uint16_t *pSmsLength, uint8_t encoding) {
    sms_debug_print("util_decode_vcard", b, len);

    uint8_t *mess = m_new(uint8_t, LUAT_SMS_MAX_TXT_SIZE + 1);
    uint16_t mess_len = LUAT_SMS_MAX_TXT_SIZE + 1;
    if(mess != NULL) {
        uint8_t decoded = 0;
        memset(mess, 0, LUAT_SMS_MAX_TXT_SIZE + 1);
        if(encoding == UTF7ENCODING) {
            util_decode_7Bit_ASCII(b, len, mess, &mess_len);
            decoded = 1;
        } else if(encoding == UTF8ENCODING) {
            util_decode_7Bit_ASCII(b, len, mess, &mess_len);
            decoded = 1;
        };

        if(decoded) { 
            const char *vcard_begin = "BEGIN:VCARD";
            uint8_t vcard_begin_len = strlen(vcard_begin);
            const char *vcard_name = "N:";
            uint8_t vcard_name_len = strlen(vcard_name);
            const char *vcard_tel = "TEL:";
            uint8_t vcard_tel_len = strlen(vcard_tel);

            for(uint8_t i = 0; i < mess_len; i++) {
                if(i + vcard_begin_len < mess_len && strncmp((char*)(mess + i), vcard_begin, vcard_begin_len) == 0)  {
                    for(uint8_t j = i + vcard_begin_len + 1; j < mess_len; j++, i++) {
                        if(j + vcard_name_len < mess_len && strncmp((char*)(mess + j), vcard_name, vcard_name_len) == 0)  {
                            uint8_t crlf = j + vcard_name_len;
                            while(crlf < mess_len && mess[crlf] != '\r' && mess[crlf] != '\n') crlf++;
                            strcat((char*)pOutData, "Name: ");strncat((char*)pOutData, (char*)(mess + j + vcard_name_len), crlf);
                        }
                        if(j + vcard_tel_len < mess_len && strncmp((char*)(mess + j), vcard_tel, vcard_tel_len) == 0)  {
                            uint8_t crlf = j + vcard_tel_len;
                            while(crlf < mess_len && mess[crlf] != '\r' && mess[crlf] != '\n') crlf++;
                            strcat((char*)pOutData, "Tel: ");strncat((char*)pOutData, (char*)(mess + j + vcard_tel_len), crlf);
                        }
                    }
                }
            }
            *pSmsLength = strlen((char*)pOutData);
            m_del(uint8_t, mess, LUAT_SMS_MAX_TXT_SIZE + 1);
            return;
        }
        m_del(uint8_t, mess, LUAT_SMS_MAX_TXT_SIZE + 1);
    }
    strcpy((char*)pOutData, "decode VCARD failed");
    *pSmsLength = strlen((char*)pOutData);
}

void util_decode_stk(sms_obj_t *self, uint8_t* b, uint16_t len, uint8_t *pOutData, uint16_t *pSmsLength) {
    sms_debug_print("util_decode_stk", b, len);
    char *mess = m_new(char, len * 3 + 1);
    if(mess != NULL) {
        memset(b, 0, len * 3 + 1);
        for(int i = 0; i < len; i++) sprintf((char*)(b + i * 3) , "%02X ", *(uint8_t *)(b + i));
        uint8_t *dst = pOutData;
        sprintf((char*)pOutData, "%s, dcs=%02x, ports=%d/%d, data=%s", (self->usim_toolkit) ? "STK" : "U", self->dcs, self->source_port, self->destination_port, mess);
        *pSmsLength = strlen((char*)pOutData);
        m_del(char, mess, len * 3 + 1);
    }
}

void util_decode_wap(sms_obj_t *self, uint8_t* b, uint16_t len, uint8_t *pOutData, uint16_t *pSmsLength) {
    sms_debug_print("util_decode_wap", b, len);

    u_int16_t c = 0;
    uint8_t transaction_id = b[c++];
    uint8_t pdu_type = b[c++];
    if (pdu_type == 0x06) {  // PUSH
        u_int8_t pdu_hlen = b[c++];
        if (pdu_hlen == 1) {
            
            //uint8_t content_type = b[c++]; 
            c++;
            uint8_t version = b[c++];
            byte public_idenifier = b[c++];
            byte character_set = b[c++];
            //Encoding enc = Encoding.Default;
            //if (character_set == 0x6A) enc = Encoding.UTF8;
            uint8_t string_table = b[c++];
            uint16_t mess_len = 0;
            uint16_t mess_offset = 0;
            uint16_t out_len;

            if (string_table == 0) {
                while (c < len) {
                    uint8_t header = (byte)(b[c++] & 0x0F);
                    switch (header) {
                        case 0x01: strcat((char*)pOutData, "</>"); break;
                        case 0x05: strcat((char*)pOutData, "<si>"); break;
                        case 0x06: strcat((char*)pOutData, "<indication>"); break;
                        case 0x0C: strcat((char*)pOutData, "href=http://"); break;
                        case 0x03:
                            mess_offset = c;
                            while (c < len && b[c] != 0) { mess_len++; c++; }
                            util_decode_UTF8((uint8_t *)(b + mess_offset), mess_len + 1, pOutData + strlen((char*)pOutData), &out_len);
                            c++;
                            break;
                        case 0x07: strcat((char*)pOutData, "action='signal-medium'"); 
                            break;
                        default: 
                            break;
                    }
                }
            } else strcpy((char*)pOutData, "decode WAP failed");
        } else strcpy((char*)pOutData, "decode WAP failed");
        *pSmsLength = strlen((char*)pOutData);
    } else {
        util_decode_stk(self, b, len, pOutData, pSmsLength);
    }
}

void modcellular_sms_decode_user_data(sms_obj_t *self, uint8_t *pUserData, uint16_t pduDataLen, PsilSmsDcsInfo dcs, uint8_t hdrPresent, 
                                          uint8_t *pOutData, uint16_t *pSmsLength) {

    uint16_t c = 0;
    uint8_t mess[160];
    uint8_t mess2[161];
    uint16_t mess2_len = sizeof(mess2);
    uint8_t user_data_len = pUserData[c++];

    self->usim_toolkit = 0;    
    self->combined_sms = 0;
    self->combined_sms_reference = 0;
    self->combined_sms_parts = 0;
    self->combined_sms_part_number = 0;
    self->source_port = 0;
    self->destination_port = 0;

    if (user_data_len > 0) {
        uint16_t user_data_header_len = 0;
        uint16_t message_len = 0;
        if (hdrPresent) {
            // byte array consists of following fields
            // User Header data length
            // Information element identifier
            // Information element data length
            // A reference number (must be the same for all parts of the same larger messages)
            // Parts quantity
            // Parts number
            user_data_header_len = pUserData[c++];
            uint16_t offset = c;
            uint16_t cLen = 0;
            while (c < offset + user_data_header_len && c + 1 < pduDataLen) {
                cLen = pUserData[c + 1];
                switch (pUserData[c]) {
                case 0x00: // Concatenated short messages, 8-bit reference number
                    self->combined_sms = true;
                    if (cLen == 3 && c + cLen <= pduDataLen) {                        
                        self->combined_sms_reference = pUserData[c + 2];
                        self->combined_sms_parts = pUserData[c + 3];
                        self->combined_sms_part_number = pUserData[c + 4];
                    } else {
                        LUAT_DEBUG_PRINT("Incorrect user data header\n");
                        return;
                    }
                    break;
                case 0x01: // Special SMS Message Indication
                    break;
                case 0x02: // Reserved IE N/A
                    break;
                case 0x04: // VCARD
                    if (cLen == 2 && c + cLen <= pduDataLen) {
                        self->destination_port = pUserData[c + 2];
                        self->source_port = pUserData[c + 3];
                    }
                    break;
                case 0x05: // From http://www.openmobilealliance.org/tech/affiliates/wap/wapindex.html
                    // Application port addressing 16bit
                    if (cLen == 4 && c + cLen <= pduDataLen) {
                        self->destination_port = (uint16_t)((uint16_t)(pUserData[c + 2] << 8) + pUserData[c + 3]);
                        self->source_port = (uint16_t)((uint16_t)(pUserData[c + 4] << 8) + pUserData[c + 5]);
                    }
                    break;
                case 0x06: // SMSC Control Parameters
                    break;
                case 0x07: // Created by SMSc
                    break;
                case 0x08: // Concatenated short messages, 16-bit reference number
                    self->combined_sms = true; 
                    if (cLen == 4 && c + cLen <= pduDataLen) {
                        uint16_t temp = (uint16_t)((uint16_t)(pUserData[c + 2] << 8) + pUserData[c + 3]);
                        self->combined_sms_reference = temp;
                        self->combined_sms_parts = pUserData[c + 4];
                        self->combined_sms_part_number = pUserData[c + 5];
                    } else {
                        LUAT_DEBUG_PRINT("Incorrect user data header\n");
                        return;
                    }
                    break;                
                case 0x09: // Wireless Control Message Protocol
                case 0x0A: // Text Formatting
                case 0x0B: // Predefined Sound
                case 0x0C: // User Defined Sound (iMelody max 128 bytes)
                case 0x0D: // Predefined animation (EMS Content)
                case 0x0E: // Large Animation (16*16 times 4 = 32*4 =128 bytes)
                case 0x0F: // Small Animation (8*8 times 4 = 8*4 =32 bytes)
                case 0x10: // Large Picture (32*32 = 128 bytes)
                case 0x11: // Small Picture (16*16 = 32 bytes)
                case 0x12: // Variable Picture
                case 0x13: // User prompt indicator
                case 0x14: // Extended Object
                case 0x15: // Reused Extended Object
                case 0x16: // Compression Control
                case 0x17: // Object Distribution Indicator
                case 0x18: // Standard WVG object
                case 0x19: // Character Size WVG object
                case 0x1A: // Extended Object Data Request Command
                case 0x20: // RFC 5322 E-Mail Header
                case 0x21: // Hyperlink format element
                case 0x22: // Reply Address Element
                case 0x23: // Enhanced Voice Mail Information
                case 0x24: // National Language Single Shift
                case 0x25: // National Language Locking Shift                
                    LUAT_DEBUG_PRINT("TPDU header not implemented - 0x%02x\n", pUserData[c]);
                    break;
                case 0x70: // (U)SIM Toolkit Security Headers
                    self->usim_toolkit = 1;
                    break;
                case 0x71: // (U)SIM Toolkit Security Headers
                    self->usim_toolkit = 1;
                    break;
                default:
                    LUAT_DEBUG_PRINT("Unknown IE TPDU header 0x%02x\n", pUserData[c]);
                    return;
                }
                c += cLen + 2;
            }
            c = offset + user_data_header_len;
            message_len = (uint8_t)(user_data_len - user_data_header_len - 1);
        } else message_len = user_data_len;

        if (dcs.dcs == 0xF1) message_len--;

        if ((dcs.dcs & 0x0C) == 0x00) {
            if ((message_len % 8) == 0 || (user_data_len % 8) == 0) { 
                message_len = (uint8_t)(message_len * 7 / 8);
            } else {
                message_len = (uint8_t)(message_len * 7 / 8 + 1); // + 1
                if (message_len == 0)
                    message_len = 1;
            }
        }
        
        if (message_len < sizeof(mess)) {
            uint8_t lshift = 0;
            //uint8_t lshift2 = 0;
            if ((dcs.dcs & 0x0C) == 0x00 && hdrPresent) {
                // shifting for Decoding 7 bit ACSII
                if (user_data_header_len == 0x03) lshift = 5;
                else if (user_data_header_len == 0x04) lshift = 6;
                else if (user_data_header_len == 0x05) lshift = 7;
                else if (user_data_header_len == 0x06) lshift = 1;
                else if (user_data_header_len == 0x08) lshift = 3;
                else if (user_data_header_len == 0x0B) lshift = 6;
                else if (user_data_header_len == 0x2A) lshift = 2;
                else lshift = (uint8_t)((((user_data_header_len + 2) << 3)) % 7);
                //lshift2 = (uint8_t)((((user_data_header_len + 2) << 3)) % 7);
                uint8_t rshift = (uint8_t)(8 - lshift);
                memcpy(mess2, pUserData + c, MIN(message_len, pduDataLen - c));
                for (int i = 0; i < message_len; i++) {
                    mess[i] = (uint8_t)((uint8_t)(mess2[i + 1] << lshift) | (uint8_t)(mess2[i] >> rshift));
                }
            } else {
                memcpy(mess, pUserData + c, MIN(message_len, pduDataLen - c));
            }
        }
        
        LUAT_DEBUG_PRINT("dcs = %02x, source_port = %d, destination_port = %d", dcs.dcs, self->source_port, self->destination_port);
        if ((dcs.dcs & 0x80) == 0) {
            switch (dcs.dcs & 0x0C) {
                case 0x00:
                    if (self->source_port == 0x00 &&
                        self->destination_port == 0xE2) {
                        util_decode_vcard(mess, message_len, pOutData, pSmsLength, UTF7ENCODING);
                    } else {
                        util_decode_7Bit_ASCII(mess, message_len, pOutData, pSmsLength);
                    }
                    break;
                case 0x04:
                    if (self->source_port == 0x23F0 &&
                        self->destination_port == 0x0b84) {
                        // MMS
                        util_decode_mms(mess, message_len, pOutData, pSmsLength);
                    } else if (self->source_port == 0x00 && self->destination_port == 0xE2) {
                        util_decode_vcard(mess, message_len, pOutData, pSmsLength, UTF8ENCODING);
                    } else {
                        util_decode_UTF8(mess, message_len, pOutData, pSmsLength);
                    }
                    break;
                case 0x08:
                    util_decode_UCS(mess, message_len, pOutData, pSmsLength);
                    break;
                case 0x0C:
                    util_decode_7Bit_ASCII(mess, message_len, pOutData, pSmsLength);
                    break;
                default:
                    LUAT_DEBUG_PRINT("Unknown data coding scheme 0x%02x\n", dcs.dcs);
                    return;
            }
        } else {
            switch (dcs.dcs & 0x0C) {
                case 0x00:
                    util_decode_7Bit_ASCII(mess, message_len, pOutData, pSmsLength);
                    break;
                case 0x04:
                case 0x0C:
                    if (self->source_port == 0x23F0 &&
                        self->destination_port == 0x0b84) {
                        util_decode_wap(self, mess, message_len, pOutData, pSmsLength);                        
                    } else {                        
                        util_decode_stk(self, mess, message_len, pOutData, pSmsLength);
                    }
                    break;
                case 0x08:
                    util_decode_7Bit_ASCII(mess, message_len, pOutData, pSmsLength);
                    break;
                default:
                    LUAT_DEBUG_PRINT("Unknown data coding scheme 0x%02x\n", dcs.dcs);
                    return;
            }
        }
        c += message_len;
    }
}


void modcellular_sms_decode_pdu(sms_obj_t *self, CmiSmsPdu *smsPduData, uint8_t *phoneNumberType, char *phone, uint8_t *dcs, 
                                PsilSmsTimeStampInfo *tem_time, uint8_t *message, uint16_t *message_len) {
    uint8_t start_offset = 0;
    bool hdr_present = false;
    UdhIe hIe = {0};
    uint8_t fix_year = 0;

    if ((smsPduData->pduData[start_offset]) & (0x40)) hdr_present = true;
    start_offset++;
    //Get the sender's mobile phone number
    smsPduDecodeAddress(smsPduData->pduData, &start_offset, phoneNumberType, (UINT8*)phone, (LUAT_MSG_MAX_ADDR_LEN + 1));
    // LUAT_DEBUG_PRINT("phone: %s", phone);
    start_offset++;
    PsilSmsDcsInfo msg_dcs_info;
    smsPduDecodeDcs(smsPduData->pduData, &start_offset, &msg_dcs_info);
    *dcs = msg_dcs_info.dcs;
    smsPduDecodeTimeStamp(smsPduData->pduData, &start_offset, tem_time); 
    // smsPduDecodeUserData corrupts tem_time->year. Do not know why
    fix_year = tem_time->year;
    modcellular_sms_decode_user_data(self, (smsPduData->pduData + start_offset),
                        (smsPduData->pduLength - start_offset - 1),
                        msg_dcs_info, hdr_present, message, message_len);
    // LUAT_DEBUG_PRINT("message (%d): %s", *message_len, message);
    tem_time->year = fix_year;
}

/* typedef struct CmiSmsListSmsMsgRecCnf_Tag {
    CmiSmsErrorCode         errorCode;
    UINT8                   endStatus;          // where or not is the last item 
    BOOL                    scAddrPresent;      // SC Address info present or not 
    UINT8                   index;              // the index of the SMS record in SIM 
    UINT8                   smsStatus;          // SMS record status. CmiSmsRecStorStatus 
    UINT8                   smsMsgType;         // SMS message type, defined type CmiSmsMessageType 
    UINT8                   reserved0;
    CmiSmsAddressInfo       scAddrInfo;         // SC address info 
    CmiSmsPdu               smsPduData;         // SMS PDU contents 
} CmiSmsListSmsMsgRecCnf; */
mp_obj_t modcellular_sms_from_list_record(CmiSmsListSmsMsgRecCnf *record) {

    sms_obj_t *self = m_new_obj_with_finaliser(sms_obj_t);
    self->base.type = &modcellular_sms_type;
    self->index = record->index;
    self->type = (uint8_t)record->smsStatus;

    uint8_t phoneNumberType = 0;
    char *phone = m_new(char, LUAT_MSG_MAX_ADDR_LEN + 1);
    uint8_t dcs = 0;
    PsilSmsTimeStampInfo tem_time = {0};
    uint8_t *message = m_new(u_int8_t, LUAT_SMS_MAX_TXT_SIZE + 1);
    uint16_t message_len = LUAT_SMS_MAX_TXT_SIZE + 1;
    memset(message, 0, LUAT_SMS_MAX_TXT_SIZE + 1);

    if(phone != NULL && message != NULL) {
        modcellular_sms_decode_pdu(self, &(record->smsPduData), &phoneNumberType, phone, &dcs, &tem_time, message, &message_len);
        self->dcs = dcs;        
        self->phone_number = mp_obj_new_str(phone, strlen(phone));
        self->message = mp_obj_new_str((const char*)message, message_len);
    } else {
        self->phone_number = mp_obj_new_str(phone, strlen(phone));
        self->message = mp_obj_new_str((const char*)message, message_len);
    }
    self->pn_type = phoneNumberType;
    self->dcs = dcs;
    self->year = tem_time.year;
    self->month = tem_time.month;
    self->day = tem_time.day;
    self->hour = tem_time.hour;
    self->minute = tem_time.minute;
    self->second = tem_time.second;
    self->tz = tem_time.tz;
    self->tzSign = tem_time.tzSign;

    m_del(char, phone, LUAT_MSG_MAX_ADDR_LEN + 1);
    m_del(uint8_t, message, LUAT_SMS_MAX_TXT_SIZE + 1);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t modcellular_ussd(size_t n_args, const mp_obj_t *args) {
    REQUIRES_NETWORK_REGISTRATION; // checks network_status
    mp_printf(&mp_plat_print, "USSD over IMS not supported and module does not support 2G fallback\n");
    mp_obj_t tuple[2];
    uint8_t dummy[0];
    tuple[0] = mp_obj_new_int(0);
    tuple[1] = mp_obj_new_bytes(dummy, 0); 
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_ussd_obj, 1, 2, modcellular_ussd);

STATIC mp_obj_t modcellular_on_sms(mp_obj_t callable) {
    sms_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modcellular_on_sms_obj, modcellular_on_sms);


STATIC mp_obj_t modcellular_on_ussd(mp_obj_t callable) {
    ussd_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modcellular_on_ussd_obj, modcellular_on_ussd);

STATIC mp_obj_t modcellular_sms_delete_by_index(mp_obj_t index) {
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    mp_int_t int_index = mp_obj_get_int(index);
    if(modcellular_get_sms_list(CMI_SMS_STOR_STATUS_ALL)) {
        for(int i = 0; i < (int)sms_list_buffer->len; i++) {
            sms_obj_t *sms = MP_OBJ_TO_PTR(sms_list_buffer->items[i]);
            if(sms->index == int_index) {
                if(!modcellular_sms_delete_internal(int_index)) {
                    modcellular_clear_sms_list();
                    mp_raise_ValueError("Failed to delete SMS (index not found)");
                }
                mp_hal_delay_ms(500); // wait SIM storage update
                modcellular_clear_sms_list();
                return mp_const_none;
            }
        }
    } 
    modcellular_clear_sms_list();
    mp_raise_ValueError("Failed to delete SMS (index not found)");
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(modcellular_sms_delete_by_index_obj, modcellular_sms_delete_by_index);

STATIC mp_obj_t modcellular_sms_delete_all_read() {
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    if(modcellular_get_sms_list(CMI_SMS_STOR_STATUS_ALL)) {
        for(int i = 0; i < (int)sms_list_buffer->len; i++) {
            sms_obj_t *sms = MP_OBJ_TO_PTR(sms_list_buffer->items[i]);
            if(sms->type == CMI_SMS_STOR_STATUS_REC_READ) {
                if(!modcellular_sms_delete_internal(sms->index)) {
                    modcellular_clear_sms_list();
                    mp_raise_ValueError("Failed to delete SMSs");
                }
            }
        }
        mp_hal_delay_ms(500); // wait SIM storage update
        modcellular_clear_sms_list();
    } else {
        modcellular_clear_sms_list();
        mp_raise_ValueError("Failed to delete SMSs");
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_delete_all_read_obj, modcellular_sms_delete_all_read);


mp_obj_t modcellular_sms_read_all(void) {
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    mp_obj_list_t *result = mp_obj_new_list(0, NULL);
    if(modcellular_get_sms_list(CMI_SMS_STOR_STATUS_REC_UNREAD)) {
        for(int i = 0; i < (int)sms_list_buffer->len; i++) {
            mp_obj_list_append(result, sms_list_buffer->items[i]);
        }       
    }
    modcellular_clear_sms_list(); 
    return (mp_obj_t)result;    
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_read_all_obj, modcellular_sms_read_all);


STATIC mp_obj_t modcellular_sms_list_read(void) {
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    mp_obj_list_t *result = mp_obj_new_list(0, NULL);
    if(modcellular_get_storage_info() && modcellular_get_sms_list(CMI_SMS_STOR_STATUS_REC_READ)) {
        for(int i = 0; i < (int)sms_list_buffer->len; i++) mp_obj_list_append(result, sms_list_buffer->items[i]);        
    }
    modcellular_clear_storage_info();
    modcellular_clear_sms_list(); 
    return (mp_obj_t)result;
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_list_read_obj, modcellular_sms_list_read);


STATIC mp_obj_t modcellular_sms_list(void) {
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    mp_obj_list_t *result = mp_obj_new_list(0, NULL);
    if(modcellular_get_storage_info() && modcellular_get_sms_list(CMI_SMS_STOR_STATUS_ALL)) {
        for(int i = 0; i < (int)sms_list_buffer->len; i++) mp_obj_list_append(result, sms_list_buffer->items[i]);
    }
    modcellular_clear_storage_info();
    modcellular_clear_sms_list(); 
    return (mp_obj_t)result;
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_list_obj, modcellular_sms_list);


STATIC mp_obj_t modcellular_sms_get_storage_size(void) {
    if(modcellular_get_storage_info() && modcellular_get_sms_list(CMI_SMS_STOR_STATUS_ALL)) {
        // count statuses
        int unReadRecords = 0;
        int readRecords = 0;
        int sentRecords = 0;
        int unsentRecords = 0;
        int unknownRecords = 0;
        for(int i = 0; i < (int)sms_list_buffer->len; i++) {
            sms_obj_t *sms = MP_OBJ_TO_PTR(sms_list_buffer->items[i]);
            if(sms->type == CMI_SMS_STOR_STATUS_REC_UNREAD) unReadRecords++;
            if(sms->type == CMI_SMS_STOR_STATUS_REC_READ) readRecords++;
            if(sms->type == CMI_SMS_STOR_STATUS_STO_UNSENT) unsentRecords++;
            if(sms->type == CMI_SMS_STOR_STATUS_STO_SENT) sentRecords++;
        }
        unknownRecords = storage->usedNumOfSim - (unReadRecords + readRecords + unsentRecords + sentRecords);

        mp_obj_t tuple[8] = {
            mp_obj_new_int(storage->usedNumOfSim),
            mp_obj_new_int(storage->totalNumOfSim),
            mp_obj_new_int(unReadRecords),
            mp_obj_new_int(readRecords),
            mp_obj_new_int(sentRecords),
            mp_obj_new_int(unsentRecords),
            mp_obj_new_int(unknownRecords),
            mp_obj_new_int(PSIL_SMS_STOR_MEM_TYPE_SM),
        };
        modcellular_clear_storage_info();
        modcellular_clear_sms_list();
        return mp_obj_new_tuple(8, tuple);
    }
    modcellular_clear_storage_info();
    modcellular_clear_sms_list();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(modcellular_sms_get_storage_size_obj, modcellular_sms_get_storage_size);


