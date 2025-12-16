#include "common_api.h"
#include "luat_rtos.h"
#include "luat_mem.h"
#include "luat_debug.h"
#include "audio_Task.h"
luat_rtos_task_handle task1_handle;
luat_rtos_task_handle task2_handle;
#include "FreeRTOS.h"
#include "queue.h"
extern int fun;
extern QueueHandle_t audioQueueHandle;
static void loop_Playback_task(void *param)
{
    while (1)
    {
        luat_rtos_task_sleep(10000);
        LUAT_DEBUG_PRINT("fun %d",fun);
        if (1 == fun)
        {
            audioQueueData loop_play = {0};
            loop_play.playType = TTS_PLAY;
            loop_play.priority = MONEY_PLAY;
            char tts_string[] = "Ö§¸¶±¦ÊÕ¿î1234567.89Ôª,Î¢ÐÅÊÕ¿î9876543.21Ôª";
            loop_play.message.tts.data = malloc(sizeof(tts_string));
            memcpy(loop_play.message.tts.data, tts_string, sizeof(tts_string));
            loop_play.message.tts.len = sizeof(tts_string);
            if (pdTRUE != xQueueSend(audioQueueHandle, &loop_play, 0))
            {
                LUAT_DEBUG_PRINT("start send audio fail");
            }
        }
    }
}

void loop_Playback_task_init(void)
{
	luat_rtos_task_create(&task1_handle, 2*1024, 50, "loop_Playback_task", loop_Playback_task, NULL, 0);
}



