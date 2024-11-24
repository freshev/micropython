
/*@Modulescc
@summary VoLTE calling function
@version 1.0
@date 2024.1.17
@democc
@tag LUAT_USE_VOLTE
@usage
-- Currently only Air780EPV supports VoLTE calling function*/

#include "luat_base.h"
#include "luat_mem.h"
#include "luat_rtos.h"
#include "luat_msgbus.h"
#include "luat_zbuff.h"
#include "luat_mobile.h"
#include "luat_network_adapter.h"

#include "luat_i2s.h"
#include "luat_audio.h"

#define LUAT_LOG_TAG "cc"
#include "luat_log.h"
enum{
	VOLTE_EVENT_PLAY_TONE = 1,
	VOLTE_EVENT_RECORD_VOICE_START,
	VOLTE_EVENT_RECORD_VOICE_UPLOAD,
	VOLTE_EVENT_PLAY_VOICE,
	VOLTE_EVENT_HANGUP,
	VOLTE_EVENT_CALL_READY,
};

//Playback control
typedef struct
{
	luat_rtos_task_handle task_handle;
	luat_zbuff_t *up_buff[2];
	luat_zbuff_t *down_buff[2];
	int record_cb;
	HANDLE record_timer;
	uint32_t next_download_point;
	uint8_t *download_buffer;
	uint8_t total_download_cnt;
	uint8_t play_type;
	uint8_t record_type;
	uint8_t is_call_uplink_on;
	uint8_t record_on_off;
	uint8_t record_start;
	uint8_t upload_need_stop;
	volatile uint8_t record_down_zbuff_point;
	volatile uint8_t record_up_zbuff_point;
}luat_cc_ctrl_t;
static luat_cc_ctrl_t luat_cc;

static int l_cc_handler(lua_State *L, void* ptr) {
    (void)ptr;
    //LLOGD("l_uart_handler");
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_pop(L, 1);
    if (luat_cc.record_on_off && luat_cc.record_cb)
    {
    	lua_geti(L, LUA_REGISTRYINDEX, luat_cc.record_cb);
        if (lua_isfunction(L, -1)) {
        	lua_pushboolean(L, msg->arg1);
        	lua_pushinteger(L, msg->arg2);
        	lua_call(L, 2, 0);
        }
    }
    // Return empty data to rtos.recv method
    lua_pushinteger(L, 0);
    return 1;
}

static void mobile_voice_data_input(uint8_t *input, uint32_t len, uint32_t sample_rate, uint8_t bits){
	if (luat_cc.record_on_off) {
        luat_cc.record_down_zbuff_point = 0;
        luat_cc.download_buffer = (uint8_t *)input;
		if (1 == sample_rate)
		{
			luat_cc.total_download_cnt = 6;
		}
		else
		{
			luat_cc.total_download_cnt = 3;
		}
        memcpy(luat_cc.down_buff[0]->addr, luat_cc.download_buffer, sample_rate * 320);
        luat_cc.down_buff[0]->used = sample_rate * 320;
        luat_cc.next_download_point = 1;
		luat_start_rtos_timer(luat_cc.record_timer, 20, 1);
		if (luat_cc.down_buff[0]->used >= luat_cc.down_buff[0]->len) {
			rtos_msg_t msg;
			msg.handler = l_cc_handler;
			msg.arg1 = 1;
			msg.arg2 = 0;
			luat_msgbus_put(&msg, 0);
			luat_cc.record_down_zbuff_point = !luat_cc.record_down_zbuff_point;
			luat_cc.down_buff[luat_cc.record_down_zbuff_point]->used = 0;
		}
	}
	luat_rtos_event_send(luat_cc.task_handle, VOLTE_EVENT_PLAY_VOICE, (uint32_t)input, len, sample_rate, 0);

}

static int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param){
	if (luat_cc.upload_need_stop) return 0;
	switch(event){
	case LUAT_I2S_EVENT_RX_DONE:
		luat_rtos_event_send(luat_cc.task_handle, VOLTE_EVENT_RECORD_VOICE_UPLOAD, (uint32_t)rx_data, rx_len, 0, 0);
		break;
	case LUAT_I2S_EVENT_TX_DONE:
	case LUAT_I2S_EVENT_TRANSFER_DONE:
		break;
	default:
		break;
	}
	return 0;
}

static LUAT_RT_RET_TYPE download_data_callback(LUAT_RT_CB_PARAM)
{
	if (luat_cc.record_type && luat_cc.record_on_off) {
		luat_zbuff_t *buff = luat_cc.down_buff[luat_cc.record_down_zbuff_point];
		memcpy(buff->addr + buff->used, luat_cc.download_buffer + luat_cc.next_download_point * luat_cc.record_type * 320, luat_cc.record_type * 320);//20ms recording completed
		luat_cc.next_download_point = (luat_cc.next_download_point + 1) % luat_cc.total_download_cnt;
		buff->used += luat_cc.record_type * 320;
		if (buff->used >= buff->len) {
			rtos_msg_t msg;
            msg.handler = l_cc_handler;
            msg.arg2 = luat_cc.record_down_zbuff_point;
            msg.arg1 = 1;
            luat_msgbus_put(&msg, 0);
            luat_cc.record_down_zbuff_point = !luat_cc.record_down_zbuff_point;
            luat_cc.down_buff[luat_cc.record_down_zbuff_point]->used = 0;
		}
	}
}

static void luat_volte_task(void *param){
	luat_zbuff_t *zbuff = NULL;
	luat_event_t event;
	uint8_t multimedia_id = (int)param;
	luat_audio_conf_t* audio_conf = luat_audio_get_config(multimedia_id);
	luat_i2s_conf_t *i2s = luat_i2s_get_config(multimedia_id);
	i2s->is_full_duplex = 1;
	i2s->luat_i2s_event_callback = record_cb;
	while (1){
		luat_rtos_event_recv(luat_cc.task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		switch(event.id)
		{
		case VOLTE_EVENT_PLAY_TONE:
            if (LUAT_MOBILE_CC_PLAY_STOP == event.param1){
                luat_cc.record_type = 0;
                luat_cc.play_type = 0;
				if (luat_rtos_timer_is_active(luat_cc.record_timer))
				{
					luat_rtos_timer_stop(luat_cc.record_timer);
					rtos_msg_t msg;
					msg.handler = l_cc_handler;
					msg.arg2 = luat_cc.record_up_zbuff_point;
					msg.arg1 = 0;
					luat_msgbus_put(&msg, 0);
					msg.arg2 = luat_cc.record_down_zbuff_point;
					msg.arg1 = 1;
					luat_msgbus_put(&msg, 0);
				}
                luat_cc.is_call_uplink_on = 0;
                luat_audio_speech_stop(multimedia_id);
	            LLOGD("VOLTE_EVENT_PLAY_STOP");
                break;
            }
			break;
		case VOLTE_EVENT_RECORD_VOICE_START:
			luat_cc.record_type = event.param1;
			luat_cc.is_call_uplink_on = 1;
			luat_audio_speech(multimedia_id, 0, event.param1, NULL, 0, 1);
            luat_cc.record_up_zbuff_point = 0;
            if (luat_cc.record_on_off) {
            	luat_cc.up_buff[0]->used = 0;
            }
            LLOGD("VOLTE_EVENT_RECORD_VOICE_START");
			break;
		case VOLTE_EVENT_RECORD_VOICE_UPLOAD:
			if (!luat_cc.is_call_uplink_on) break;
			if (luat_cc.upload_need_stop) {
				LLOGD("VOLTE RECORD VOICE ALREADY STOP");
				break;
			}
			if (luat_cc.record_on_off && luat_cc.record_type) {
				zbuff = luat_cc.up_buff[luat_cc.record_up_zbuff_point];
				memcpy(zbuff->addr + zbuff->used, (uint8_t *)event.param1, event.param2);
				zbuff->used += event.param2;
			}
            if (luat_cc.record_type) {
				luat_mobile_speech_upload((uint8_t *)event.param1, event.param2);
			}
            if (luat_cc.record_on_off && luat_cc.record_type) {
				if (zbuff->used >= zbuff->len) {
					rtos_msg_t msg;
					msg.handler = l_cc_handler;
					msg.arg2 = luat_cc.record_up_zbuff_point;
					msg.arg1 = 0;
					luat_msgbus_put(&msg, 0);
					luat_cc.record_up_zbuff_point = !luat_cc.record_up_zbuff_point;
					luat_cc.up_buff[luat_cc.record_up_zbuff_point]->used = 0;
				}
            }
			break;
		case VOLTE_EVENT_PLAY_VOICE:
			luat_cc.play_type = event.param3; //1 = 8K 2 = 16K
			luat_audio_speech(multimedia_id, 1, event.param3, (uint8_t *)event.param1, event.param2, 1);
			LLOGD("VOLTE PLAY VOICE");
			break;
		case VOLTE_EVENT_HANGUP:
			luat_mobile_hangup_call(event.param1);
			break;
		}
	}
}

/**
Get the number of the last call
@apicc.lastNum()
@return string Get the number of the last call*/
static int l_cc_get_last_call_num(lua_State* L) {
    char number[64] = {0};
    luat_mobile_get_last_call_num(number, sizeof(number));
    lua_pushlstring(L, (const char*)(number),strlen(number));
    return 1;
}

/**
Make a call
@api cc.dial(sim_id, number)
@number sim_id
@string phone number
@return bool Whether the call was successful or not*/
static int l_cc_make_call(lua_State* L) {
    uint8_t sim_id = luaL_optinteger(L, 1, 0);
    size_t len = 0;
	char* number = luaL_checklstring(L, 2, &len);
    lua_pushboolean(L, !luat_mobile_make_call(sim_id,number, len));
    return 1;
}

/**
hang up the phone
@api cc.hangUp(sim_id)
@number sim_id*/
static int l_cc_hangup_call(lua_State* L) {
    uint8_t sim_id = luaL_optinteger(L, 1, 0);
    luat_rtos_event_send(luat_cc.task_handle, VOLTE_EVENT_HANGUP, sim_id, 0, 0, 0);
    return 0;
}

/**
Answer the call
@api cc.accept(sim_id)
@number sim_id
@return bool Whether the call was answered successfully or not*/
static int l_cc_answer_call(lua_State* L) {
    uint8_t sim_id = luaL_optinteger(L, 1, 0);
    lua_pushboolean(L, !luat_mobile_answer_call(sim_id));
    return 1;
}

/**
Initialize phone function
@api cc.init(multimedia_id)
@number multimedia_id multimedia id
@return bool success or failure*/
static int l_cc_speech_init(lua_State* L) {
    uint8_t multimedia_id = luaL_optinteger(L, 1, 0);
    if (luat_mobile_speech_init(multimedia_id,mobile_voice_data_input)){
        lua_pushboolean(L, 0);
        return 1;
    }
    luat_cc.record_timer = luat_create_rtos_timer(download_data_callback, NULL, NULL);
    luat_rtos_task_create(&luat_cc.task_handle, 4*1024, 100, "volte", luat_volte_task, multimedia_id, 64);
    lua_pushboolean(L, 1);
    return 1;
}

/**
Record a call
@api cc.record(on_off,upload_zbuff1, upload_zbuff2, download_zbuff1, download_zbuff2)
@boolean turns on and off the call recording function, false or nil turns it off, others turns it on
@zbuff Upstream data storage area 1, the space capacity when zbuff is created must be a multiple of 640, the same below
@zbuff Upstream data storage area 2 and upstream data storage area 1 form a double buffer
@zbuff Downstream data storage area 1
@zbuff Downstream data storage area 2 and downstream data storage area 1 form a double buffer
@return bool Whether it is successful or not, if it is in a call state, it will fail
@usage
buff1 = zbuff.create(6400,0,zbuff.HEAP_AUTO)
buff2 = zbuff.create(6400,0,zbuff.HEAP_AUTO)
buff3 = zbuff.create(6400,0,zbuff.HEAP_AUTO)
buff4 = zbuff.create(6400,0,zbuff.HEAP_AUTO)
cc.on("record", function(type, buff_point)
 log.info(type, buff_point) -- type==true is the downstream data, false is the upstream data buff_point indicates which one of the double buffers is returned
end)
cc.record(true, buff1, buff2, buff3, buff4)*/
static int l_cc_record_call(lua_State* L) {
	if (luat_cc.record_type)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
    luat_cc.record_on_off = lua_toboolean(L, 1);
    if (luat_cc.record_on_off)
    {
    	luat_cc.up_buff[0] = (luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE);
    	luat_cc.up_buff[1] = (luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE);
    	luat_cc.down_buff[0] = (luat_zbuff_t *)luaL_checkudata(L, 4, LUAT_ZBUFF_TYPE);
    	luat_cc.down_buff[1] = (luat_zbuff_t *)luaL_checkudata(L, 5, LUAT_ZBUFF_TYPE);
    }
    else
    {
    	luat_cc.up_buff[0] = NULL;
    	luat_cc.up_buff[1] = NULL;
    	luat_cc.down_buff[0] = NULL;
    	luat_cc.down_buff[1] = NULL;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/**
Get current call quality
@apicc.quality()
@return int 1 is low quality (8K), 2 is high quality (16k), 0 is not talking*/
static int l_cc_get_quality(lua_State* L) {
    lua_pushinteger(L, luat_cc.record_type);
    return 1;
}

/**
Register callback
@api cc.on(event, func)
@string event name audio recording data is "record"
@function callback method
@return nil no return value
@usage
cc.on("record", function(type, buff_point)
 log.info(type, buff_point) -- type==true is the downstream data, false is the upstream data buff_point indicates which one of the double buffers is returned
end)*/
static int l_cc_on(lua_State *L) {
    const char* event = luaL_checkstring(L, 1);
    if (!strcmp("record", event)) {
        if (luat_cc.record_cb != 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, luat_cc.record_cb);
            luat_cc.record_cb = 0;
        }
        if (lua_isfunction(L, 2)) {
            lua_pushvalue(L, 2);
            luat_cc.record_cb = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }
    return 0;
}


#include "rotable2.h"
static const rotable_Reg_t reg_cc[] =
{
    { "init" ,      ROREG_FUNC(l_cc_speech_init)},
    { "dial" ,      ROREG_FUNC(l_cc_make_call)},
    { "accept" ,    ROREG_FUNC(l_cc_answer_call)},
    { "hangUp" ,    ROREG_FUNC(l_cc_hangup_call)},
    { "lastNum" ,   ROREG_FUNC(l_cc_get_last_call_num)},
	{ "quality" ,   ROREG_FUNC(l_cc_get_quality)},
    { "on" ,        ROREG_FUNC(l_cc_on)},
    { "record", ROREG_FUNC(l_cc_record_call)},
	{ NULL,          {}}
};

LUAMOD_API int luaopen_cc( lua_State *L ) {
    luat_newlib2(L, reg_cc);
    return 1;
}



void luat_cc_start_speech(uint32_t param)
{
	luat_cc.upload_need_stop = 0;
	luat_rtos_event_send(luat_cc.task_handle, VOLTE_EVENT_RECORD_VOICE_START, param, 0, 0, 0);
}

void luat_cc_play_tone(uint32_t param)
{
	if (!param) luat_cc.upload_need_stop = 1;
	luat_rtos_event_send(luat_cc.task_handle, VOLTE_EVENT_PLAY_TONE, param, 0, 0, 0);
}

