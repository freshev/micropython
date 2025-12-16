
#include "luat_rtos.h"
#include "luat_debug.h"
#include "mqtt_task.h"
#include "luat_mem.h"
luat_rtos_task_handle mqtt_send_message_task_handle = NULL;
extern luat_rtos_task_handle mqtt_publish_task_handle;
static void mqtt_publish_callbcak(LUAT_MQTT_PUBLISH_ERROR_CODE_E code)
{
    LUAT_DEBUG_PRINT("cloud_speaker mqtt publish result %d", code);
}
static void send_message_task(void *args)
{
	while(1)
	{
        mqtt_publish_para_t *para = (mqtt_publish_para_t *)luat_heap_malloc(sizeof(mqtt_publish_para_t));
        memset(para, 0, sizeof(mqtt_publish_para_t));
        para->msg = (char *)luat_heap_malloc(50);
        snprintf(para->msg, 50, "%s", "luat_mqtt_send_payload");
        para->msg_len = strlen("luat_mqtt_send_payload");
        para->qos = 1;
        para->retain = 0;
        para->callback = mqtt_publish_callbcak;
        if(luat_rtos_message_send(mqtt_publish_task_handle, TOPIC_1, (void *)para) != 0)
        {
            luat_heap_free(para->msg);
            luat_heap_free(para);
        }
        luat_rtos_task_sleep(5000);
	}
}

void mqtt_send_message_init(void)
{
    luat_rtos_task_create(&mqtt_send_message_task_handle, 2 * 1024, 10, "send message", send_message_task, NULL, NULL);
}
