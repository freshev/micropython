/**************************************************** ***************************
 * multimedia codec abstraction layer
 *************************************************** ****************************/
#ifndef __LUAT_MULTIMEDIA_CODEC_H__
#define __LUAT_MULTIMEDIA_CODEC_H__

#include "luat_base.h"

#ifdef __LUATOS__
#include "luat_zbuff.h"

#define LUAT_M_CODE_TYPE "MCODER*"

#endif

#define MP3_FRAME_LEN 4 * 1152

#define MAX_DEVICE_COUNT 2
#define MP3_MAX_CODED_FRAME_SIZE 1792

enum{
	LUAT_MULTIMEDIA_DATA_TYPE_NONE,
	LUAT_MULTIMEDIA_DATA_TYPE_PCM,
	LUAT_MULTIMEDIA_DATA_TYPE_MP3,
	LUAT_MULTIMEDIA_DATA_TYPE_WAV,
	LUAT_MULTIMEDIA_DATA_TYPE_AMR_NB,
	LUAT_MULTIMEDIA_DATA_TYPE_AMR_WB,
};

enum{
	LUAT_MULTIMEDIA_CB_AUDIO_DECODE_START,	//Start decoding the file
	LUAT_MULTIMEDIA_CB_AUDIO_OUTPUT_START,	//Start outputting the decoded audio data
	LUAT_MULTIMEDIA_CB_AUDIO_NEED_DATA,		//The underlying driver has finished playing part of the data and needs more data.
	LUAT_MULTIMEDIA_CB_AUDIO_DONE,			//The underlying driver has played all the data
	LUAT_MULTIMEDIA_CB_DECODE_DONE,			//Audio decoding completed
	LUAT_MULTIMEDIA_CB_TTS_INIT,			//TTS has completed the necessary initialization, and users can make personalized configuration through audio_play_tts_set_param
	LUAT_MULTIMEDIA_CB_TTS_DONE,			//TTS encoding is completed. Note that playback is not completed
	LUAT_MULTIMEDIA_CB_RECORD_DATA,			//recording data
	LUAT_MULTIMEDIA_CB_RECORD_DONE,			//Recording completed
};

#include <stddef.h>
#include <stdio.h>

#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif

typedef struct{
	union{
		void *mp3_decoder;
		uint32_t read_len;
		void *amr_coder;
	};
	FILE* fd;
#ifdef __LUATOS__
	luat_zbuff_t buff;
#endif
	Buffer_Struct file_data_buffer;
	Buffer_Struct audio_data_buffer;
	uint8_t type;
	uint8_t is_decoder;
}luat_multimedia_codec_t;

typedef struct luat_multimedia_cb {
    int function_ref;
} luat_multimedia_cb_t;

void *mp3_decoder_create(void);
void mp3_decoder_init(void *decoder);
void mp3_decoder_set_debug(void *decoder, uint8_t onoff);
int mp3_decoder_get_info(void *decoder, const uint8_t *input, uint32_t len, uint32_t *hz, uint8_t *channel);
int mp3_decoder_get_data(void *decoder, const uint8_t *input, uint32_t len, int16_t *pcm, uint32_t *out_len, uint32_t *hz, uint32_t *used);

#ifdef __LUATOS__
int l_multimedia_raw_handler(lua_State *L, void* ptr);
#endif

#endif
