/*@Modules sms
@summary SMS
@version 1.0
@date 2022.12.08
@demo sms
@tag LUAT_USE_SMS
@usage
-- Note that Air780E/Air600E/Air780EG/Air780EG do not support text messages from telecommunications cards!!
-- Meaning, when the above Modules is paired with a telecommunications SIM card, text messages cannot be sent from the Modules, and text messages cannot be received on the Modules.
-- If it is a China Unicom card or China Mobile card, you can receive text messages, but only a real-name card can send text messages.*/

#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_mem.h"
#include "luat_mobile.h"
#include "luat_timer.h"

void luat_str_fromhex(char* str, size_t len, char* buff) ;

#ifndef bool
#define bool uint8_t
#endif

#include "luat_sms.h"

#define LUAT_LOG_TAG "sms"
#include "luat_log.h"
static int lua_sms_ref = 0;
static int lua_sms_recv_long = 1;

typedef struct long_sms
{
    uint8_t refNum;
    uint8_t maxNum;
    uint8_t seqNum;
    char buff[1];
}long_sms_t;

#define LONG_SMS_CMAX (128)
static long_sms_t* lngbuffs[LONG_SMS_CMAX];
// static char* longsms = NULL;
// static int longsms_refNum = -1;


static void ucs2char(char* source, size_t size, char* dst2, size_t* outlen) {
    char buff[size + 2];
    memset(buff, 0, size + 2);
    luat_str_fromhex(source, size, buff);
    //LLOGD("sms %s", source);
    uint16_t* tmp = (uint16_t*)buff;
    char* dst = dst2;
    // size_t tmplen = origin_len / 2;
    // size_t dstoff = 0;
    uint16_t unicode = 0;
    size_t dstlen = 0;
    while (1) {
        unicode = *tmp ++;
        unicode = ((unicode >> 8) & 0xFF) + ((unicode & 0xFF) << 8);
        //LLOGD("unicode %04X", unicode);
        if (unicode == 0)
            break; // terminated
        if (unicode <= 0x0000007F) {
            dst[dstlen++] = (unicode & 0x7F);
            continue;
        }
        if (unicode <= 0x000007FF) {
            dst[dstlen++]	= ((unicode >> 6) & 0x1F) | 0xC0;
		    dst[dstlen++] 	= (unicode & 0x3F) | 0x80;
            continue;
        }
        if (unicode <= 0x0000FFFF) {
            dst[dstlen++]	= ((unicode >> 12) & 0x0F) | 0xE0;
		    dst[dstlen++]	= ((unicode >>  6) & 0x3F) | 0x80;
		    dst[dstlen++]	= (unicode & 0x3F) | 0x80;
            //LLOGD("why? %02X %02X %02X", ((unicode >> 12) & 0x0F) | 0xE0, ((unicode >>  6) & 0x3F) | 0x80, (unicode & 0x3F) | 0x80);
            continue;
        }
        break;
    }
    *outlen = dstlen;
    //LLOGD("ucs2char %d", dstlen);
}

static void push_sms_args(lua_State* L, LUAT_SMS_RECV_MSG_T* sms, char* dst, size_t dstlen) {
    char phone[strlen(sms->phone_address) * 3 + 1];
    memset(phone, 0, strlen(sms->phone_address) * 3 + 1);
    size_t outlen = 0;
    memcpy(phone, sms->phone_address, strlen(sms->phone_address));
    if (strlen(phone) > 4 && phone[0] == '0' && phone[1] == '0' && strlen(phone) % 2 == 0) {
        // It seems to be ucs encoded
        ucs2char(sms->phone_address, strlen(sms->phone_address), phone, &outlen);
        phone[outlen] = 0x00;
    }
    lua_pushstring(L, phone);


    if (dst == NULL) {
        luaL_Buffer buff;
        luaL_buffinit(L, &buff);
        for (size_t j = 0; j < sms->maxNum; j++)
        {
            for (size_t i = 0; i < LONG_SMS_CMAX; i++)
            {
                if (lngbuffs[i] && lngbuffs[i]->refNum == dstlen && lngbuffs[i]->seqNum == j + 1) {
                    luaL_addstring(&buff, lngbuffs[i]->buff);
                }
            }
        }
        luaL_pushresult(&buff);
    }
    else {
        lua_pushlstring(L, dst, dstlen);
    }
    //Add metadata
    lua_newtable(L);

    //Total number of long text messages
    lua_pushinteger(L, sms->refNum);
    lua_setfield(L, -2, "refNum");
    // current serial number
    lua_pushinteger(L, sms->seqNum);
    lua_setfield(L, -2, "seqNum");
    // current serial number
    lua_pushinteger(L, sms->maxNum);
    lua_setfield(L, -2, "maxNum");

    // time information
    lua_pushinteger(L, sms->time.year);
    lua_setfield(L, -2, "year");
    lua_pushinteger(L, sms->time.month);
    lua_setfield(L, -2, "mon");
    lua_pushinteger(L, sms->time.day);
    lua_setfield(L, -2, "day");
    lua_pushinteger(L, sms->time.hour);
    lua_setfield(L, -2, "hour");
    lua_pushinteger(L, sms->time.minute);
    lua_setfield(L, -2, "min");
    lua_pushinteger(L, sms->time.second);
    lua_setfield(L, -2, "sec");
    lua_pushinteger(L, sms->time.tz_sign == '+' ? sms->time.tz : - sms->time.tz);
    lua_setfield(L, -2, "tz");

}


static int l_sms_recv_handler(lua_State* L, void* ptr) {
    LUAT_SMS_RECV_MSG_T* sms = ((LUAT_SMS_RECV_MSG_T*)ptr);
    // char buff[280+2] = {0};
    size_t dstlen = strlen(sms->sms_buffer);
    char tmpbuff[140*3+2] = {0};
    char *dst = tmpbuff;

    LLOGD("dcs %d | %d | %d | %d", sms->dcs_info.alpha_bet, sms->dcs_info.dcs, sms->dcs_info.msg_class, sms->dcs_info.type);

    if (sms->dcs_info.alpha_bet == 0) {
        memcpy(dst, sms->sms_buffer, strlen(sms->sms_buffer));
    }
    else {
        ucs2char(sms->sms_buffer, strlen(sms->sms_buffer), dst, &dstlen);
        dst[dstlen] = 0;
    }

    if (sms->maxNum > 0 && lua_sms_recv_long) {
        int index = -1;
        for (size_t i = 0; i < LONG_SMS_CMAX; i++)
        {
            if (lngbuffs[i] == NULL) {
                index = i;
                break;
            }
        }
        if (index < 0) {
            LLOGE("too many long-sms!!");
            goto exit;
        }
        lngbuffs[index] = luat_heap_malloc(sizeof(long_sms_t) + dstlen);
        if (lngbuffs[index] == NULL) {
            LLOGE("out of memory when malloc long sms buff");
            goto exit;
        }
        lngbuffs[index]->maxNum = sms->maxNum;
        lngbuffs[index]->seqNum = sms->seqNum;
        lngbuffs[index]->refNum = sms->refNum;
        memcpy(lngbuffs[index]->buff, dst, dstlen);
        lngbuffs[index]->buff[dstlen] = 0x00;
        size_t counter = (sms->maxNum + 1) *  sms->maxNum / 2;
        for (size_t i = 0; i < LONG_SMS_CMAX; i++)
        {
            if (lngbuffs[i] == NULL || lngbuffs[i]->refNum != sms->refNum) {
                continue;
            }
            counter -= lngbuffs[i]->seqNum;
        }
        if (counter != 0) {
            LLOGI("long-sms, wait more frags %d/%d", sms->seqNum, sms->maxNum);
            goto exit;
        }
        LLOGI("long-sms is ok");
        dst = NULL;
        dstlen = sms->refNum;
    }

    // Send system message first
    lua_getglobal(L, "sys_pub");
    if (lua_isnil(L, -1)) {
        luat_heap_free(sms);
        return 0;
    }
/*@sys_pub sms
Receive text message
SMS_INC
@string mobile phone number
@string SMS content, UTF8 encoding
@usage
--Examples of use, can be multiple lines
-- Multiple methods are supported for receiving text messages, just choose one.
-- 1. Set callback function
--sms.setNewSmsCb( function(phone,sms)
    log.info("sms",phone,sms)
end)
-- 2. Subscribe to system messages
--sys.subscribe("SMS_INC", function(phone,sms)
    log.info("sms",phone,sms)
end)*/
    lua_pushliteral(L, "SMS_INC");
    push_sms_args(L, sms, dst, dstlen);
    lua_call(L, 4, 0);

    // If there is a callback function, call it
    if (lua_sms_ref) {
        lua_geti(L, LUA_REGISTRYINDEX, lua_sms_ref);
        if (lua_isfunction(L, -1)) {
            push_sms_args(L, sms, dst, dstlen);
            lua_call(L, 3, 0);
        }
    }
    // Clear the buffer of long text messages, if any
    for (size_t i = 0; i < 16; i++)
    {
        if (lngbuffs[i] && lngbuffs[i]->refNum == sms->refNum) {
            luat_heap_free(lngbuffs[i]);
            lngbuffs[i] = NULL;
        }
    }

exit:
    luat_heap_free(sms);
    return 0;
}

void luat_sms_recv_cb(uint32_t event, void *param)
{
    LUAT_SMS_RECV_MSG_T* sms = ((LUAT_SMS_RECV_MSG_T*)param);
    rtos_msg_t msg = {0};
    if (event != 0) {
        return;
    }
    LUAT_SMS_RECV_MSG_T* tmp = luat_heap_malloc(sizeof(LUAT_SMS_RECV_MSG_T));
    if (tmp == NULL) {
        LLOGE("out of memory when malloc sms content");
        return;
    }
    memcpy(tmp, sms, sizeof(LUAT_SMS_RECV_MSG_T));
    msg.handler = l_sms_recv_handler;
    msg.ptr = tmp;
    luat_msgbus_put(&msg, 0);
}

/*sending a text message
@api sms.send(phone, msg, auto_phone_fix)
@string phone number, required
@string SMS content, required
@bool Whether to automatically process the format of the phone number. The default is to automatically judge based on the text message content and number format. Set to false to disable it.
@return bool Returns true if successful, otherwise returns false or nil
@usgae
--SMS number supports 2 forms
-- +XXYYYYYYY where XX represents the country code, China is 86, it is recommended to use this
-- YYYYYYYYY directly fill in the target number, such as 10010, 10086, or domestic mobile phone number
log.info("sms", sms.send("+8613416121234", "Hi, LuatOS - " .. os.date()))

-- Use the target number directly without any automated processing. Added on 2023.09.21
log.info("sms", sms.send("85513416121234", "Hi, LuatOS - " .. os.date()), false)*/
static int l_sms_send(lua_State *L) {
    size_t phone_len = 0;
    size_t payload_len = 0;
    const char* phone = luaL_checklstring(L, 1, &phone_len);
    const char* payload = luaL_checklstring(L, 2, &payload_len);
    int auto_phone = 1;
    if (lua_isboolean(L, 3) && !lua_toboolean(L, 3)) {
        auto_phone = 0;
    }
    int ret = 0;
    char phone_buff[32] = {0};

    if (payload_len == 0) {
        LLOGI("sms is emtry");
        return 0;
    }
    if (payload_len > 140) {
        LLOGI("sms is too long %d", payload_len);
        return 0;
    }
    int pdu_mode = 0;
    for (size_t i = 0; i < payload_len; i++)
    {
        if (payload[i] & 0x80) {
            LLOGD("found non-ASCII char, using PDU mode");
            pdu_mode = 1;
            break;
        }
    }

    
    if (phone_len < 3 || phone_len > 29) {
        LLOGI("phone is too short or too long!! %d", phone_len);
        return 0;
    }
    uint8_t gateway_mode = 0;	//Special processing of SMS gateway
    if ((phone_len >= 15) && !memcmp(phone, "10", 2)) {
    	LLOGI("sms gateway mode");
    	gateway_mode = 1;
    	pdu_mode = 1;
    	memcpy(phone_buff, phone, phone_len);
    	goto NUMBER_CHECK_DONE;
    }
    // +8613416121234
    if (auto_phone) {

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
    }
    else {
        memcpy(phone_buff, phone, phone_len);
    }
    
NUMBER_CHECK_DONE:
    phone_len = strlen(phone_buff);
    phone = phone_buff;
    LLOGD("phone [%s]", phone);
    if (pdu_mode) {
        char pdu[280 + 100] = {0};
        // First, fill in the PDU header
        strcat(pdu, "00"); // Use the built-in SMS center, which cannot be set temporarily.
        strcat(pdu, "01"); // Only receive information, no retention time is passed
        strcat(pdu, "00"); // TP-MR, always fill in 0
        sprintf_(pdu + strlen(pdu), "%02X", phone_len); // phone number length
        if (gateway_mode) {
        	strcat(pdu, "81"); //Destination address format
        } else {
        	strcat(pdu, "91"); //Destination address format
        }
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
                    //LLOGD("unicode %04X %02X%02X%02X", unicode, payload[i], payload[i+1], payload[i+2]);
                    sprintf_(pdu + strlen(pdu), "%02X%02X", (unicode >> 8) & 0xFF, unicode & 0xFF);
                    i+=2;
                    pdu_userdata_len += 2;
                    continue;
                }
                if (payload[i] & 0xC0) { // 110xxxxx 10xxxxxx
                    unicode = ((payload[i] & 0x1F) << 6) + (payload[i+1] & 0x3F);
                    //LLOGD("unicode %04X %02X%02X", unicode, payload[i], payload[i+1]);
                    sprintf_(pdu + strlen(pdu), "%02X%02X", (unicode >> 8) & 0xFF, unicode & 0xFF);
                    i++;
                    pdu_userdata_len += 2;
                    continue;
                }
                LLOGD("bad UTF8 string");
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
        LLOGD("PDU %s", pdu);
        payload = pdu;
        payload_len = strlen(pdu);
        phone = "";
        ret = luat_sms_send_msg(pdu, "", 1, 54);
        lua_pushboolean(L, ret == 0 ? 1 : 0);
    }
    else {
        ret = luat_sms_send_msg(payload, phone, 0, 0);
        lua_pushboolean(L, ret == 0 ? 1 : 0);
    }
    LLOGD("sms ret %d", ret);
    return 1;
}

/**
Set callback function for new SMS
@api sms.setNewSmsCb(func)
@function callback function, 3 parameters, num, txt, metas
@return nil If the function is passed in, it will succeed and there will be no return value.
@usage

sms.setNewSmsCb(function(num, txt, metas)
    -- num mobile phone number
    -- txt text content
    -- metas metadata of text messages, such as sending time, long text message number
    -- Note that long text messages will be automatically merged into one txt
    log.info("sms", num, txt, metas and json.encode(metas) or "")
end)*/
static int l_sms_cb(lua_State *L) {
    if (lua_sms_ref) {
        luaL_unref(L, LUA_REGISTRYINDEX, lua_sms_ref);
        lua_sms_ref = 0;
    }
    if (lua_isfunction(L, 1)) {
        lua_sms_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return 0;
}

/**
Set up the automatic merging function of long text messages
@api sms.autoLong(mode)
@bool Whether to automatically merge, true means automatic merge, which is the default value
@return bool value after setting
@usage
-- Disable the automatic merging of long text messages. Generally, there is no need to disable it.
sms.autoLong(false)*/
static int l_sms_auto_long(lua_State *L) {
    if (lua_isboolean(L, 1)) {
        lua_sms_recv_long = lua_toboolean(L, 1);
    }
    else if (lua_isinteger(L, 1))
    {
        lua_sms_recv_long = lua_toboolean(L, 1);
    }
    lua_pushboolean(L, lua_sms_recv_long == 0 ? 0 : 1);
    return 1;
}

/**
Clear cache of long text messages
@api sms.clearLong()
@return int Number of cleared fragments
@usage
sms.clearLong()*/
static int l_sms_clear_long(lua_State *L) {
    int counter = 0;
    for (size_t i = 0; i < LONG_SMS_CMAX; i++)
    {
        if (lngbuffs[i]) {
            counter ++;
            luat_heap_free(lngbuffs[i]);
            lngbuffs[i] = NULL;
        }
    }
    lua_pushinteger(L, counter);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_sms[] =
{
    { "send",           ROREG_FUNC(l_sms_send)},
    { "setNewSmsCb",    ROREG_FUNC(l_sms_cb)},
    { "autoLong",       ROREG_FUNC(l_sms_auto_long)},
    { "clearLong",      ROREG_FUNC(l_sms_clear_long)},
	{ NULL,             ROREG_INT(0)}
};


LUAMOD_API int luaopen_sms( lua_State *L ) {
    luat_newlib2(L, reg_sms);
    return 1;
}
