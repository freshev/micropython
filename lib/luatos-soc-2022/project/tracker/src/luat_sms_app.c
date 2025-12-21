#include "luat_debug.h"
#include "luat_rtos.h"
#include "luat_sms.h"
#include "luat_sms.h"
#include "luat_mem.h"
#include "protocol_text.h"
#include "param_ctrl.h"
#define CMD_MAX_LEN 180
#define NUMBER "121xxxx21xx"
extern luat_rtos_task_handle sms_task_handle;
static luat_rtos_task_handle send_msg_task_handle;
static luat_rtos_task_handle recv_msg_task_handle;
static luat_rtos_semaphore_t send_message_semaphore;

void luat_sms_send_pdu_msg(uint8_t *p_input, char *p_des)
{
    size_t phone_len = 0;
    size_t payload_len = 0;
    const char* phone = p_des;
    const char* payload = p_input;
    int ret = 0;
    char phone_buff[32] = {0};
    payload_len = strlen(payload);
	phone_len = strlen(phone);

    if (payload_len == 0) {
        LUAT_DEBUG_PRINT("sms is empty");
        return 0;
    }
    if (payload_len >= 140) {
        LUAT_DEBUG_PRINT("sms is too long %d", payload_len);
        return 0;
    }
    int pdu_mode = 0;
    for (size_t i = 0; i < payload_len; i++)
    {
        if (payload[i] & 0x80) {
            LUAT_DEBUG_PRINT("found non-ASCII char, using PDU mode");
            pdu_mode = 1;
            break;
        }
    }

    
    if (phone_len < 3 || phone_len > 29) {
        LUAT_DEBUG_PRINT("phone is too short or too long!! %d", phone_len);
        return 0;
    }
    // +8613416121234
    if (pdu_mode) { // In PDU mode, the country code must be brought
        if (phone[0] == '+') {
            memcpy(phone_buff, phone + 1, phone_len - 1);
        }
        // 13416121234
        else if (phone[0] != '8' && phone[1] != '6') {
            phone_buff[0] = '8';
            phone_buff[1] = '6';
            memcpy(phone_buff + 2, phone, phone_len);
        }
        else {
            memcpy(phone_buff, phone, phone_len);
        }
    }
    else {
        if (phone[0] == '+') {
            memcpy(phone_buff, phone + 3, phone_len - 3);
        }
        else if (phone[0] == '8' && phone[1] == '6') {
            memcpy(phone_buff, phone+2, phone_len - 2);
        }
        else {
            memcpy(phone_buff, phone, phone_len);
        }
    }
    
    
    phone_len = strlen(phone_buff);
    phone = phone_buff;
    LUAT_DEBUG_PRINT("phone %s", phone);
    if (pdu_mode) {
        char pdu[280 + 100] = {0};
        // First, fill in the PDU header
        strcat(pdu, "00"); // Use the built-in SMS center, which cannot be set temporarily.
        strcat(pdu, "01"); // Only receive information, no retention time is passed
        strcat(pdu, "00"); // TP-MR, always fill in 0
        sprintf_(pdu + strlen(pdu), "%02X", phone_len); // phone number length
        strcat(pdu, "91"); //Destination address format
        //Mobile phone number
        for (size_t i = 0; i < phone_len; i+=2)
        {
            if (i == (phone_len - 1) && phone_len % 2 != 0) {
                pdu[strlen(pdu)] = 'F';
                pdu[strlen(pdu)] = phone[i];
            }
            else {
                pdu[strlen(pdu)] = phone[i+1];
                pdu[strlen(pdu)] = phone[i];
            }
        }
        strcat(pdu, "00"); //The protocol identifier (TP-PID) is an ordinary GSM type, point-to-point mode
        strcat(pdu, "08"); // Encoding format, UCS encoding
        size_t pdu_len_offset = strlen(pdu);
        strcat(pdu, "00"); // This is reserved, and the filled data will be updated to the correct value later.
        uint16_t unicode = 0;
        size_t pdu_userdata_len = 0;
        for (size_t i = 0; i < payload_len; i++)
        {
            // First, is it a single byte?
            if (payload[i] & 0x80) {
                // non-ASCII encoding
                if (payload[i] && 0xE0) { // 1110xxxx 10xxxxxx 10xxxxxx
                    unicode = ((payload[i] & 0x0F) << 12) + ((payload[i+1] & 0x3F) << 6) + (payload[i+2] & 0x3F);
                    //LUAT_DEBUG_PRINT("unicode %04X %02X%02X%02X", unicode, payload[i], payload[i+1], payload[i+2]);
                    sprintf_(pdu + strlen(pdu), "%02X%02X", (unicode >> 8) & 0xFF, unicode & 0xFF);
                    i+=2;
                    pdu_userdata_len += 2;
                    continue;
                }
                if (payload[i] & 0xC0) { // 110xxxxx 10xxxxxx
                    unicode = ((payload[i] & 0x1F) << 6) + (payload[i+1] & 0x3F);
                    //LUAT_DEBUG_PRINT("unicode %04X %02X%02X", unicode, payload[i], payload[i+1]);
                    sprintf_(pdu + strlen(pdu), "%02X%02X", (unicode >> 8) & 0xFF, unicode & 0xFF);
                    i++;
                    pdu_userdata_len += 2;
                    continue;
                }
                LUAT_DEBUG_PRINT("bad UTF8 string");
                break;
            }
            // Single ASCII character, but needs to be extended to 2 characters
            else {
                // ASCII encoding
                strcat(pdu, "00");
                sprintf_(pdu + strlen(pdu), "%02X", payload[i]);
                pdu_userdata_len += 2;
                continue;
            }
        }
        // Correct pdu length
        char tmp[3] = {0};
        sprintf_(tmp, "%02X", pdu_userdata_len);
        memcpy(pdu + pdu_len_offset, tmp, 2);

        //Print PDU data, for debugging
        LUAT_DEBUG_PRINT("PDU %s", pdu);
        payload = pdu;
        payload_len = strlen(pdu);
        phone = "";
        luat_sms_send_msg(pdu, "", 1, payload_len);
    }
}

void app_sms_send_msg(uint8_t *p_input, char *p_des)
{
    bool input_has_chinese = false;
    char* judgeChinese = (char*)p_input;
    for (int i = 0; i < strlen(judgeChinese); i++)
    {
        if (*(judgeChinese+i) & 0x80)
        {
            LUAT_DEBUG_PRINT("[DIO]The input is Chinese");
            input_has_chinese = true;
            break;
        }
    }

    if (!input_has_chinese)
    {
        LUAT_DEBUG_PRINT("[DIO]The input is English");
        luat_sms_send_msg(p_input, p_des, false, 0);
    }
    else
    {
        LUAT_DEBUG_PRINT("[DIO]The input is Chinese");
        luat_sms_send_pdu_msg(p_input, p_des);
    }
}

static void msg_send_task_proc(void *arg)
{
	int ret = -1;
    protocol_text_cmd message_id;
	uint8_t *data = NULL;
	while(1)
	{
		if(luat_rtos_message_recv(send_msg_task_handle, (uint32_t *)&message_id, (void **)&data, LUAT_WAIT_FOREVER) == 0)
		{
            app_sms_send_msg(data, NUMBER);
            luat_rtos_semaphore_take(send_message_semaphore, LUAT_WAIT_FOREVER);
            luat_rtos_task_sleep(5000);
			LUAT_MEM_FREE(data);
			data = NULL;
        }
    }
    luat_rtos_task_delete(send_msg_task_handle);
}

static void luat_sms_recv_cb(uint8_t event,void *param)
{
    LUAT_SMS_RECV_MSG_T *sms_data = NULL;
    sms_data = (LUAT_SMS_RECV_MSG_T *)malloc(sizeof(LUAT_SMS_RECV_MSG_T));
    memset(sms_data, 0x00, sizeof(LUAT_SMS_RECV_MSG_T));
    memcpy(sms_data, (LUAT_SMS_RECV_MSG_T *)param, sizeof(LUAT_SMS_RECV_MSG_T));
    int ret = luat_rtos_message_send(recv_msg_task_handle, 0, sms_data);
	if(ret != 0)
	{
		LUAT_MEM_FREE(sms_data);
		sms_data = NULL;
	}
}



static void luat_sms_send_cb(int ret)
{
	LUAT_DEBUG_PRINT("send ret:[%d]", ret);
    luat_rtos_semaphore_release(send_message_semaphore);
}

static void msg_recv_task_proc(void *arg)
{
	int ret = -1;
    uint32_t message_id;
	LUAT_SMS_RECV_MSG_T *data = NULL;

	while(1)
	{
		if(luat_rtos_message_recv(recv_msg_task_handle, &message_id, (void **)&data, LUAT_WAIT_FOREVER) == 0)
		{
	        LUAT_DEBUG_PRINT("Dcs:[%d]", data->dcs_info.alpha_bet);
	        LUAT_DEBUG_PRINT("Time:[\"%02d/%02d/%02d,%02d:%02d:%02d %c%02d\"]", data->time.year, data->time.month, data->time.day, data->time.hour, data->time.minute, data->time.second,data->time.tz_sign, data->time.tz);
	        LUAT_DEBUG_PRINT("Phone:[%s]", data->phone_address);
	        LUAT_DEBUG_PRINT("ScAddr:[%s]", data->sc_address);
	        LUAT_DEBUG_PRINT("PDU len:[%d]", data->sms_length);
	        LUAT_DEBUG_PRINT("PDU: [%s]", data->sms_buffer);
            uint8_t* pOut = NULL;
            pOut = luat_heap_malloc(CMD_MAX_LEN + 1);
            if (NULL == pOut)
            {
                LUAT_DEBUG_PRINT("luat_sms_recv_cb assert(MemoryAlloc(%d)) failed.", CMD_MAX_LEN + 1);
                return;
            }
            memset(pOut, 0x00, CMD_MAX_LEN + 1);
            //Get the default echo language
            uint8_t language = 1;
            config_service_get(CFG_LANG, TYPE_BYTE, &language, sizeof(language));
            protocol_text_type which_language = ((language == 1) ? CHINESE : ENGLISH);
            protocol_text_cmd cmd = protocol_text_receive_data(data->sms_buffer, data->sms_length, pOut, which_language);
            if (cmd != PROTOCOL_MAX)
            {
                LUAT_DEBUG_PRINT("[DIO] [%s]", data->sms_buffer);
                LUAT_DEBUG_PRINT("[DIO pOut] [%s] [%d]", pOut, strlen(pOut));
	            int ret = luat_rtos_message_send(send_msg_task_handle, cmd, pOut);
	            if(ret != 0)
	            {
	            	LUAT_MEM_FREE(pOut);
	            	pOut = NULL;
	            }
            }
            else
            {
                LUAT_MEM_FREE(pOut);
	            pOut = NULL;
            }
            
			LUAT_MEM_FREE(data);
			data = NULL;
        }
    }
    luat_rtos_task_delete(recv_msg_task_handle);
}

void luat_sms_task_init(void)
{
    luat_rtos_semaphore_create(send_message_semaphore, 1);
	luat_sms_init();
    luat_sms_recv_msg_register_handler(luat_sms_recv_cb);
    luat_sms_send_msg_register_handler(luat_sms_send_cb);
    luat_rtos_task_create(&send_msg_task_handle, 4096, 30, "msg_send", msg_send_task_proc, NULL, 50);
    luat_rtos_task_create(&recv_msg_task_handle, 4096, 30, "msg_recv", msg_recv_task_proc, NULL, 50);
}
