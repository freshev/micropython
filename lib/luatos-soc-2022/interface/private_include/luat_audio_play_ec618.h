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

#ifndef __LUAT_AUDIO_PLAY_EC618_H__
#define __LUAT_AUDIO_PLAY_EC618_H__
#include "common_api.h"
#include "audio_ll_drv.h"
#include "audio_play.h"
/**
 * @defgroup luatos_audio audio interface
 * @{*/
/**
 * @brief audio playback control parameters*/
typedef struct
{
	char *path;		/**< file path, if it is NULL, it means it is a ROM array*/
	uint32_t address;	/**< ROM array address*/
	uint32_t rom_data_len;	/**< ROM array length*/
	uint8_t fail_continue;	/**< If decoding fails, whether to skip and continue to the next one, if it is the last file, force stop and set error message*/
	uint8_t dummy[3];
}luat_audio_play_info_t;

enum
{
	LUAT_MULTIMEDIA_CB_AUDIO_DECODE_START = MULTIMEDIA_CB_AUDIO_DECODE_START,	/**< Start decoding the file*/
	LUAT_MULTIMEDIA_CB_AUDIO_OUTPUT_START,	/**< Start outputting decoded audio data*/
	LUAT_MULTIMEDIA_CB_AUDIO_NEED_DATA,		/**< The underlying driver has finished playing part of the data and needs more data.*/
	LUAT_MULTIMEDIA_CB_AUDIO_DONE,			/**< The underlying driver has played all the data*/
	LUAT_MULTIMEDIA_CB_DECODE_DONE,			/**< Audio decoding completed*/
	LUAT_MULTIMEDIA_CB_TTS_INIT,				/**< TTS has completed the necessary initialization, and users can make personalized configuration through audio_play_tts_set_param*/
	LUAT_MULTIMEDIA_CB_TTS_DONE,				/**< TTS encoding completed. Note that playback is not completed*/

	LUAT_AUDIO_BUS_DAC=0,
	LUAT_AUDIO_BUS_I2S,
	LUAT_AUDIO_BUS_SOFT_DAC
};

void luat_audio_play_file_default_fun(void *param);
void luat_audio_play_tts_default_fun(void *param);
/**
 * @brief Audio playback initialization, you can choose whether to enable TTS and file playback function to save flash space
 *
 * @param event_cb event callback function during playback
 * @param data_cb data decoding callback function, it will not be used if the original data stream is played directly
 * @param play_file_fun The specific processing function of playing files, select luat_audio_play_file_default_fun for processing, or write NULL to disable the function of playing files
 * @param play_tts_fun The specific processing function for playing TTS, select luat_audio_play_tts_default_fun for processing, or write NULL to disable the function of playing TTS
 * @param user_param user parameter of the callback function*/
void luat_audio_play_global_init(
		audio_play_event_cb_fun_t event_cb,
		audio_play_data_cb_fun_t data_cb,
		audio_play_default_fun_t play_file_fun,
		audio_play_default_fun_t play_tts_fun,
		void *user_param);
/**
 * @brief Audio playback initialization, you can choose whether to enable TTS, file playback function, used to save flash space, you can configure the audio task priority
 *
 * @param event_cb event callback function during playback
 * @param data_cb data decoding callback function, it will not be used if the original data stream is played directly
 * @param play_file_fun The specific processing function of playing files, select luat_audio_play_file_default_fun for processing, or write NULL to disable the function of playing files
 * @param play_tts_fun The specific processing function for playing TTS, select luat_audio_play_tts_default_fun for processing, or write NULL to disable the function of playing TTS
 * @param user_param user parameter of the callback function
 * @param priority audio task priority*/
void luat_audio_play_global_init_with_task_priority(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param, uint8_t priority);

/**
 * @brief Play the specified number of files or ROM array (file data is directly written in array form)
 *
 * @param multimedia_id multimedia channel, currently only 0
 * @param info file information, file path or ROM information
 * @param files_num number of files
 * @return int =0 success, others failure*/
int luat_audio_play_multi_files(uint8_t multimedia_id, audio_play_info_t info[], uint32_t files_num);

/**
 * @brief Whether to play all data
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return uint8_t =1 yes, =0 no*/
uint8_t luat_audio_play_is_finish(uint8_t multimedia_id);

/**
 * @brief Forces to stop playing the file, but will not stop the playback of data that has been output to the underlying driver
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int luat_audio_play_stop(uint8_t multimedia_id);
/**
 * @brief Forces to stop playing the file. If the audio decoding has been completed, it will stop the data playback that has been output to the underlying driver.
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int luat_audio_play_fast_stop(uint8_t multimedia_id);
/**
 * @brief Clear the flag of the file that is forced to stop playing, so that the next playback will not be affected when stop is accidentally operated.
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int luat_audio_play_clear_stop_flag(uint8_t multimedia_id);

/**
 * @brief pause/resume playback
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param is_pause 0 resumes, others pause
 * @return int =0 success, others failure*/
int luat_audio_play_pause_raw(uint8_t multimedia_id, uint8_t is_pause);

/**
 * @brief Get the last playback result, best to call during MULTIMEDIA_CB_AUDIO_DONE callback
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 Complete playback is completed, <0 was stopped by the user, >0 TTS failed, or the decoding of the audio file failed (the user continued after the decoding failed when play_info was not set, the file position +1)*/
int luat_audio_play_get_last_error(uint8_t multimedia_id);

/**
 * @brief Insert multiple pieces of blank data at the beginning or end, each piece of data takes about 100ms
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param cnt segment number
 * @param add_font Whether to load to the beginning position 1 yes, 0 no
 * @return int =0 success, others failure*/
int luat_audio_play_write_blank_raw(uint8_t multimedia_id, uint8_t cnt, uint8_t add_font);
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
int luat_audio_play_start_raw(uint8_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed);
/**
 * @brief passes a piece of original audio data to the underlying driver
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param data original audio data
 * @param len original audio data length
 * @return int =0 success, others failure*/
int luat_audio_play_write_raw(uint8_t multimedia_id, uint8_t *data, uint32_t len);
/**
 * @brief forces all playback to stop, and the underlying driver will also stop output. Do not use it to end the playback file.
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int luat_audio_play_stop_raw(uint8_t multimedia_id);
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
 * @brief Set the TTS resources and corresponding SDKID. There are many types of TTS resources.
 *
 * @param address the flash or ram address of the resource
 * @param sdk_id is essentially passing in AISOUND_SDK_USERID
 * @param tts_resource_read_fun function to read resource files. If it is NULL, the default function is used, which is a simple copy. If you use your own function, it must be defined according to ivCBReadResExt*/
void luat_audio_play_tts_set_resource(void *address, void *sdk_id, void *tts_resource_read_fun);
/**
 * @brief Set the audio hardware output type
 *
 * @param bus_type See LUAT_AUDIO_BUS, currently only 1=I2S 2=SOFT_DAC*/
void luat_audio_play_set_bus_type(uint8_t bus_type);
/**
 * @brief Get the stream pointer of the underlying playback
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return void *stream pointer, the real type is Audio_StreamStruct*/
void *luat_audio_play_get_stream(uint8_t multimedia_id);
/**
 * @brief controls whether the underlying playback is allowed to end
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return onoff =0 is allowed to end, others are not allowed to end*/
void luat_audio_play_set_user_lock(uint8_t multimedia_id, uint8_t onoff);

void luat_audio_play_debug_onoff(uint8_t multimedia_id, uint8_t onoff);
/**@}*/
#endif
