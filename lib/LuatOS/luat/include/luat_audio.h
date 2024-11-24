/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __LUAT_AUDIO_H__
#define __LUAT_AUDIO_H__
#include "luat_base.h"

#include "luat_rtos.h"
#include"luat_audio_codec.h"
#include"luat_multimedia_codec.h"

#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif

typedef struct luat_audio_conf {
    luat_multimedia_codec_t multimedia_codec;
	uint64_t last_wakeup_time_ms;
	luat_audio_codec_conf_t codec_conf;
	void *hardware_data;
	luat_rtos_timer_t pa_delay_timer;
    uint32_t after_sleep_ready_time;                                    //
    uint16_t pa_delay_time;
	uint16_t power_off_delay_time;                                      //Delay time after power off
	uint16_t soft_vol;
    uint16_t speech_downlink_type;
    uint16_t speech_uplink_type;
    uint16_t i2s_rx_cb_save;
	uint16_t last_vol;
	uint16_t last_mic_vol;
    uint8_t bus_type;
    uint8_t raw_mode;
    uint8_t debug_on_off;
    uint8_t sleep_mode;
    uint8_t wakeup_ready;
    uint8_t pa_on_enable;
    uint8_t record_mode;
    uint8_t pa_pin;                                                     // pa pin
	uint8_t pa_on_level;                                                // pa enable level
	uint8_t power_pin;													// power control
	uint8_t power_on_level;                                             //Power enable level
	uint8_t pa_is_control_enable;
	uint8_t voltage;
} luat_audio_conf_t;

typedef enum{
    LUAT_AUDIO_PM_RESUME = 0,       /*working mode*/
    LUAT_AUDIO_PM_STANDBY,          /*standby mode*/
    LUAT_AUDIO_PM_SHUTDOWN,         /*shutdown mode*/
	LUAT_AUDIO_PM_POWER_OFF,        /*Complete power down mode*/
}luat_audio_pm_mode_t;
typedef enum{
    LUAT_AUDIO_VOLTAGE_3300 = 0,         	 /*Works at 3.3V*/
	LUAT_AUDIO_VOLTAGE_1800,       			 /*Works at 1.8V*/
}luat_audio_voltage_t;

typedef enum{
	LUAT_AUDIO_BUS_DAC=0,
	LUAT_AUDIO_BUS_I2S,
	LUAT_AUDIO_BUS_SOFT_DAC
}luat_audio_bus_type_t;

#ifdef LUAT_USE_RECORD
#include "luat_i2s.h"
typedef struct{
//	luat_rtos_task_handle task_handle;
	FILE* fd;
    uint8_t multimedia_id;
	uint8_t quailty;
	uint8_t type;
	uint8_t is_run;
	uint32_t record_time;
	uint32_t record_time_tmp;
	void* encoder_handler;
	luat_zbuff_t * record_buffer[2];
	int record_buffer_index;
	int zbuff_ref[2];
    uint32_t bak_sample_rate;                                   // i2s sampling rate
    uint32_t bak_cb_rx_len;                                     //Receive trigger callback data length
    int (*bak_luat_i2s_event_callback)(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param); // i2s callback function
    uint8_t bak_is_full_duplex;		                            // Whether full duplex
}luat_record_ctrl_t;

#endif

/**
 * @brief Set the audio hardware output type. Subsequent initialization will be processed differently according to the type, so you must first use this function to set the type!!!
 *
 * @param bus_type See MULTIMEDIA_AUDIO_BUS, currently only 0=DAC 1=I2S 2=SOFT_DAC*/
int luat_audio_set_bus_type(uint8_t multimedia_id,uint8_t bus_type);

//This function can obtain the audio structure corresponding to multimedia_id for dynamic modification. If there are functions that cannot be set directly, you can modify the structure by yourself through this method. Generally, there is no need to use this function.
luat_audio_conf_t *luat_audio_get_config(uint8_t multimedia_id);

/**
 * @brief audio and codec binding
 *
 * @param multimedia_id multimedia channel, currently only 0
 * @param codec_conf codec information
 * @return int =0 success, others failure*/
int luat_audio_setup_codec(uint8_t multimedia_id, const luat_audio_codec_conf_t *codec_conf);

/**
 * @brief initialize audio
 *
 * @param multimedia_id multimedia channel, currently only 0
 * @param init_vol default hardware volume
 * @param init_mic_vol default MIC volume
 * @return int =0 success, others failure*/
int luat_audio_init(uint8_t multimedia_id, uint16_t init_vol, uint16_t init_mic_vol);

/**
 * @brief audio sleep control, note that the power consumption in each mode of pm is determined by the specific audio hardware
 *
 * @param multimedia_id multimedia channel
 * @param mode
 * @return int =0 success, others failure*/
int luat_audio_pm_request(uint8_t multimedia_id,luat_audio_pm_mode_t mode);

/**
 * @brief plays blank sound, generally does not need to be called actively
 *
 * @param multimedia_id multimedia channel
 * @param on_off 1 turns on blank, 0 turns off
 * @return int =0 success, others failure*/
int luat_audio_play_blank(uint8_t multimedia_id, uint8_t on_off);

#ifdef __LUATOS__
/**
 * @brief Play the specified number of files or ROM array (file data is directly written in array form)
 *
 * @param multimedia_id multimedia channel, currently only 0
 * @param info file information, file path information
 * @param files_num number of files
 * @return int =0 success, others failure*/
int luat_audio_play_multi_files(uint8_t multimedia_id, uData_t *info, uint32_t files_num, uint8_t error_stop);
#endif
/**
 * @brief Play the specified file or
 *
 * @param multimedia_id multimedia channel, currently only 0
 * @param path file path
 * @return int =0 success, others failure*/
int luat_audio_play_file(uint8_t multimedia_id, const char *path);
/**
 * @brief Whether to play all data
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return uint8_t =1 yes, =0 no*/
uint8_t luat_audio_is_finish(uint8_t multimedia_id);

/**
 * @brief Forces to stop playing the file, but will not stop the playback of data that has been output to the underlying driver
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int luat_audio_play_stop(uint8_t multimedia_id);

/**
 * @brief Get the last playback result, best to call during MULTIMEDIA_CB_AUDIO_DONE callback
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 Complete playback is completed, <0 was stopped by the user, >0 TTS failed, or the decoding of the audio file failed (the user continued after the decoding failed when play_info was not set, the file position +1)*/
int luat_audio_play_get_last_error(uint8_t multimedia_id);


/**
 * @brief Immediately initializes the playback of unencoded original audio data stream
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param audio_format audio data format, currently only supports PCM, which requires manual decoding
 * @param num_channels Number of channels, currently only 1 or 2
 * @param sample_rate sampling rate, note that only 8K, 16K, 32K, 48K, 96K, 22.05K, 44.1K can be supported
 * @param bits_per_sample quantization bit, can only be 16
 * @param is_signed Whether the quantified data is signed, it can only be 1
 * @return int =0 success, others failure*/
int luat_audio_start_raw(uint8_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed);
/**
 * @brief passes a piece of original audio data to the underlying driver
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param data original audio data
 * @param len original audio data length
 * @return int =0 success, others failure*/
int luat_audio_write_raw(uint8_t multimedia_id, uint8_t *data, uint32_t len);
/**
 * @brief forces all playback to stop, and the underlying driver will also stop output. Do not use it to end the playback file.
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int luat_audio_stop_raw(uint8_t multimedia_id);
/**
 * @brief pause/resume playback
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param is_pause 0 resumes, others pause
 * @return int =0 success, others failure*/
int luat_audio_pause_raw(uint8_t multimedia_id, uint8_t is_pause);

/**
 * @brief Encode and play a piece of text
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param text text data
 * @param text_bytes text data length
 * @return int =0 success, others failure*/
int luat_audio_play_tts_text(uint8_t multimedia_id, void *text, uint32_t text_bytes);
/**
 * @brief When receiving the MULTIMEDIA_CB_TTS_INIT callback, you can set the TTS parameters, which is equivalent to ivTTS_SetParam
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param param_id see ivTTS_PARAM_XXX
 * @param param_value The value corresponding to param_id
 * @return int =0 success, others failure*/
int luat_audio_play_tts_set_param(uint8_t multimedia_id, uint32_t param_id, uint32_t param_value);

/**
 * @brief pa pin configuration
 *
 * @param multimedia_id multimedia_id multimedia channel
 * @param pin pa pin
 * @param level pa enable level
 * @param dummy_time_len
 * @param pa_delay_time*/
void luat_audio_config_pa(uint8_t multimedia_id, uint32_t pin, int level, uint32_t dummy_time_len, uint32_t pa_delay_time);

/**
 * @brief power pin configuration
 *
 * @param multimedia_id multimedia channel
 * @param pin power pin
 * @param level enable level
 * @param dac_off_delay_time*/
void luat_audio_config_dac(uint8_t multimedia_id, int pin, int level, uint32_t dac_off_delay_time);

/**
 * @brief volume control
 *
 * @param multimedia_id multimedia channel
 * @param vol volume, 0-1000 0-100 is hardware, 100-1000 is software scaling, actual implementation is based on different bsp and different hardware underlying
 * @return uint16_t volume, 0-1000*/
uint16_t luat_audio_vol(uint8_t multimedia_id, uint16_t vol);

/**
 * @brief mic volume control
 *
 * @param multimedia_id multimedia channel
 * @param vol volume, 0-100
 * @return uint8_t volume 0-100*/
uint8_t luat_audio_mic_vol(uint8_t multimedia_id, uint16_t vol);

/**
 * @brief mute
 *
 * @param multimedia_id multimedia channel
 * @param on 1 mute, 0 unmute
 * @return uint8_t 1 mute, 0 cancel mute*/
uint8_t luat_audio_mute(uint8_t multimedia_id, uint8_t on);

/**
 * @brief audio debugging switch
 *
 * @param multimedia_id multimedia channel
 * @param onoff 0 is off, 1 is on*/
void luat_audio_play_debug_onoff(uint8_t multimedia_id, uint8_t onoff);

/**
 * @brief Check whether the audio is ready
 *
 * @param multimedia_id multimedia channel
 * @return int -1 not ready, 0 ready*/
int luat_audio_check_ready(uint8_t multimedia_id);

/**
 * @brief record and play
 *
 * @param multimedia_id multimedia channel
 * @param sample_rate sampling rate
 * @param play_buffer buffer
 * @param one_trunk_len one transmission length
 * @param total_trunk_cnt number of transmissions
 * @return int Returns 0 on success, -1 on failure*/
int luat_audio_record_and_play(uint8_t multimedia_id, uint32_t sample_rate, const uint8_t *play_buffer, uint32_t one_trunk_len, uint32_t total_trunk_cnt);

/**
 * @brief recording stops
 *
 * @param multimedia_id multimedia channel
 * @return int Returns 0 on success, -1 on failure*/
int luat_audio_record_stop(uint8_t multimedia_id);

/**
 * @brief Start call output
 *
 * @param multimedia_id multimedia channel
 * @param is_downlink whether the call is connected
 * @param type type
 * @param downlink_buffer buffer
 * @param buffer_len buffer length
 * @param channel_num channel
 * @return int Returns 0 on success, -1 on failure*/
int luat_audio_speech(uint8_t multimedia_id, uint8_t is_downlink, uint8_t type, const uint8_t *downlink_buffer, uint32_t buffer_len, uint8_t channel_num);

/**
 * @brief call output stops
 *
 * @param multimedia_id multimedia channel
 * @return int Returns 0 on success, -1 on failure*/
int luat_audio_speech_stop(uint8_t multimedia_id);

/**
 * @brief pa control function, generally no need to use it, the bottom layer will automatically call it
 *
 * @param multimedia_id multimedia_id multimedia channel
 * @param on 1 on, 0 off
 * @param delay delay time, non-blocking, write 0 without delay*/
void luat_audio_pa(uint8_t multimedia_id,uint8_t on, uint32_t delay);

/**
 * @brief power control function, generally does not need to be used, the bottom layer will automatically call it
 *
 * @param multimedia_id multimedia channel
 * @param on 1 on, 0 off*/
void luat_audio_power(uint8_t multimedia_id,uint8_t on);
/**
 * @brief power maintains control. Some GPIOs are powered off when sleeping on some platforms, so it is necessary to control whether to enter sleep.
 *
 * @param on_off 1 is maintained, 0 is not maintained*/
void luat_audio_power_keep_ctrl_by_bsp(uint8_t on_off);

/**
 * @brief Put the api into the audio task to run
 *
 * @param api the api that needs to be run
 * @param data api input data
 * @param len api input data length*/
void luat_audio_run_callback_in_task(void *api, uint8_t *data, uint32_t len);

void *luat_audio_inter_amr_coder_init(uint8_t is_wb, uint8_t quality);
int luat_audio_inter_amr_coder_encode(void *handle, const uint16_t *pcm_buf, uint8_t *amr_buf, uint8_t *amr_len);
int luat_audio_inter_amr_coder_decode(void *handle, uint16_t *pcm_buf, const uint8_t *amr_buf, uint8_t *amr_len);
void luat_audio_inter_amr_coder_deinit(void *handle);
#endif
