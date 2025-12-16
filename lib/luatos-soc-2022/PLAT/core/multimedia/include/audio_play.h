/**
 * @file audio_play.h

 * @brief audio playback layer, needs to be used with tts library, audio_decoder library and audio_ll_drv
 * @version 0.1
 * @date 2022-10-23
 *
 * @copyright
 **/

#ifndef __AUDIO_PLAY_H__
#define __AUDIO_PLAY_H__

typedef struct
{
	char *path;		//File path, if it is NULL, it means it is a ROM array
	uint32_t address;	//ROM array address
	uint32_t rom_data_len;	//ROM array length
	uint8_t fail_continue;	//If decoding fails, whether to skip and continue to the next one, if it is the last file, force stop and set error information
	uint8_t dummy[3];
}audio_play_info_t;

enum
{
	MULTIMEDIA_DATA_TYPE_NONE,
	MULTIMEDIA_DATA_TYPE_PCM,
	MULTIMEDIA_DATA_TYPE_MP3,
	MULTIMEDIA_DATA_TYPE_WAV,
	MULTIMEDIA_DATA_TYPE_AMR_NB,
	MULTIMEDIA_DATA_TYPE_AMR_WB,
};

enum
{
	MULTIMEDIA_CB_AUDIO_DECODE_START,	//Start decoding the file
	MULTIMEDIA_CB_AUDIO_OUTPUT_START,	//Start outputting the decoded audio data
	MULTIMEDIA_CB_AUDIO_NEED_DATA,		//The underlying driver has finished playing part of the data and needs more data.
	MULTIMEDIA_CB_AUDIO_DONE,			//The underlying driver has played all the data
	MULTIMEDIA_CB_DECODE_DONE,			//Audio decoding completed
	MULTIMEDIA_CB_TTS_INIT,				//TTS has completed the necessary initialization, and users can make personalized configuration through audio_play_tts_set_param
	MULTIMEDIA_CB_TTS_DONE,				//TTS encoding is completed. Note that playback is not completed
};
extern const unsigned char ivtts_8k[];
extern const unsigned char ivtts_8k_lite[];
extern const unsigned char ivtts_16k_lite[];
extern const unsigned char ivtts_16k[];
extern const unsigned char ivtts_8k_tz_data[];
extern const unsigned char ivtts_8k_tz_frags[];
extern const unsigned char ivtts_16k_tz_data[];
extern const unsigned char ivtts_16k_tz_frags[];
extern const unsigned char ivtts_8k_eng[];
extern const unsigned char ivtts_16k_eng[];
/**
 * @brief event callback during playback, see MULTIMEDIA_CB_XXX, user_param is the user_param passed in during initialization
 **/
typedef void (*audio_play_event_cb_fun_t)(uint32_t cb_audio_event, void *user_param);

/**
 * @brief When playing a file, after the data is decoded, it will be called back to the user for further processing, such as the software increasing or decreasing the volume, or muting the software. Of course, it does not need to be processed. The data length is the number of bytes, bits is the number of quantization bits, usually 16, and channels is the number of channels, 1 or 2
 **/
typedef void (*audio_play_data_cb_fun_t)(uint8_t *data, uint32_t data_len, uint8_t bits, uint8_t channels);

typedef void (*audio_play_default_fun_t)(void *param);
/**
 * @brief Audio playback initialization
 *
 * @param event_cb event callback function during playback
 * @param data_cb data decoding callback function, it will not be used if the original data stream is played directly
 * @param user_param user parameter of the callback function*/
void audio_play_global_init(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, void *user_param);

/**
 * @brief Play the specified number of files or ROM array (file data is directly written in array form)
 *
 * @param multimedia_id multimedia channel, currently only 0
 * @param info file information, file path or ROM information
 * @param files_num number of files
 * @return int =0 success, others failure*/
int audio_play_multi_files(uint32_t multimedia_id, audio_play_info_t info[], uint32_t files_num);

/**
 * @brief Whether to play all data
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return uint8_t =1 yes, =0 no*/
uint8_t audio_play_is_finish(uint32_t multimedia_id);

/**
 * @brief Forces to stop playing the file, but will not stop the playback of data that has been output to the underlying driver
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int audio_play_stop(uint32_t multimedia_id);

/**
 * @brief Forces to stop playing the file. If the audio decoding has been completed, it will stop the data playback that has been output to the underlying driver.
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int audio_play_fast_stop(uint32_t multimedia_id);
/**
 * @brief Clear the flag of the file that is forced to stop playing, so that the next playback will not be affected when stop is accidentally operated.
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int audio_play_clear_stop_flag(uint32_t multimedia_id);
/**
 * @brief pause/resume playback
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param is_pause 0 resumes, others pause
 * @return int =0 success, others failure*/
int audio_play_pause_raw(uint32_t multimedia_id, uint8_t is_pause);

/**
 * @brief Get the last playback result, best to call during MULTIMEDIA_CB_AUDIO_DONE callback
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 Complete playback is completed, <0 was stopped by the user, >0 TTS failed, or the decoding of the audio file failed (the user continued after the decoding failed when play_info was not set, the file position +1)*/
int audio_play_get_last_error(uint32_t multimedia_id);

/**
 * @brief Insert multiple pieces of blank data, each piece of data takes about 100ms
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param cnt segment number
 * @return int =0 success, others failure*/
int audio_play_write_blank_raw(uint32_t multimedia_id, uint8_t cnt);
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
int audio_play_start_raw(uint32_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed);
/**
 * @brief passes a piece of original audio data to the underlying driver
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param data original audio data
 * @param len original audio data length
 * @return int =0 success, others failure*/
int audio_play_write_raw(uint32_t multimedia_id, uint8_t *data, uint32_t len);
/**
 * @brief forces all playback to stop, and the underlying driver will also stop output. Do not use it to end the playback file.
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return int =0 success, others failure*/
int audio_play_stop_raw(uint32_t multimedia_id);
/**
 * @brief Encode and play a piece of text
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param text text data
 * @param text_bytes text data length
 * @return int =0 success, others failure*/
int audio_play_tts_text(uint32_t multimedia_id, void *text, uint32_t text_bytes);
/**
 * @brief When receiving the MULTIMEDIA_CB_TTS_INIT callback, you can set the TTS parameters, which is equivalent to ivTTS_SetParam
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @param param_id see ivTTS_PARAM_XXX
 * @param param_value The value corresponding to param_id
 * @return int =0 success, others failure*/
int audio_play_tts_set_param(uint32_t multimedia_id, uint32_t param_id, uint32_t param_value);
/**
 * @brief Set the TTS resources and corresponding SDKID. There are many types of TTS resources.
 *
 * @param address the flash or ram address of the resource
 * @param sdk_id is essentially passing in AISOUND_SDK_USERID*/
void audio_play_tts_set_resource(void *address, void *sdk_id);

/**
 * @brief Get the sampling rate of the currently playing audio
 *
 * @param multimedia_id multimedia_id multimedia channel, currently only 0
 * @return uint32_t sampling rate*/
uint32_t audio_play_get_sample_rate(uint32_t multimedia_id);

void audio_play_file_default_fun(void *param);
void audio_play_TTS_default_fun(void *param);
void audio_play_tts_set_resource_ex(void *address, void *sdk_id, void *read_resource_fun);
void audio_play_global_init_ex(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param);
int audio_play_write_blank_raw_ex(uint32_t multimedia_id, uint8_t cnt, uint8_t add_font);
void audio_play_set_bus_type(uint8_t bus_type);
void *audio_play_get_stream(uint32_t multimedia_id);
void audio_play_set_user_lock(uint32_t multimedia_id, uint8_t onoff);
#endif
