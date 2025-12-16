#include "audio_Task.h"
QueueHandle_t audioQueueHandle = NULL;
/**demo Ä¬ÈÏµÄÊÇ780E¿ª·¢°å¼ÓÒôÆµºËÐÄÐ¡°å
*Ê¹ÓÃÔÆÀ®°È¿ª·¢°åÐèÒª½« air780e_tm8211 µÄºêÖÃ1£¬Ä¬ÈÏ¹Ø±Õ
*Ê¹ÓÃÈn¼þDAC²¥·Å£¬ÐèÒª´ò¿ªsoft_dac µÄÒý½Å£¬¹Ø±Õair780e_tm8211µÄºê*/
/*-------------------------------------------------audio define-----------------------------------------------*/
static osEventFlagsId_t waitAudioPlayDone = NULL;
static HANDLE g_s_delay_timer;
#define soft_dac 0 //Ö§³ÖÍ¨¹ýDACÒý½Å²¥·Å
#define air780e_tm8211 0

/*-------------------------------------------------audio define-----------------------------------------------*/
/*------------------------------------------------audio-----------------------------------------------*/

void audio_data_cb(uint8_t *data, uint32_t len, uint8_t bits, uint8_t channels)
{
    int value = 10;
    int ret = luat_fskv_get("volume", &value, sizeof(int));
    if(ret > 0)
    {
        HAL_I2sSrcAdjustVolumn(data, len, value);
    }
    else
    {
        HAL_I2sSrcAdjustVolumn(data, len, 15);
    }
    LUAT_DEBUG_PRINT("cloud_speaker_audio_task %x,%d,%d,%d,%d", data, len, bits, channels);
}
void app_pa_on(uint32_t arg)
{
    luat_gpio_set(PA_PWR_PIN, 1);
}
void audio_event_cb(uint32_t event, void *param)
{
    LUAT_DEBUG_PRINT("%d", event);
    switch (event)
    {
    case LUAT_MULTIMEDIA_CB_AUDIO_DECODE_START:
        luat_gpio_set(CODEC_PWR_PIN, 1);
        luat_audio_play_write_blank_raw(0, 6, 1);
        break;
    case LUAT_MULTIMEDIA_CB_AUDIO_OUTPUT_START:
        if ((1 == air780e_tm8211) || (1 == soft_dac))
        {
            luat_rtos_timer_start(g_s_delay_timer, 200, 0, app_pa_on, NULL);
        }

        break;
    case LUAT_MULTIMEDIA_CB_TTS_INIT:
        audio_play_tts_set_param(0, ivTTS_PARAM_INPUT_CODEPAGE, ivTTS_CODEPAGE_GBK);
        break;
    case LUAT_MULTIMEDIA_CB_TTS_DONE:
        if (!luat_audio_play_get_last_error(0))
        {
            luat_audio_play_write_blank_raw(0, 1, 0);
        }
        break;
    case LUAT_MULTIMEDIA_CB_AUDIO_DONE:
        luat_rtos_timer_stop(g_s_delay_timer);
        LUAT_DEBUG_PRINT("audio play done, result=%d!", luat_audio_play_get_last_error(0));
        if ((1 == air780e_tm8211) || (1 == soft_dac))
        {
            luat_gpio_set(PA_PWR_PIN, 0);
        }
        luat_gpio_set(CODEC_PWR_PIN, 0);
        break;
    }
}
void audio_task(void *param)
{
    ivCStrA sdk_id = AISOUND_SDK_USERID_16K;
    luat_rtos_timer_create(&g_s_delay_timer);
    luat_audio_play_global_init(audio_event_cb, audio_data_cb, luat_audio_play_file_default_fun, luat_audio_play_tts_default_fun, NULL);
    luat_audio_play_tts_set_resource(ivtts_16k, sdk_id, NULL);
    if (1 == soft_dac)
    {
        luat_audio_play_set_bus_type(LUAT_AUDIO_BUS_SOFT_DAC);
    }
    else
    {
        if (1 == air780e_tm8211)
        {
            luat_i2s_base_setup(0, I2S_MODE_MSB, I2S_FRAME_SIZE_16_16); // ÔÆÀ®°È¿ª·¢°åTM8211
        }
        else
        {
             // air780E +ÒôÆµÐ¡°å
            luat_i2s_base_setup(0, I2S_MODE_I2S, I2S_FRAME_SIZE_16_16);
        }
    }
    audioQueueData audioQueueRecv = {0};
    uint32_t result = 0;
    while (1)
    {
        if (xQueueReceive(audioQueueHandle, &audioQueueRecv, portMAX_DELAY))
        {
            LUAT_DEBUG_PRINT("this is play priority %d", audioQueueRecv.priority);
            LUAT_DEBUG_PRINT("this is play playType %d", audioQueueRecv.playType);
            if (audioQueueRecv.priority == MONEY_PLAY)
            {

                if (audioQueueRecv.playType == TTS_PLAY)
                {
                    luat_audio_play_tts_text(0, audioQueueRecv.message.tts.data, audioQueueRecv.message.tts.len);
                }
                else if (audioQueueRecv.playType == FILE_PLAY)
                {
                    LUAT_DEBUG_PRINT("TEST address 2 %p", audioQueueRecv.message.file.info);
                    luat_audio_play_multi_files(0, audioQueueRecv.message.file.info, audioQueueRecv.message.file.count);
                }
            }
            else if (audioQueueRecv.priority == PAD_PLAY)
            {
            }
            result = osEventFlagsWait(waitAudioPlayDone, WAIT_PLAY_FLAG, osFlagsWaitAll, 20000);
            if (audioQueueRecv.playType == TTS_PLAY)
            {
                LUAT_DEBUG_PRINT("FREE MY AUDIO TTS");
                free(audioQueueRecv.message.tts.data);
            }
            else if (audioQueueRecv.playType == FILE_PLAY)
            {
                free(audioQueueRecv.message.file.info);
                LUAT_DEBUG_PRINT("FREE MY AUDIO FILE");
            }
        }
    }
}

void audio_task_init(void)
{
    luat_gpio_cfg_t gpio_cfg;
    luat_gpio_set_default_cfg(&gpio_cfg);
    luat_rtos_task_handle audio_task_handle;
    if ((1 == air780e_tm8211) || (1 == soft_dac))//Ê¹ÓÃtm8211 soft_dac Òý½Å¶¼ÐèÒª´ò¿ªPAµÄ¿ØÖÆÒý½Å
    {
        gpio_cfg.pin = PA_PWR_PIN;
        luat_gpio_open(&gpio_cfg);
    }
    gpio_cfg.pin = CODEC_PWR_PIN;
    luat_gpio_open(&gpio_cfg);
    gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
    luat_gpio_open(&gpio_cfg);
    audioQueueHandle = xQueueCreate(100, sizeof(audioQueueData));

    audioQueueData powerOn = {0};
    powerOn.playType = TTS_PLAY;
    powerOn.priority = MONEY_PLAY;
    char str[] = "»¶ÓÊ¹ÓÃºÏÖæÔÆ¶ËÒôÆµ²¥·ÅÉè±¸";
    powerOn.message.tts.data = malloc(sizeof(str));
    memcpy(powerOn.message.tts.data, str, sizeof(str));
    powerOn.message.tts.len = sizeof(str);
    if (pdTRUE != xQueueSend(audioQueueHandle, &powerOn, 0))
    {
        LUAT_DEBUG_PRINT("start send audio fail");
        free(powerOn.message.tts.data);
    }
    luat_rtos_task_create(&audio_task_handle, 5 * 1024, 60, "audio", audio_task, NULL, NULL);
}
/*------------------------------------------------audio-----------------------------------------------*/
