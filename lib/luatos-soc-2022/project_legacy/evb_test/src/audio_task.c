#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "common_api.h"
#include "audio_task.h"
#include "timers.h"
#include "portmacro.h"
#include "audio_play.h"
#include "ivTTS.h"
#include "slpman.h"
#include "gpio.h"
#include "pad.h"
#define WAIT_PLAY_FLAG (0x1)
#include "common_api.h"
#include "bsp_custom.h"
#include "ostask.h"
#include DEBUG_LOG_HEADER_FILE
#include "plat_config.h"
#include "audio_play.h"
#include "audio_ll_drv.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "slpman.h"
#include "osasys.h"
#include "version.h"
#include "ivTTS.h"
extern const unsigned char audiopoweron[];
static osEventFlagsId_t waitAudioPlayDone = NULL;
QueueHandle_t audioQueueHandle = NULL;
static uint8_t audio_sleep_handler = 0xff;
static TimerHandle_t delay_timer;
void audio_data_cb(uint8_t *data, uint32_t len, uint8_t bits, uint8_t channels)
{
    //Here you can perform software volume scaling on the audio data, or clear it directly to mute it.
    //Software volume scaling reference HAL_I2sSrcAdjustVolumn
    DBG("%x,%d,%d,%d", data, len, bits, channels);
}
void app_pa_on(uint32_t arg)
{
#if defined(EVB_AIR600EAC_CLOUD_SPEAK) || (EVB_AIR600E_CLOUD_SPEAK)
    GPIO_pinWrite(0, 1 << 10, 1 << 10);
#elif defined(EVB_AIR780E_CLOUD_SPEAK)
    GPIO_pinWrite(1, 1 << 9, 1 << 9);
#endif
}
void audio_event_cb(uint32_t event, void *param)
{
    //	PadConfig_t pad_config;
    //	GpioPinConfig_t gpio_config;

    DBG("%d", event);
    switch (event)
    {
    case MULTIMEDIA_CB_AUDIO_DECODE_START:
        slpManPlatVoteDisableSleep(audio_sleep_handler, SLP_SLP1_STATE);
        GPIO_pinWrite(0, 1 << 12, 1 << 12);
        audio_play_write_blank_raw(0, 6);
        break;
    case MULTIMEDIA_CB_AUDIO_OUTPUT_START:
        xTimerStart(delay_timer, 200);
        break;
    case MULTIMEDIA_CB_TTS_INIT:
        if (4 == sizeof("Hello"))
        {
            audio_play_tts_set_param(0, ivTTS_PARAM_INPUT_CODEPAGE, ivTTS_CODEPAGE_GBK);
        }
        else
        {
            audio_play_tts_set_param(0, ivTTS_PARAM_INPUT_CODEPAGE, ivTTS_CODEPAGE_UTF8);
        }
        break;
    case MULTIMEDIA_CB_AUDIO_DONE:
        xTimerStop(delay_timer, 0);
        DBG("audio play done, result = %d!", audio_play_get_last_error(0));

#if defined(EVB_AIR600EAC_CLOUD_SPEAK) || (EVB_AIR600E_CLOUD_SPEAK)
        GPIO_pinWrite(0, 1 << 10, 0);
#elif defined(EVB_AIR780E_CLOUD_SPEAK)
        GPIO_pinWrite(1, 1 << 9, 0);
#endif
        GPIO_pinWrite(0, 1 << 12, 0);
        slpManPlatVoteEnableSleep(audio_sleep_handler, SLP_SLP1_STATE);
        osEventFlagsSet(waitAudioPlayDone, WAIT_PLAY_FLAG);
        break;
    }
}

void audio_task(void *param)
{
    audioQueueData audioQueueRecv = {0};
    uint32_t result = 0;
    while (1)
    {
        if (xQueueReceive(audioQueueHandle, &audioQueueRecv, portMAX_DELAY))
        {
            DBG("this is play priority %d", audioQueueRecv.priority);
            DBG("this is play playType %d", audioQueueRecv.playType);
            if (audioQueueRecv.priority == MONEY_PLAY)
            {

                if (audioQueueRecv.playType == TTS_PLAY)
                {
                    audio_play_tts_text(0, audioQueueRecv.message.tts.data, audioQueueRecv.message.tts.len);
                }
                else if (audioQueueRecv.playType == FILE_PLAY)
                {
                    audio_play_multi_files(0, audioQueueRecv.message.file.info, audioQueueRecv.message.file.count);
                }
            }
            else if (audioQueueRecv.priority == PAD_PLAY)
            {
            }
            result = osEventFlagsWait(waitAudioPlayDone, WAIT_PLAY_FLAG, osFlagsWaitAll, 20000);
            if (audioQueueRecv.playType == TTS_PLAY)
            {
                DBG("free tts data");
                free(audioQueueRecv.message.tts.data);
            }
            else if (audioQueueRecv.playType == FILE_PLAY)
            {
                free(audioQueueRecv.message.file.info);
                DBG("free file data");
            }
        }
    }
    vTaskDelete(NULL);
}

void audio_task_init(void)
{
    PadConfig_t pad_config;
    GpioPinConfig_t gpio_config;
    PAD_getDefaultConfig(&pad_config);
    pad_config.mux = PAD_MUX_ALT4;
    pad_config.pullSelect = PAD_PULL_INTERNAL;
    pad_config.pullDownEnable = PAD_PULL_DOWN_ENABLE;
    PAD_setPinConfig(11, &pad_config); // SWCLK0 = GPIO12
    gpio_config.pinDirection = GPIO_DIRECTION_OUTPUT;
    gpio_config.misc.initOutput = 0;
    GPIO_pinConfig(0, 12, &gpio_config);
    pad_config.mux = PAD_MUX_ALT0;
#if defined(EVB_AIR600EAC_CLOUD_SPEAK) || (EVB_AIR600E_CLOUD_SPEAK)
    PAD_setPinConfig(25, &pad_config); // PAD25 = GPIO10
    GPIO_pinConfig(0, 10, &gpio_config);
#elif defined(EVB_AIR780E_CLOUD_SPEAK)
    PAD_setPinConfig(45, &pad_config);
    GPIO_pinConfig(1, 9, &gpio_config);
#endif

    ivCStrA sdk_id = AISOUND_SDK_USERID;
    slpManSetPmuSleepMode(true, SLP_SLP1_STATE, false);
    slpManApplyPlatVoteHandle("audio", &audio_sleep_handler);

    slpManPlatVoteDisableSleep(audio_sleep_handler, SLP_SLP2_STATE);
    delay_timer = xTimerCreate(NULL, 200, 0, 0, app_pa_on);

    audio_play_global_init(audio_event_cb, audio_data_cb, NULL);
    audio_play_tts_set_resource(ivtts_16k_lite, sdk_id);
//Now use ES7149/ES7148, use the following configuration, if not, please configure according to the actual situation, bus_id directly write 0
#if defined(EVB_AIR600EAC_CLOUD_SPEAK)
    Audio_CodecI2SInit(0, I2S_MODE_I2S, I2S_FRAME_SIZE_16_16);
//The following configuration can use TM8211
#elif defined(EVB_AIR780E_CLOUD_SPEAK) || (EVB_AIR600E_CLOUD_SPEAK)
    Audio_CodecI2SInit(0, I2S_MODE_MSB, I2S_FRAME_SIZE_16_16);
#endif

    if (waitAudioPlayDone == NULL)
    {
        waitAudioPlayDone = osEventFlagsNew(NULL);
    }
    audioQueueHandle = xQueueCreate(100, sizeof(audioQueueData));
    audioQueueData powerOn = {0};
    powerOn.playType = TTS_PLAY;
    powerOn.priority = MONEY_PLAY;
    char str[] = "Starting up";
    powerOn.message.tts.data = malloc(sizeof(str));
    memcpy(powerOn.message.tts.data, str, sizeof(str));
    powerOn.message.tts.len = sizeof(str);
    if (pdTRUE != xQueueSend(audioQueueHandle, &powerOn, 0))
    {
        DBG("start send audio fail");
    }
    xTaskCreate(audio_task, " ", 2048, NULL, 20, NULL);
}
