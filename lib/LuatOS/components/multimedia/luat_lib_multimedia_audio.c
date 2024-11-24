
/*@Modulesaudio
@summary Multimedia-Audio
@version 1.0
@date 2022.03.11
@demomultimedia
@tagLUAT_USE_MEDIA*/
#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_zbuff.h"
#define LUAT_LOG_TAG "audio"
#include "luat_log.h"

#include "luat_multimedia.h"
#include "luat_audio.h"
#include "luat_mem.h"

#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif
#ifdef LUAT_USE_RECORD
static luat_record_ctrl_t g_s_record = {0};
#endif
static luat_multimedia_cb_t multimedia_cbs[MAX_DEVICE_COUNT];

int l_multimedia_raw_handler(lua_State *L, void* ptr) {
    (void)ptr;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    if (multimedia_cbs[msg->arg2].function_ref) {
        lua_geti(L, LUA_REGISTRYINDEX, multimedia_cbs[msg->arg2].function_ref);
        if (lua_isfunction(L, -1)) {
            lua_pushinteger(L, msg->arg2);
            lua_pushinteger(L, msg->arg1);
#ifdef LUAT_USE_RECORD
            if (msg->arg1 == LUAT_MULTIMEDIA_CB_RECORD_DATA){
                lua_pushlightuserdata(L, g_s_record.record_buffer[(int)msg->ptr]);
                lua_call(L, 3, 0);
            }else{
                lua_call(L, 2, 0);
                if (msg->arg1 == LUAT_MULTIMEDIA_CB_RECORD_DONE){
                    luaL_unref(L,LUA_REGISTRYINDEX, g_s_record.zbuff_ref[0]);
                    if (g_s_record.record_buffer[0]->addr){
                        luat_heap_opt_free(g_s_record.record_buffer[0]->type,g_s_record.record_buffer[0]->addr);
                        g_s_record.record_buffer[0]->addr = NULL;
                        g_s_record.record_buffer[0]->len = 0;
                        g_s_record.record_buffer[0]->used = 0;
                    }
                    luaL_unref(L,LUA_REGISTRYINDEX, g_s_record.zbuff_ref[1]);
                    if (g_s_record.record_buffer[1]->addr){
                        luat_heap_opt_free(g_s_record.record_buffer[1]->type,g_s_record.record_buffer[1]->addr);
                        g_s_record.record_buffer[1]->addr = NULL;
                        g_s_record.record_buffer[1]->len = 0;
                        g_s_record.record_buffer[1]->used = 0;
                    }
                }
            }
#else
            lua_call(L, 2, 0);
#endif
        }
    }
    lua_pushinteger(L, 0);
    return 1;
}

/*Start a multimedia channel ready to play audio
@api audio.start(id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed)
@int multimedia playback channel number
@int audio format
@int number of sound channels
@int sampling frequency
@int number of sampling bits
@boolean whether there is a symbol, default true
@return boolean true on success, false on failure
@usage
audio.start(0, audio.PCM, 1, 16000, 16)*/
static int l_audio_start_raw(lua_State *L){
	int multimedia_id = luaL_checkinteger(L, 1);
	int audio_format = luaL_checkinteger(L, 2);
	int num_channels= luaL_checkinteger(L, 3);
	int sample_rate = luaL_checkinteger(L, 4);
	int bits_per_sample = luaL_checkinteger(L, 5);
	int is_signed = 1;
	if (lua_isboolean(L, 6))
	{
		is_signed = lua_toboolean(L, 6);
	}
	lua_pushboolean(L, !luat_audio_start_raw(multimedia_id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed));
    return 1;
}

#ifdef LUAT_USE_RECORD

#ifdef LUAT_SUPPORT_AMR
#include "interf_enc.h"
#include "interf_dec.h"
#endif
#include "luat_fs.h"
#define RECORD_ONCE_LEN	5


#ifdef LUAT_SUPPORT_AMR
static void record_encode_amr(uint8_t *data, uint32_t len){
	uint8_t outbuf[64];
	int16_t *pcm = (int16_t *)data;
	uint32_t total_len = len >> 1;
	uint32_t done_len = 0;
	uint8_t out_len;
	uint32_t pcm_len = (g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB)?160:320;
	while ((total_len - done_len) >= pcm_len){
#ifdef LUAT_USE_INTER_AMR
		luat_audio_inter_amr_coder_encode(g_s_record.encoder_handler, &pcm[done_len], outbuf,&out_len);
#else
        out_len = Encoder_Interface_Encode(g_s_record.encoder_handler, g_s_record.quailty , &pcm[done_len], outbuf, 0);
#endif
		if (out_len <= 0){
			LLOGD("encode error in %d,result %d", done_len, out_len);
		}else{
            luat_fs_fwrite(outbuf, out_len, 1, g_s_record.fd);
		}
		done_len += pcm_len;
	}
}

static void record_stop_encode_amr(void){
#ifdef LUAT_USE_INTER_AMR
	luat_audio_inter_amr_coder_deinit(g_s_record.encoder_handler);
#else
	Encoder_Interface_exit(g_s_record.encoder_handler);
#endif
	g_s_record.encoder_handler = NULL;
}
#endif

static void record_stop(uint8_t *data, uint32_t len);
static void record_run(uint8_t *data, uint32_t len)
{
	rtos_msg_t msg = {0};
	if (g_s_record.fd){
#ifdef LUAT_SUPPORT_AMR
		if (g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB||g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_WB){
			record_encode_amr(data, len);
		}
		else
#endif
		{
			luat_fs_fwrite(data, len, 1, g_s_record.fd);
		}
	}else{
		memcpy(g_s_record.record_buffer[g_s_record.record_buffer_index]->addr + g_s_record.record_buffer[g_s_record.record_buffer_index]->used, data, len);
		g_s_record.record_buffer[g_s_record.record_buffer_index]->used += len;
		if (g_s_record.record_buffer[g_s_record.record_buffer_index]->used >= g_s_record.record_buffer[g_s_record.record_buffer_index]->len)
		{
			msg.handler = l_multimedia_raw_handler;
			msg.arg1 = LUAT_MULTIMEDIA_CB_RECORD_DATA;
			msg.arg2 = g_s_record.multimedia_id;
			msg.ptr = g_s_record.record_buffer_index;
			luat_msgbus_put(&msg, 1);
			g_s_record.record_buffer_index = !g_s_record.record_buffer_index;
			g_s_record.record_buffer[g_s_record.record_buffer_index]->used = 0;
		}


	}
	if (g_s_record.record_time)
	{
		g_s_record.record_time_tmp++;
		if (g_s_record.record_time_tmp >= (g_s_record.record_time * 10) )
		{
			record_stop(NULL, 0);
		}
	}
}

static int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param)
{
	switch(event)
	{
	case LUAT_I2S_EVENT_RX_DONE:
		luat_audio_run_callback_in_task(record_run, rx_data, rx_len);
		break;
	default:
		break;
	}
	return 0;
}

static void record_start(uint8_t *data, uint32_t len){
	luat_i2s_conf_t *i2s = luat_i2s_get_config(g_s_record.multimedia_id);
	g_s_record.bak_cb_rx_len = i2s->cb_rx_len;
	g_s_record.bak_is_full_duplex = i2s->is_full_duplex;
	g_s_record.bak_sample_rate = i2s->sample_rate;
	g_s_record.bak_luat_i2s_event_callback = i2s->luat_i2s_event_callback;


	i2s->is_full_duplex = 1;
	i2s->luat_i2s_event_callback = record_cb;
    if (g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB){
    	i2s->cb_rx_len = 320 * RECORD_ONCE_LEN;
        i2s->sample_rate = 8000;
    }else if(g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_WB){
    	i2s->cb_rx_len = 640 * RECORD_ONCE_LEN;
        i2s->sample_rate = 16000;
    }else if(g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_PCM){
    	i2s->cb_rx_len = 320 * RECORD_ONCE_LEN;
        i2s->sample_rate = 8000;
    }

    //Need to save the file and turn on the encoding function depending on the situation
    if (g_s_record.fd){
        if (g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB||g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_WB){
#ifdef LUAT_SUPPORT_AMR
#ifdef LUAT_USE_INTER_AMR
            g_s_record.encoder_handler = luat_audio_inter_amr_coder_init(g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB?0:1, g_s_record.quailty);
#else
            g_s_record.encoder_handler = Encoder_Interface_init(g_s_record.quailty);
#endif
        if (g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB){
            luat_fs_fwrite("#!AMR\n", 6, 1, g_s_record.fd);
        }else{
            luat_fs_fwrite("#!AMR-WB\n", 9, 1, g_s_record.fd);
        }
#endif
        }
    }
	luat_audio_record_and_play(g_s_record.multimedia_id, i2s->sample_rate, NULL, 3200, 2);
}


static void record_stop(uint8_t *data, uint32_t len){
	rtos_msg_t msg = {0};
	//Turn off the audio hardware function
	luat_audio_record_stop(g_s_record.multimedia_id);
	luat_audio_pm_request(g_s_record.multimedia_id, LUAT_AUDIO_PM_STANDBY);
	//Restore parameters
	luat_i2s_conf_t *i2s = luat_i2s_get_config(g_s_record.multimedia_id);
	i2s->cb_rx_len = g_s_record.bak_cb_rx_len;
	i2s->is_full_duplex = g_s_record.bak_is_full_duplex;
	i2s->sample_rate = g_s_record.bak_sample_rate;
	i2s->luat_i2s_event_callback = g_s_record.bak_luat_i2s_event_callback;
	//When recording and saving files, turn off the encoding function depending on the situation.
	if (g_s_record.fd) {
		if (g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB||g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_WB){
#ifdef LUAT_SUPPORT_AMR
			record_stop_encode_amr();
#endif
		}else if(g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_PCM){
			// No special handling required
		}else{
			LLOGE("not support %d", g_s_record.type);
		}
		luat_fs_fclose(g_s_record.fd);
		g_s_record.fd = NULL;
	}
	//Notify the luat task to clear the zbuff data and call back the user
	msg.handler = l_multimedia_raw_handler;
	msg.arg1 = LUAT_MULTIMEDIA_CB_RECORD_DONE;
	msg.arg2 = g_s_record.multimedia_id;
	g_s_record.record_time_tmp = 0;
	g_s_record.is_run = 0;
	g_s_record.record_buffer_index = 0;
	luat_msgbus_put(&msg, 1);
}

/**
recording
@api audio.record(id, record_type, record_time, amr_quailty, path)
@int id multimedia playback channel number
@int record_type recording audio format, supports audio.AMR audio.PCM (some platforms support audio.AMR_WB)
@int record_time recording duration in seconds, optional, default 0 means always recording
@int amr_quailty quality, valid under audio.AMR
@string path recording file path, optional. If not specified, it will not be saved. The original PCM data can be processed in the audio.on callback function.
@int record_callback_time When the recording file path is not specified, the duration of a single recording callback, the unit is 100ms. Default 1, which is 100ms
@return boolean returns true if successful, otherwise returns false
@usage
err,info = audio.record(id, type, record_time, quailty, path)*/
static int l_audio_record(lua_State *L){
    size_t len;
    uint32_t record_buffer_len;
    g_s_record.multimedia_id = luaL_checkinteger(L, 1);
    g_s_record.type = luaL_optinteger(L, 2,LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB);
    g_s_record.record_time = luaL_optinteger(L, 3, 0);
    g_s_record.quailty = luaL_optinteger(L, 4, 0);
    
    if (lua_isstring(L, 5)) {
        const char *path = luaL_checklstring(L, 5, &len);
        luat_fs_remove(path);
        g_s_record.fd = luat_fs_fopen(path, "wb+");
        if(!g_s_record.fd){
            LLOGE("open file %s failed", path);
            return 0;
        }
    }
    if (g_s_record.is_run){
        LLOGE("record is running");
        return 0;
    }
    record_buffer_len = luaL_optinteger(L, 6, 1);
    if (g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB||g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_WB){
#ifdef LUAT_SUPPORT_AMR
    if (g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB){
        record_buffer_len *= 320 * RECORD_ONCE_LEN;
    }else if(g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_AMR_WB){
#ifdef LUAT_USE_INTER_AMR
        record_buffer_len *= 640 * RECORD_ONCE_LEN;
#else
    LLOGE("not support 16k");
    return 0;
#endif
    }
    
#else
    LLOGE("not support AMR");
    return 0;
#endif
    }else if(g_s_record.type==LUAT_MULTIMEDIA_DATA_TYPE_PCM){
        record_buffer_len *= 320 * RECORD_ONCE_LEN;
    }else{
        LLOGE("not support %d", g_s_record.type);
        return 0;
    }

    g_s_record.record_buffer[0] = lua_newuserdata(L, sizeof(luat_zbuff_t));
    g_s_record.record_buffer[0]->type = LUAT_HEAP_AUTO;
    g_s_record.record_buffer[0]->len = record_buffer_len;
    g_s_record.record_buffer[0]->used = 0;
    g_s_record.record_buffer[0]->addr = luat_heap_opt_malloc(LUAT_HEAP_AUTO,g_s_record.record_buffer[0]->len);
    g_s_record.zbuff_ref[0] = luaL_ref(L, LUA_REGISTRYINDEX);

    g_s_record.record_buffer[1] = lua_newuserdata(L, sizeof(luat_zbuff_t));
    g_s_record.record_buffer[1]->type = LUAT_HEAP_AUTO;
    g_s_record.record_buffer[0]->used = 0;
    g_s_record.record_buffer[1]->len = record_buffer_len;
    g_s_record.record_buffer[1]->addr = luat_heap_opt_malloc(LUAT_HEAP_AUTO,g_s_record.record_buffer[1]->len);
    g_s_record.zbuff_ref[1] = luaL_ref(L, LUA_REGISTRYINDEX);

    g_s_record.is_run = 1;
    luat_audio_run_callback_in_task(record_start, NULL, 0);
    lua_pushboolean(L, 1);
    return 1;
}

/**
Recording stops
@api audio.recordStop(id)
@int id multimedia playback channel number
@return boolean returns true if successful, otherwise returns false
@usage
audio.recordStop(0)*/
static int l_audio_record_stop(lua_State *L) {

    if (g_s_record.is_run) {
    	luat_audio_run_callback_in_task(record_stop, NULL, 0);
        lua_pushboolean(L, 1);
        return 1;
    } else {
        LLOGE("record is not running");
        return 0;
    }
}

#endif

/**
Write audio data to a multimedia channel
@api audio.write(id, data)
@string or zbuff audio data
@return boolean returns true if successful, otherwise returns false
@usage
audio.write(0, "xxxxxx")*/
static int l_audio_write_raw(lua_State *L) {
    int multimedia_id = luaL_checkinteger(L, 1);
    size_t len;
    const char *buf;
    if(lua_isuserdata(L, 2))
    {
        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
        len = buff->used;
        buf = (const char *)(buff->addr);
    }
    else
    {
        buf = lua_tolstring(L, 2, &len);//Get string data
    }
	lua_pushboolean(L, !luat_audio_write_raw(multimedia_id, (uint8_t*)buf, len));
    return 1;
}

/**
Stop the specified multimedia channel
@api audio.stop(id)
@int audio id, for example 0
@return boolean returns true if successful, otherwise returns false
@usage
audio.stop(0)*/
static int l_audio_stop_raw(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    lua_pushboolean(L, !luat_audio_stop_raw(id));
    return 1;
}

/**
Pause/resume the specified multimedia channel
@api audio.pause(id, pause)
@int audio id, for example 0
@boolean onoff true pause, false resume
@return boolean returns true if successful, otherwise returns false
@usage
audio.pause(0, true) --Pause channel 0
audio.pause(0, false) --Restore channel 0*/
static int l_audio_pause_raw(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    lua_pushboolean(L, !luat_audio_pause_raw(id, lua_toboolean(L, 2)));
    return 1;
}

/**
Register audio playback event callback
@api audio.on(audio_id, func)
@int audio id, write 0 for audio 0, write 1 for audio 1
@function callback method, the parameters passed in during callback are 1, int channel ID 2, int message value, only audio.MORE_DATA and audio.DONE
@return nil no return value
@usage
audio.on(0, function(audio_id, msg)
    log.info("msg", audio_id, msg)
end)*/
static int l_audio_raw_on(lua_State *L) {
    int multimedia_id = luaL_checkinteger(L, 1);
	if (multimedia_cbs[multimedia_id].function_ref != 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, multimedia_cbs[multimedia_id].function_ref);
		multimedia_cbs[multimedia_id].function_ref = 0;
	}
	if (lua_isfunction(L, 2)) {
		lua_pushvalue(L, 2);
		multimedia_cbs[multimedia_id].function_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

    return 0;
}

/*Play or stop playing a file. After the playback is completed, an audio.DONE message will be called back. You can use pause to pause or resume. Other APIs are not available. Considering that reading the SD card is slow and slows down the luavm process, try to use this API.
@api audio.play(id, path, errStop)
@int audio channel
@string/table file name. If it is empty, it means to stop playing. If it is table, it means to play multiple files continuously. It is mainly used for cloud speakers. Currently only EC618 supports it, and the errStop parameter will be used.
@boolean Whether to stop decoding after file decoding fails. It is only useful when playing multiple files continuously. The default is true. It will automatically stop when encountering decoding errors.
@return boolean returns true if successful, otherwise returns false
@usage
audio.play(0, "xxxxxx") --Start playing a file
audio.play(0) --stop playing a file*/
static int l_audio_play(lua_State *L) {
    int multimedia_id = luaL_checkinteger(L, 1);
    size_t len = 0;
    int result = 0;
    const char *buf;
    uint8_t is_error_stop = 1;
    if (lua_istable(L, 2))
    {
    	size_t len = lua_rawlen(L, 2); //Return the length of the array
    	if (!len)
    	{
        	luat_audio_play_stop(multimedia_id);
        	lua_pushboolean(L, 1);
        	return 1;
    	}
        uData_t *info = (uData_t *)luat_heap_malloc(len * sizeof(uData_t));
        for (size_t i = 0; i < len; i++)
        {
            lua_rawgeti(L, 2, 1 + i);
            info[i].value.asBuffer.buffer = (void*)lua_tolstring(L, -1, &info[i].value.asBuffer.length);
            info[i].Type = UDATA_TYPE_OPAQUE;
            lua_pop(L, 1); //Pop the element value just obtained from the stack
        }
    	if (lua_isboolean(L, 3))
    	{
    		is_error_stop = lua_toboolean(L, 3);
    	}
        result = luat_audio_play_multi_files(multimedia_id, info, len, is_error_stop);
    	lua_pushboolean(L, !result);
    	luat_heap_free(info);
    }
    else if (LUA_TSTRING == (lua_type(L, (2))))
    {
        buf = lua_tolstring(L, 2, &len);//Get string data
        result = luat_audio_play_file(multimedia_id, buf);
    	lua_pushboolean(L, !result);
    }
    else
    {
    	luat_audio_play_stop(multimedia_id);
    	lua_pushboolean(L, 1);
    }
    return 1;
}
#ifdef LUAT_USE_TTS
/*TTS play or stop
@api audio.tts(id, data)
@int audio channel
@string/zbuff Content to be played
@return boolean returns true if successful, otherwise returns false
@tag LUAT_USE_TTS
@usage
audio.tts(0, "test it") --Start playing
audio.tts(0) --stop playing
-- Detailed description of TTS function of Air780E
-- https://wiki.luatos.com/chips/air780e/tts.html*/
static int l_audio_play_tts(lua_State *L) {
    int multimedia_id = luaL_checkinteger(L, 1);
    size_t len = 0;
    int result = 0;
    const char *buf;
    if (LUA_TSTRING == (lua_type(L, (2))))
    {
        buf = lua_tolstring(L, 2, &len);//Get string data
        result = luat_audio_play_tts_text(multimedia_id, (void*)buf, len);
    	lua_pushboolean(L, !result);
    }
    else if(lua_isuserdata(L, 2))
    {
        luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
        result = luat_audio_play_tts_text(multimedia_id, buff->addr, buff->used);
    	lua_pushboolean(L, !result);
    }
    else
    {
    	luat_audio_play_stop(multimedia_id);
    	lua_pushboolean(L, 1);
    }
    return 1;
}
#endif
/**
Stop playing the file, which has the same effect as audio.play(id)
@api audio.playStop(id)
@int audio id, for example 0
@return boolean returns true if successful, otherwise returns false
@usage
audio.playStop(0)*/
static int l_audio_play_stop(lua_State *L) {
    lua_pushboolean(L, !luat_audio_play_stop(luaL_checkinteger(L, 1)));
    return 1;
}


/**
Check if the current file has finished playing
@api audio.isEnd(id)
@int audio channel
@return boolean returns true if successful, otherwise returns false
@usage
audio.isEnd(0)*/
static int l_audio_play_wait_end(lua_State *L) {
    int multimedia_id = luaL_checkinteger(L, 1);
    lua_pushboolean(L, luat_audio_is_finish(multimedia_id));
    return 1;
}

/*Get the latest playback result. Not supported by all platforms. Currently only EC618 supports it.
@api audio.getError(id)
@int audio channel
@return boolean Whether all the files were played successfully, true if successful, false if some files failed to play.
@return boolean If the playback fails, whether the user stopped it, true is, false is not
@return int Which file failed, starting from 1
@usage
local result, user_stop, file_no = audio.getError(0)*/
static int l_audio_play_get_last_error(lua_State *L) {
    int multimedia_id = luaL_checkinteger(L, 1);
	int result = luat_audio_play_get_last_error(multimedia_id);
	lua_pushboolean(L, 0 == result);
	lua_pushboolean(L, result < 0);
	lua_pushinteger(L, result > 0?result:0);
    return 3;
}

/*Configure the characteristics of an audio channel, such as automatically controlling the PA switch. Note that this is not necessary. Generally, automatic control is required when calling play. In other situations, such as when you manually control playback, you can control the PA switch yourself.
@api audio.config(id, paPin, onLevel, dacDelay, paDelay, dacPin, dacLevel, dacTimeDelay)
@int audio channel
@int PA control IO
@int Level when PA is on
@int Redundancy time inserted before DAC starts, unit 100ms, generally used for external DAC
@int After DAC starts, how long to delay turning on PA, unit 1ms
@int External DAC power control IO. If not filled in, it means using the platform default IO. For example, Air780E uses the DACEN pin, but air105 does not enable it.
@int When the external DAC is turned on, the power supply controls the IO level, which is pulled high by default.
@int When the audio playback is completed, the time interval between PA and DAC shut down, unit 1ms, default 0ms
@usage
audio.config(0, pin.PC0, 1) --PA control pin is PC0, high level is turned on, air105 can be used with this configuration
audio.config(0, 25, 1, 6, 200) --The PA control pin is GPIO25 and is turned on at high level. The Air780E cloud speaker board can be used with this configuration.*/
static int l_audio_config(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int pa_pin = luaL_optinteger(L, 2, -1);
    int level = luaL_optinteger(L, 3, 1);
    int dac_pre_delay = luaL_optinteger(L, 4, 5);
    int dac_last_delay = luaL_optinteger(L, 5, 200);
    int dac_power_pin = luaL_optinteger(L, 6, -1);
    int dac_power_level = luaL_optinteger(L, 7, 1);
    int pa_dac_delay = luaL_optinteger(L, 8, 0);
    if (pa_dac_delay < 0)
        pa_dac_delay = 0;
    if (dac_pre_delay < 0)
        dac_pre_delay = 0;
    if (dac_last_delay < 0)
        dac_last_delay = 0;
    luat_audio_config_pa(id, pa_pin, level, (uint32_t)dac_pre_delay, (uint32_t)dac_last_delay);
    luat_audio_config_dac(id, dac_power_pin, dac_power_level, (uint32_t)pa_dac_delay);
    return 0;
}

/*Configure the volume adjustment of an audio channel to directly amplify or reduce the original data. Not all platforms support it. It is recommended to use hardware methods to zoom.
@api audio.vol(id, value)
@int audio channel
@int Volume, percentage, 1%~1000%, default 100%, that is, no adjustment
@return int current volume
@usage
local result = audio.vol(0, 90) --The volume of channel 0 is adjusted to 90%. The result stores the adjusted volume level, which may still be 100.*/
static int l_audio_vol(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int vol = luaL_optinteger(L, 2, 100);
    lua_pushinteger(L, luat_audio_vol(id, vol));
    return 1;
}

/*Configure the mic volume adjustment of an audio channel
@api audio.micVol(id, value)
@int audio channel
@int mic volume, percentage, 1%~100%, default 100%, that is, no adjustment
@return int current mic volume
@usage
local result = audio.vol(0, 90) --The volume of channel 0 is adjusted to 90%. The result stores the adjusted volume level, which may still be 100.*/
static int l_audio_mic_vol(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int mic_vol = luaL_optinteger(L, 2, 100);
    lua_pushinteger(L, luat_audio_mic_vol(id, mic_vol));
    return 1;
}

/*Configure the hardware output bus of an audio channel. The corresponding type can only be set if the corresponding SOC software and hardware platform supports it.
@api audio.setBus(id, bus_type)
@int audio channel, for example 0
@int bus type, such as audio.BUS_SOFT_DAC
@int hardware id, for example, when the bus type is audio.BUS_I2S, the hardware id is the i2c id of the i2s codec
@return nil no return value
@usage
audio.setBus(0, audio.BUS_SOFT_DAC) --The hardware output channel of channel 0 is set to the software DAC
audio.setBus(0, audio.BUS_I2S) --The hardware output channel of channel 0 is set to I2S*/
static int l_audio_set_output_bus(lua_State *L) {
    size_t len;
    int id = luaL_checkinteger(L, 1);
    luat_audio_conf_t* audio_conf = luat_audio_get_config(id);
    int tp = luaL_checkinteger(L, 2);
    int ret = luat_audio_set_bus_type(id,tp);
    if (audio_conf!=NULL && lua_istable(L,3) && tp==LUAT_AUDIO_BUS_I2S){
        audio_conf->codec_conf.multimedia_id = id;
        audio_conf->bus_type = LUAT_AUDIO_BUS_I2S;
        audio_conf->codec_conf.codec_opts = &codec_opts_common;
		lua_pushstring(L, "chip");
		if (LUA_TSTRING == lua_gettable(L, 3)) {
            const char *chip = luaL_checklstring(L, -1,&len);
            if(strcmp(chip,"es8311") == 0){
                audio_conf->codec_conf.codec_opts = &codec_opts_es8311;
            }
		}
		lua_pop(L, 1);
		lua_pushstring(L, "i2cid");
		if (LUA_TNUMBER == lua_gettable(L, 3)) {
			audio_conf->codec_conf.i2c_id = luaL_checknumber(L, -1);
		}
		lua_pop(L, 1);
		lua_pushstring(L, "i2sid");
		if (LUA_TNUMBER == lua_gettable(L, 3)) {
			audio_conf->codec_conf.i2s_id = luaL_checknumber(L, -1);
		}
		lua_pop(L, 1);
		lua_pushstring(L, "voltage");
		if (LUA_TNUMBER == lua_gettable(L, 3)) {
			audio_conf->voltage = luaL_checknumber(L, -1);
		}
		lua_pop(L, 1);
    }
    ret |= luat_audio_init(id, 0, 0);
    lua_pushboolean(L, !ret);
    return 1;
}

LUAT_WEAK void luat_audio_set_debug(uint8_t on_off)
{
	(void)on_off;
}
/*Configure debugging information output
@api audio.debug(on_off)
@boolean true on false off
@return
@usage
audio.debug(true) --enable debugging information output
audio.debug(false) --Turn off debugging information output*/
static int l_audio_set_debug(lua_State *L) {
	luat_audio_set_debug(lua_toboolean(L, 1));
    return 0;
}

/*audio sleep control (usually called automatically and does not need to be executed manually)
@api audio.pm(id,pm_mode)
@int audio channel
@int sleep mode
@return boolean true success
@usage
audio.pm(multimedia_id,audio.RESUME)*/
static int l_audio_pm_request(lua_State *L) {
    lua_pushboolean(L, !luat_audio_pm_request(luaL_checkinteger(L, 1),luaL_checkinteger(L, 2)));
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_audio[] =
{
    { "start" ,        ROREG_FUNC(l_audio_start_raw)},
    { "write" ,        ROREG_FUNC(l_audio_write_raw)},
    { "pause",         ROREG_FUNC(l_audio_pause_raw)},
	{ "stop",		   ROREG_FUNC(l_audio_stop_raw)},
    { "on",            ROREG_FUNC(l_audio_raw_on)},
	{ "play",		   ROREG_FUNC(l_audio_play)},
#ifdef LUAT_USE_TTS
	{ "tts",		   ROREG_FUNC(l_audio_play_tts)},
#endif
	{ "playStop",	   ROREG_FUNC(l_audio_play_stop)},
	{ "isEnd",		   ROREG_FUNC(l_audio_play_wait_end)},
	{ "config",			ROREG_FUNC(l_audio_config)},
	{ "vol",			ROREG_FUNC(l_audio_vol)},
    { "micVol",			ROREG_FUNC(l_audio_mic_vol)},
	{ "getError",		ROREG_FUNC(l_audio_play_get_last_error)},
	{ "setBus",			ROREG_FUNC(l_audio_set_output_bus)},
	{ "debug",			ROREG_FUNC(l_audio_set_debug)},
    { "pm",			    ROREG_FUNC(l_audio_pm_request)},
#ifdef LUAT_USE_RECORD
    { "record",			ROREG_FUNC(l_audio_record)},
    { "recordStop",		ROREG_FUNC(l_audio_record_stop)},
    
#endif
	//@const RESUME number PM mode working mode
    { "RESUME",         ROREG_INT(LUAT_AUDIO_PM_RESUME)},
    //@const STANDBY number PM mode standby mode, PA is powered off, codec is in standby state, the system cannot enter low power consumption state, if PA is uncontrollable, codec enters silent mode
    { "STANDBY",        ROREG_INT(LUAT_AUDIO_PM_STANDBY)},
    //@const SHUTDOWN number PM mode shutdown mode, PA is powered off, configurable codec is powered off, non-configurable codec is powered off, the system can enter a low power consumption state
    { "SHUTDOWN",       ROREG_INT(LUAT_AUDIO_PM_SHUTDOWN)},
	//@const POWEROFF number PM mode power-off mode, PA is powered off, codec is powered off, the system can enter a low power consumption state
    { "POWEROFF",         ROREG_INT(LUAT_AUDIO_PM_POWER_OFF)},
	//@const PCM number PCM format, that is, original ADC data
    { "PCM",           ROREG_INT(LUAT_MULTIMEDIA_DATA_TYPE_PCM)},
    //@const MP3 number MP3 format
    { "MP3",           ROREG_INT(LUAT_MULTIMEDIA_DATA_TYPE_MP3)},
    //@const WAV number WAV格式
    { "WAV",           ROREG_INT(LUAT_MULTIMEDIA_DATA_TYPE_WAV)},
    //@const AMR number AMR_NB format
    { "AMR",           ROREG_INT(LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB)},
    //@const AMR_NB number AMR_NB format
    { "AMR_NB",           ROREG_INT(LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB)},
    //@const AMR_WB number AMR_WB format
    { "AMR_WB",           ROREG_INT(LUAT_MULTIMEDIA_DATA_TYPE_AMR_WB)},
	//@const MORE_DATA number The value of the parameter passed into the audio.on callback function indicates that after the bottom layer has played a piece of data, more data can be passed in
	{ "MORE_DATA",     ROREG_INT(LUAT_MULTIMEDIA_CB_AUDIO_NEED_DATA)},
	//@const DONE number The value of the parameter passed in by the audio.on callback function indicates that all the data has been played by the underlying layer.
	{ "DONE",          ROREG_INT(LUAT_MULTIMEDIA_CB_AUDIO_DONE)},
	//@const RECORD_DATA number The value of the parameter passed in by the audio.on callback function represents the recording data
	{ "RECORD_DATA",     ROREG_INT(LUAT_MULTIMEDIA_CB_RECORD_DATA)},
	//@const RECORD_DONE number audio.on callback function passes the value of the parameter, indicating that the recording is completed
	{ "RECORD_DONE",          ROREG_INT(LUAT_MULTIMEDIA_CB_RECORD_DONE)},
	//@const BUS_DAC number Hardware output bus, DAC type
	{ "BUS_DAC", 		ROREG_INT(LUAT_AUDIO_BUS_DAC)},
	//@const BUS_I2S number Hardware output bus, I2S type
	{ "BUS_I2S", 		ROREG_INT(LUAT_AUDIO_BUS_I2S)},
	//@const BUS_SOFT_DAC number Hardware output bus, software mode DAC type
	{ "BUS_SOFT_DAC", 		ROREG_INT(LUAT_AUDIO_BUS_SOFT_DAC)},
    //@const VOLTAGE_1800 number Configurable codec working voltage, 1.8V
	{ "VOLTAGE_1800", 		ROREG_INT(LUAT_AUDIO_VOLTAGE_1800)},
    //@const VOLTAGE_3300 number Configurable codec working voltage, 3.3V
	{ "VOLTAGE_3300", 		ROREG_INT(LUAT_AUDIO_VOLTAGE_3300)},
	{ NULL,            ROREG_INT(0)}
};

LUAMOD_API int luaopen_multimedia_audio( lua_State *L ) {
    luat_newlib2(L, reg_audio);
    return 1;
}
