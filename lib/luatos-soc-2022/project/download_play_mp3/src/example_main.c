/**
 * MP3 online playback function through HTTP, because the 44.1K file requires more ram space, low-speed mode compilation must be turned on! ! ! For lower code rates, you can compile without turning on the low-speed mode.
 *HTTP download is not a good streaming method, and MP3 format is not a good streaming format either
 * This demo verifies the feasibility of broadcasting while downloading
 **/
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_audio_play_ec618.h"
#include "luat_i2s_ec618.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_mobile.h"
#include "luat_network_adapter.h"
#include "networkmgr.h"
#include "luat_http.h"
//AIR780E+TM8211 development board configuration
#define CODEC_PWR_PIN HAL_GPIO_12
#define CODEC_PWR_PIN_ALT_FUN	4
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN	0
#define MP3_BUFFER_LEN_MIN	(10 * 1024)	//MP3 data must be larger than 10KB before decoding can begin, which needs to be determined based on the actual situation.
#define MP3_BUFFER_LEN_LOW	(30 * 1024)
#define MP3_BUFFER_LEN_HIGH	(40 * 1024)
#define MP3_FRAME_LEN (4 * 1152)
#define MP3_MAX_CODED_FRAME_SIZE 1792
static luat_http_ctrl_t *g_s_http_client;
static luat_rtos_task_handle g_s_task_handle;
static HANDLE g_s_delay_timer;
static Buffer_Struct g_s_mp3_buffer = {0};
static Buffer_Struct g_s_pcm_buffer = {0};
static uint32_t g_s_mp3_downloading;
static uint32_t g_s_mp3_data_tx;	//Data sent to the audio thread
static uint32_t g_s_mp3_data_rx;	//The data received by audio, the difference is the cached data
static uint8_t g_s_mp3_error;
static uint8_t g_s_mp3_wait_start;
static uint8_t g_s_mp3_pause;
static uint8_t g_s_mp3_data_mode;
static void *g_s_mp3_decoder;	//You can free it when not in use. This demo is played in a loop and is not free.
static uint16_t g_s_mp3_dummy;
enum
{
	AUDIO_NEED_DATA = 1,
	MP3_HTTP_GET_HEAD_DONE,
	MP3_HTTP_GET_DATA,
	MP3_HTTP_GET_DATA_DONE,
	MP3_HTTP_FAILED,

};
int run_mp3_play(uint8_t is_start);

void app_pa_on(uint32_t arg)
{
	luat_gpio_set(PA_PWR_PIN, 1);
}
void run_mp3_add_blank(uint8_t *data, uint32_t len)
{
	int16_t *dummy_data = malloc(8192);
	for(int i = 0; i < 4096; i++)
	{
		dummy_data[i] = g_s_mp3_dummy;
	}
	luat_audio_play_write_raw(0, dummy_data, 8192);
	luat_audio_play_write_raw(0, dummy_data, 8192);
	free(dummy_data);
}
void run_mp3_decode(uint8_t *data, uint32_t len)
{
	if (data)
	{
		OS_BufferWrite(&g_s_mp3_buffer, data, len);
		g_s_mp3_data_rx += len;
		free(data);
		if (g_s_mp3_wait_start)
		{
			if (g_s_mp3_buffer.Pos >= MP3_BUFFER_LEN_MIN)
			{
				g_s_mp3_wait_start = 0;
				run_mp3_play(1);
			}
			return;
		}
		if (g_s_mp3_pause)
		{
			if ((g_s_mp3_buffer.Pos >= MP3_BUFFER_LEN_MIN) || !g_s_mp3_downloading)
			{
				g_s_mp3_pause = 0;
				run_mp3_play(0);
			}
			return;
		}
	}
	run_mp3_play(0);
	if (g_s_mp3_buffer.Pos < MP3_BUFFER_LEN_LOW)
	{
		if (g_s_http_client->is_pause)
		{
			luat_http_client_pause(g_s_http_client, 0);
		}
	}
	Audio_StreamStruct *stream = (Audio_StreamStruct *)luat_audio_play_get_stream(0);
	if (g_s_mp3_downloading && (llist_num(&stream->DataHead) <= 1))
	{
		LUAT_DEBUG_PRINT("no data %d, %d", g_s_mp3_downloading, llist_num(&stream->DataHead));
		run_mp3_add_blank(NULL, 0);
		return;
	}

}

void audio_event_cb(uint32_t event, void *param)
{

//	PadConfig_t pad_config;
//	GpioPinConfig_t gpio_config;
//	LUAT_DEBUG_PRINT("%d", event);
	switch(event)
	{
	case LUAT_MULTIMEDIA_CB_AUDIO_NEED_DATA:
		soc_call_function_in_audio(run_mp3_decode, NULL, 0, LUAT_WAIT_FOREVER);
		break;
	case LUAT_MULTIMEDIA_CB_AUDIO_DONE:
		if (g_s_mp3_downloading)
		{
			LUAT_DEBUG_PRINT("pause");
			g_s_mp3_pause = 1;
			soc_call_function_in_audio(run_mp3_add_blank, NULL, 0, LUAT_WAIT_FOREVER);
			return;
		}
		luat_audio_play_stop_raw(0);
		luat_rtos_timer_stop(g_s_delay_timer);
		luat_gpio_set(PA_PWR_PIN, 0);
		luat_gpio_set(CODEC_PWR_PIN, 0);
		break;
	}
}

void audio_data_cb(uint8_t *data, uint32_t len, uint8_t bits, uint8_t channels)
{
	//Here you can perform software volume scaling on the audio data, or clear it directly to mute it.
	//Software volume scaling reference HAL_I2sSrcAdjustVolumn
	//LUAT_DEBUG_PRINT("%x,%d,%d,%d", data, len, bits, channels);
}


int run_mp3_play(uint8_t is_start)
{
	Audio_StreamStruct *stream = (Audio_StreamStruct *)luat_audio_play_get_stream(0);
	uint8_t num_channels = 1;
	uint32_t sample_rate = 0;
	int result, len;
	if (!g_s_mp3_decoder)
	{
		g_s_mp3_decoder = mp3_decoder_create();
	}
	if (is_start)
	{
		mp3_decoder_init(g_s_mp3_decoder);
		result = mp3_decoder_get_info(g_s_mp3_decoder, g_s_mp3_buffer.Data, g_s_mp3_buffer.Pos, &sample_rate, &num_channels);
		if (result)
		{
			mp3_decoder_init(g_s_mp3_decoder);
			LUAT_DEBUG_PRINT("mp3 %d,%d", sample_rate, num_channels);
			len = (num_channels * sample_rate >> 2) + MP3_FRAME_LEN * 2;
			OS_ReInitBuffer(&g_s_pcm_buffer, len);
		}
		else
		{
			LUAT_DEBUG_PRINT("mp3 decode fail!");
			g_s_mp3_error = 1;
			return -1;
		}
	}
	uint32_t pos = 0;
	uint32_t out_len, hz, used;
	if (!g_s_mp3_buffer.Pos)
	{
		return -1;
	}
	while ((llist_num(&stream->DataHead) < 3) && (g_s_mp3_buffer.Pos > (MP3_MAX_CODED_FRAME_SIZE * g_s_mp3_downloading + 1)) )
	{
		while (( g_s_pcm_buffer.Pos < (g_s_pcm_buffer.MaxLen - MP3_FRAME_LEN * 2) ) && (g_s_mp3_buffer.Pos > (MP3_MAX_CODED_FRAME_SIZE * g_s_mp3_downloading + 1)))
		{
			pos = 0;
			do
			{
				result = mp3_decoder_get_data(g_s_mp3_decoder, g_s_mp3_buffer.Data + pos, g_s_mp3_buffer.Pos - pos,
						(int16_t *)&g_s_pcm_buffer.Data[g_s_pcm_buffer.Pos], &out_len, &hz, &used);
				luat_wdt_feed();
				if (result > 0)
				{
					g_s_pcm_buffer.Pos += out_len;
					//Record the last sample data and use it to add blank sound when pause
					memcpy(&g_s_mp3_dummy, g_s_pcm_buffer.Data[g_s_pcm_buffer.Pos - 2], 2);
				}
				pos += used;

				if (!result || (g_s_pcm_buffer.Pos >= (g_s_pcm_buffer.MaxLen - MP3_FRAME_LEN * 2)))
				{
					break;
				}
			} while ( ((g_s_mp3_buffer.Pos - pos) >= (MP3_MAX_CODED_FRAME_SIZE * g_s_mp3_downloading + 1)));
			OS_BufferRemove(&g_s_mp3_buffer, pos);
		}
		if (is_start)
		{
			luat_audio_play_start_raw(0, AUSTREAM_FORMAT_PCM, num_channels, sample_rate, 16, 1);
			//Open the external DAC. Since it needs to cooperate with the startup of the PA, a blank sound needs to be played.
			luat_gpio_set(CODEC_PWR_PIN, 1);
			luat_audio_play_write_blank_raw(0, 6, 1);
			is_start = 0;
			luat_rtos_timer_start(g_s_delay_timer, 200, 0, app_pa_on, NULL);
		}
		luat_audio_play_write_raw(0, g_s_pcm_buffer.Data, g_s_pcm_buffer.Pos);
		g_s_pcm_buffer.Pos = 0;
	}
}

static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	if (LUAT_MOBILE_EVENT_NETIF == event)
	{
		if (LUAT_MOBILE_NETIF_LINK_ON == status)
		{
			luat_socket_check_ready(index, NULL);
		}
	}
}

static void luatos_http_cb(int status, void *data, uint32_t len, void *param)
{
	uint8_t *mp3_data;
	if (status < 0)
	{
		LUAT_DEBUG_PRINT("http failed! %d", status);
		luat_rtos_event_send(param, MP3_HTTP_FAILED, 0, 0, 0, 0);
		return;
	}
	uint32_t start, end, total;
	switch(status)
	{
	case HTTP_STATE_GET_BODY:
		if (data)
		{
			mp3_data = malloc(len);
			memcpy(mp3_data, data, len);
			if (!g_s_mp3_data_mode)
			{
				luat_rtos_event_send(param, MP3_HTTP_GET_DATA, mp3_data, len, 0, 0);
			}
			else
			{
				g_s_mp3_data_tx += len;
				soc_call_function_in_audio(run_mp3_decode, mp3_data, len, LUAT_WAIT_FOREVER);
				if ((g_s_mp3_data_tx - g_s_mp3_data_rx + g_s_mp3_buffer.Pos) >= MP3_BUFFER_LEN_HIGH)
				{
//					LUAT_DEBUG_PRINT("%u,%u,%u", g_s_mp3_data_tx, g_s_mp3_data_rx, g_s_mp3_buffer.Pos);
					if (!g_s_http_client->is_pause)
					{
						luat_http_client_pause(g_s_http_client, 1);
					}
				}
			}
		}
		else
		{
			luat_rtos_event_send(param, MP3_HTTP_GET_DATA_DONE, 0, 0, 0, 0);
		}
		break;
	case HTTP_STATE_GET_HEAD:
		if (data)
		{
			LUAT_DEBUG_PRINT("%s", data);
		}
		else
		{
			luat_rtos_event_send(param, MP3_HTTP_GET_HEAD_DONE, 0, 0, 0, 0);
		}
		break;
	case HTTP_STATE_IDLE:
		break;
	case HTTP_STATE_SEND_BODY_START:
		//If it is POST, send the POST body data here. If it cannot be sent completely at one time, you can continue to send it in the HTTP_STATE_SEND_BODY callback.
		break;
	case HTTP_STATE_SEND_BODY:
		//If it is POST, you can send the remaining body data of POST here
		break;
	default:
		break;
	}
}

static void luat_test_task(void *param)
{
	luat_event_t event;
	/*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! ! 1*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET); 
	uint32_t all,now_free_block,min_free_block;
	luat_rtos_timer_create(&g_s_delay_timer);
	luat_audio_play_global_init_with_task_priority(audio_event_cb, audio_data_cb, NULL, NULL, NULL, 90);
//	Audio_StreamStruct *stream = (Audio_StreamStruct *)luat_audio_play_get_stream(0);
// The following configuration can use TM8211
    luat_i2s_base_setup(0, I2S_MODE_MSB, I2S_FRAME_SIZE_16_16);

    g_s_http_client = luat_http_client_create(luatos_http_cb, luat_rtos_get_current_handle(), -1);
	const char remote_domain[] = "http://www.air32.cn/test_44K.mp3";
    //const char remote_domain[] = "https://cloridge-hardware.oss-cn-shanghai.aliyuncs.com/music/test1.mp3";
	uint8_t *mp3_data;
	uint32_t start, i;
	uint8_t get_mp3_head = 0;
	uint8_t is_error;
	uint8_t mp3_head_data[12];
	uint8_t mp3_head_len = 0;
	luat_http_client_base_config(g_s_http_client, 5000, 0, 3);
	//https download additionally opens ssl configuration
	//luat_http_client_ssl_config(g_s_http_client, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
	g_s_http_client->debug_onoff = 1;
	while(1)
	{
		luat_meminfo_sys(&all, &now_free_block, &min_free_block);
		LUAT_DEBUG_PRINT("meminfo %d,%d,%d",all,now_free_block,min_free_block);
		g_s_mp3_data_mode = 0;
		get_mp3_head = 0;
		g_s_mp3_error = 0;
		g_s_mp3_data_tx = 0;
		g_s_mp3_data_rx = 0;
		g_s_mp3_wait_start = 0;
		is_error = 0;
		luat_http_client_clear(g_s_http_client);
		luat_http_client_set_user_head(g_s_http_client, "Range", "bytes=0-11");
		luat_http_client_start(g_s_http_client, remote_domain, 0, 0, 1);
		while (!get_mp3_head)
		{
			luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
			switch(event.id)
			{
			case MP3_HTTP_GET_HEAD_DONE:
				LUAT_DEBUG_PRINT("status %d total %u", luat_http_client_get_status_code(g_s_http_client), g_s_http_client->total_len);
				break;
			case MP3_HTTP_GET_DATA:
				if (event.param2 <= 12)
				{
					memcpy(mp3_head_data + get_mp3_head, event.param1, event.param2);
					mp3_head_len += event.param2;
				}
				break;
			case MP3_HTTP_GET_DATA_DONE:
				get_mp3_head = 1;
				break;
			case MP3_HTTP_FAILED:
				get_mp3_head = 1;
				mp3_head_len = 0;
				break;
			}
		}
		luat_http_client_close(g_s_http_client);
		luat_http_client_clear(g_s_http_client);
		if (get_mp3_head && mp3_head_len >= 12)
		{
			if (!memcmp(mp3_head_data, "ID3", 3) || (mp3_head_data[0] == 0xff))
			{
				start = 0;
				if (mp3_head_data[0] != 0xff)
				{
					//Skip useless data
					for(i = 0; i < 4; i++)
					{
						start <<= 7;
						start |= mp3_head_data[6 + i] & 0x7f;
					}
				}
			}
			else
			{
				LUAT_DEBUG_PRINT("Not an MP3 file, exit");
				LUAT_DEBUG_PRINT("%.*s", mp3_head_len, mp3_head_data);
				goto MP3_DOWNLOAD_END;
			}
		}
		else
		{
			LUAT_DEBUG_PRINT("Failed to download MP3 file header");
			goto MP3_DOWNLOAD_END;
		}
		luat_audio_play_set_user_lock(0, 1);
		g_s_mp3_data_mode = 1;
		luat_http_client_set_get_offset(g_s_http_client, start);
		luat_http_client_start(g_s_http_client, remote_domain, 0, 0, 1);
		g_s_mp3_downloading = 1;
		g_s_mp3_wait_start = 1;
		while(g_s_mp3_downloading)
		{
			luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
			switch(event.id)
			{
			case MP3_HTTP_GET_HEAD_DONE:
				LUAT_DEBUG_PRINT("status %d total %u", luat_http_client_get_status_code(g_s_http_client), g_s_http_client->total_len);
				if (luat_http_client_get_status_code(g_s_http_client) != 206)
				{
					g_s_mp3_downloading = 0;
					is_error = 1;
				}
				break;
			case MP3_HTTP_GET_DATA:
				break;
			case MP3_HTTP_GET_DATA_DONE:
				g_s_mp3_downloading = 0;
				break;
			case MP3_HTTP_FAILED:
				g_s_mp3_downloading = 0;
				is_error = 1;
				break;
			}
		}
MP3_DOWNLOAD_END:
		luat_http_client_close(g_s_http_client);
		g_s_mp3_downloading = 0;
		luat_audio_play_set_user_lock(0, 0);
		LUAT_DEBUG_PRINT("This MP3 download has ended, please try again in 60 seconds.");
		if (is_error || g_s_mp3_error)
		{
			LUAT_DEBUG_PRINT("An error occurred while playing MP3 this time, and playback needs to be closed.");
			luat_audio_play_stop_raw(0);
			luat_rtos_timer_stop(g_s_delay_timer);
			luat_gpio_set(PA_PWR_PIN, 0);
			luat_gpio_set(CODEC_PWR_PIN, 0);
		}

		OS_DeInitBuffer(&g_s_mp3_buffer);
		luat_meminfo_sys(&all, &now_free_block, &min_free_block);
		LUAT_DEBUG_PRINT("meminfo %d,%d,%d",all,now_free_block,min_free_block);
		luat_rtos_task_sleep(60000);
	}
}

static void luat_test_init(void)
{
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pull = LUAT_GPIO_DEFAULT;
	gpio_cfg.pin = PA_PWR_PIN;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = CODEC_PWR_PIN;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);
	luat_mobile_event_register_handler(luatos_mobile_event_callback);
	net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
	luat_rtos_task_create(&g_s_task_handle, 4 * 1024, 50, "test", luat_test_task, NULL, 16);

}

INIT_TASK_EXPORT(luat_test_init, "1");