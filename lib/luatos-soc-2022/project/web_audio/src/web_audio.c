#include "web_audio.h"
#include "audio_Task.h"
#include "net_led_Task.h"

#include "luat_network_adapter.h"
#include "networkmgr.h"

#include "libemqtt.h"
#include "luat_mqtt.h"
#include "luat_rtos.h"
#include "charge_task.h"
#include "luat_http.h"
luat_rtos_queue_t MQTT_payload_queue;
static luat_rtos_task_handle http_down_task_handle;
static luat_rtos_queue_t http_down_queue;
extern QueueHandle_t audioQueueHandle;
extern uint8_t link_UP;
extern void fdb_init(void);
extern void key_task_init(void);
extern void loop_Playback_task_init(void);
char mqtt_subTopic[40];
static char subTopic[] = "test20220929/";
//861551057085904
/************************ÔÆ¶ËÒôÆµMQTT ¸ºÔØÏûÏ¢ÀàÐÍ******************** *******************/
// 0---4×Ö½Ú´ó¶ËÎÄ×Ö³¤¶È--ÎÄ×ÖµÄgbk±àÂë--1--4×Ö½Ú´ó¶ËÒôÆµurl³¤¶È--urlµÄgbk±àÂë
// 0±nÊ¾ºóÃæÊÇÆÕÍ¨ÎÄ×Ö¡£1±nÊ¾ºóÃæÊÇÒôÆµ¡£Á½ÖÖÈÎÒâ×éºÏ¡£³¤¶ÈÊÇºóÃæ±àÂëµÄ³¤¶È
/************************ÔÆ¶ËÒôÆµMQTT ¸ºÔØÏûÏ¢ÀàÐÍ******************** *******************/
void messageArrived(uint8_t payloadlen, uint8_t *data) // ¶ÔMQTTµÄ¸ºÔØÏûÏ¢½øÐÐ´¦Àn£¬½âÎöÊý¾ý
{
    luat_event_t event;
    int payload_array[60];
    int payload_array_len = 0;
   
    char *p = (char *)data;
    uint8_t payload_len = payloadlen;
    for (uint8_t i = 0; i < payload_len; i++)
    {
        if ((p[i] == 0) || (p[i] == 1))
        {
            if ((p[i + 1] == 0) && (p[i + 2] == 0) && (p[i + 3] == 0))
            {
                payload_array[payload_array_len] = p[i];
                payload_array_len = payload_array_len + 1;
                payload_array[payload_array_len] = p[i + 4];
                payload_array_len = payload_array_len + 1;
                payload_array[payload_array_len] = &p[i + 4];
                payload_array_len = payload_array_len + 1;
            }
        }
        LUAT_DEBUG_PRINT("ceshi%d", p[i]);
    }
    LUAT_DEBUG_PRINT("payload_array_len%d", payload_array_len);
    for (int i = 0; i < payload_array_len; i = i + 3)
    {
        if (payload_array[i] == 0)
        {
            CHAR *TTS = malloc(payload_array[i + 1] * sizeof(char));
            memcpy(TTS, payload_array[i + 2] + 1, payload_array[i + 1]);
            int ret = luat_audio_play_tts_text(0, TTS, payload_array[i + 1]);
            if (0 == ret)
            {
                free(TTS);
            }
        }
        else if (payload_array[i]==1)
        {
            http_down http_down_struct={0};
            http_down_struct.url=malloc(payload_array[i + 1] * sizeof(int)); 
            memset(http_down_struct.url, '\0', payload_array[i + 1] * sizeof(int));
            memcpy(http_down_struct.url, payload_array[i + 2] + 1, payload_array[i + 1]);
            http_down_struct.url_len=payload_array[i + 1];
            LUAT_DEBUG_PRINT("http url%s",http_down_struct.url);
            LUAT_DEBUG_PRINT("url len%d", http_down_struct.url_len);
            int ret = luat_rtos_queue_send(http_down_queue, &http_down_struct, NULL, 0);
            LUAT_DEBUG_PRINT("laut_rtos_send_sucessed%d", ret);
            if (0 == ret)
            {
                LUAT_DEBUG_PRINT("laut_rtos_send_sucessed");
            }
        }
        luat_rtos_task_sleep(1500);
        LUAT_DEBUG_PRINT("payload_array%d", payload_array[i]);
    }
}

void mqtt_payload_task(void *param)
{
    payloaddata mqtt_queue_recv = {0};
    while (1)
    {
        if (0 == luat_rtos_queue_recv(MQTT_payload_queue, &mqtt_queue_recv, NULL, portMAX_DELAY))
        {
            LUAT_DEBUG_PRINT("pub_msg: %d", mqtt_queue_recv.payload_len);
            messageArrived(mqtt_queue_recv.payload_len, mqtt_queue_recv.payload_data);
        }
    }
}

void Mqtt_payload_task_Init(void) // µ¥¶À ̧ã ̧öÈÎÎñ ̧ºððÏÂÔØ£¬Êý3⁄4Í¨¨¹ÏÏûÏ¢¶ÓÁÐÊÊä
{
    int ret = -1;
    luat_rtos_task_handle Mqtt_payload_handle;
    ret = luat_rtos_queue_create(&MQTT_payload_queue, 100, sizeof(payloaddata));
    if (0 == ret)
    {
        LUAT_DEBUG_PRINT("mqtt_payload_queue create sucessed\r\n");
    }
    luat_rtos_task_create(&Mqtt_payload_handle, 6 * 1024, 50, "Mqtt_payload_task", mqtt_payload_task, NULL, NULL);
}

static void luat_mqtt_cb(luat_mqtt_ctrl_t *luat_mqtt_ctrl, uint16_t event)
{
    switch (event)
    {
    case MQTT_MSG_CONNACK:
    {
        LUAT_DEBUG_PRINT("mqtt_subscribe");
        mqtt_subscribe(&(luat_mqtt_ctrl->broker), mqtt_subTopic, NULL, 0);
        break;
    }
    case MQTT_MSG_PUBLISH:
    {
        const uint8_t *ptr;
        int ret = -1;
        uint8_t payloadlen = mqtt_parse_pub_msg_ptr(luat_mqtt_ctrl->mqtt_packet_buffer, &ptr);
        LUAT_DEBUG_PRINT("pub_msg: %d%s", payloadlen, ptr);
        payloaddata payload_send = {0};
        payload_send.payload_len = payloadlen;
        payload_send.payload_data = ptr;
        ret = luat_rtos_queue_send(MQTT_payload_queue, &payload_send, NULL, 0);
        LUAT_DEBUG_PRINT("laut_rtos_send_sucessed%d", ret);
        if (0 == ret)
        {
            LUAT_DEBUG_PRINT("laut_rtos_send_sucessed");
        }

        break;
    }
    case MQTT_MSG_PUBACK:
    case MQTT_MSG_PUBCOMP:
    {
        LUAT_DEBUG_PRINT("msg_id: %d", mqtt_parse_msg_id(luat_mqtt_ctrl->mqtt_packet_buffer));
        break;
    }
    default:
        break;
    }
    return;
}


static void mqtt_demo(void)
{
    CHAR IMEI[20] = {0};
    if (luat_mobile_get_imei(0, IMEI, sizeof(IMEI)))
    {
        sprintf(mqtt_subTopic, "%s%s", subTopic, IMEI);
        LUAT_DEBUG_PRINT("imei%s", mqtt_subTopic);
    }
    int rc = -1;
    luat_mqtt_ctrl_t *luat_mqtt_ctrl = (luat_mqtt_ctrl_t *)luat_heap_malloc(sizeof(luat_mqtt_ctrl_t));
    rc = luat_mqtt_init(luat_mqtt_ctrl, NW_ADAPTER_INDEX_LWIP_GPRS);
    if (rc)
    {
        LUAT_DEBUG_PRINT("mqtt init FAID ret %d", rc);
        return 0;
    }
    luat_mqtt_ctrl->ip_addr.type = 0xff;
    luat_mqtt_connopts_t opts = {0};
    opts.host = HOST;
    opts.port = PORT;
    opts.is_tls = 0;
    luat_mqtt_set_connopts(luat_mqtt_ctrl, &opts);

    mqtt_init(&(luat_mqtt_ctrl->broker), IMEI);

    mqtt_init_auth(&(luat_mqtt_ctrl->broker), User, Password);

    // luat_mqtt_ctrl->netc->is_debug = 1;// debug
    luat_mqtt_ctrl->broker.clean_session = 1;
    luat_mqtt_ctrl->keepalive = 240;

    luat_mqtt_ctrl->reconnect = 1;
    luat_mqtt_ctrl->reconnect_time = 3000;

    luat_mqtt_set_cb(luat_mqtt_ctrl, luat_mqtt_cb);
    while (1)
    {
        while (!link_UP)
        {
            luat_rtos_task_sleep(1000);
        }
        rc = luat_mqtt_connect(luat_mqtt_ctrl);
        if (0 == rc)
        {
            audioQueueData MQTT_link = {0};
            MQTT_link.playType = TTS_PLAY;
            MQTT_link.priority = MONEY_PLAY;
            char str[] = "·þÎñÆ÷Á¬½Ó³É¹¦";
            MQTT_link.message.tts.data = malloc(sizeof(str));
            memcpy(MQTT_link.message.tts.data, str, sizeof(str));
            MQTT_link.message.tts.len = sizeof(str);
            if (pdTRUE != xQueueSend(audioQueueHandle, &MQTT_link, 0))
            {
                LUAT_DEBUG_PRINT("start send audio fail");
                free(MQTT_link.message.tts.data);
            }
        }
        else
        {
            audioQueueData MQTT_link = {0};
            MQTT_link.playType = TTS_PLAY;
            MQTT_link.priority = MONEY_PLAY;
            char str[] = "·þÎñÆ÷Á¬½ÓÊ§°Ü";
            MQTT_link.message.tts.data = malloc(sizeof(str));
            memcpy(MQTT_link.message.tts.data, str, sizeof(str));
            MQTT_link.message.tts.len = sizeof(str);
            if (pdTRUE != xQueueSend(audioQueueHandle, &MQTT_link, 0))
            {
                LUAT_DEBUG_PRINT("start send audio fail");
                free(MQTT_link.message.tts.data);
            }
        }
        while (1)
        {
            luat_rtos_task_sleep(1000);
        }
    }
}
static void mqttclient_task_init(void)
{
    luat_rtos_task_handle mqttclient_task_handle;
    luat_rtos_task_create(&mqttclient_task_handle, 4 * 1024, 80, "mqttclient", mqtt_demo, NULL, NULL);
}
/*------------------------------------------------------MQTT event-----------------------------------------------------------------*/
/*------------------------------------------------------HTTP DOWN------------------------------------------------------------------*/
enum
{
	audio_HTTP_GET_HEAD_DONE = 1,
	audio_HTTP_GET_DATA,
	audio_HTTP_GET_DATA_DONE,
	audio_HTTP_FAILED,
};
static void luatos_http_cb(int status, void *data, uint32_t len, void *param)
{
	uint32_t *http_down_data;
    if(status < 0) 
    {
        LUAT_DEBUG_PRINT("http failed! %d", status);
		luat_rtos_event_send(param, audio_HTTP_FAILED, 0, 0, 0, 0);
		return;
    }
	switch(status)
	{
	case HTTP_STATE_GET_BODY:
		if (data)
		{
            LUAT_DEBUG_PRINT("http_down_data len%d\r\n",len);
			http_down_data = malloc(len);
			memcpy(http_down_data, data, len);
			luat_rtos_event_send(param, audio_HTTP_GET_DATA, http_down_data, len, 0, 0);
		}
        else
		{
			luat_rtos_event_send(param, audio_HTTP_GET_DATA_DONE, 0, 0, 0, 0);
		}
		break;
	case HTTP_STATE_GET_HEAD:
		if (data)
		{
			LUAT_DEBUG_PRINT("%s", data);
		}
        else
		{
			luat_rtos_event_send(param, audio_HTTP_GET_HEAD_DONE, 0, 0, 0, 0);
		}
		break;
	case HTTP_STATE_IDLE:
		break;
	case HTTP_STATE_SEND_BODY_START:
		//Èç¹ûÊÇPOST£¬ÔÚÕâÀï·¢ËÍPOSTµÄbodyÊ¾Ý£¬Èç¹ûÒ»´Î·¢ËÍ²»Íê£¬¿ÉÒÔÔÚHTTP_STATE_SEND_BODY»Øµ÷Àï¼ÌÐø·¢ËÍ
		break;
	case HTTP_STATE_SEND_BODY:
		//Èç¹ûÊÇPOST£¬¿ÉÒÔÔÚÕâÀï·¢ËÍPOSTÊ£ÓàµÄbodyÊý¾Ý
		break;
	default:
		break;
	}
}

static void http_down_task(void *param)
{
    http_down http_down_recv = {0};
    luat_audio_play_info_t info[1];
    luat_event_t event;
    uint32_t all, now_free_block, min_free_block;
    int done_len = 0;
    char *audio_Path;
    while (1)
    {
        if (0 == luat_rtos_queue_recv(http_down_queue, &http_down_recv, NULL, portMAX_DELAY))
        {

            LUAT_DEBUG_PRINT("http url %s", http_down_recv.url);
            LUAT_DEBUG_PRINT("http url len %d", http_down_recv.url_len);
            luat_http_ctrl_t *http = luat_http_client_create(luatos_http_cb, luat_rtos_get_current_handle(), -1);
            luat_http_client_ssl_config(http, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            luat_http_client_start(http, http_down_recv.url, 0, 0, 0);
            audio_Path=malloc(10);
            memset(audio_Path,'\0',10);
            if (http_down_recv.url[http_down_recv.url_len - 1] == '3')
            {
               memcpy(audio_Path,"test1.mp3",strlen("test1.mp3"));
            }
            else if (http_down_recv.url[http_down_recv.url_len - 1] == 'r')
            {
                memcpy(audio_Path,"test1.amr",strlen("test1.amr"));
            }
            else if(http_down_recv.url[http_down_recv.url_len - 1] == 'v')
            {
                memcpy(audio_Path,"test1.wav",strlen("test1.wav"));
            }
            LUAT_DEBUG_PRINT("audio path %s",audio_Path);
            while (1)
            {
                luat_rtos_event_recv(http_down_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
                LUAT_DEBUG_PRINT("ÒôÆµÎÄ¼þÏÂÔØÍê³É");
                switch (event.id)
                {
                case audio_HTTP_GET_HEAD_DONE:
                    done_len = 0;
                    LUAT_DEBUG_PRINT("status %d total %u", luat_http_client_get_status_code(http), http->total_len);
                    break;
                case audio_HTTP_GET_DATA:
                    if (done_len <=0 )
                    {
                        FILE *fp1 = luat_fs_fopen(audio_Path, "wb+");
                        uint32_t status = luat_fs_fwrite((uint32_t *)event.param1, event.param2, 1, fp1);
                        if (0 == status)
                        {
                            while (1)
                                ;
                        }
                        luat_fs_fclose(fp1);
                        done_len = done_len + event.param2;
                    }
                    else if ((done_len >= 0) && (done_len < http->total_len))
                    {
                        FILE *fp1 = luat_fs_fopen(audio_Path, "ab+");
                        uint32_t status = luat_fs_fwrite((uint32_t *)event.param1, event.param2, 1, fp1);
                        if (0 == status)
                        {
                            while (1)
                                ;
                        }
                        luat_fs_fclose(fp1);
                        done_len = done_len + event.param2;
                    }
                    if (done_len == http->total_len)
                    {
                        info[0].path = audio_Path;
                        luat_audio_play_multi_files(0, info, 1);
                    }
                    free(event.param1);
                    break;
                case audio_HTTP_GET_DATA_DONE:
                    break;
                case audio_HTTP_FAILED:
                    break;
                }
                if (done_len==http->total_len)
                {
                    free(audio_Path);
                    break;
                }
            }
            luat_rtos_task_sleep(2000);
            free(http_down_recv.url);
            luat_http_client_close(http);
            luat_http_client_destroy(&http);
            memset(info, 0, sizeof(info));
        }
        luat_meminfo_sys(&all, &now_free_block, &min_free_block);
        LUAT_DEBUG_PRINT("meminfo %d,%d,%d", all, now_free_block, min_free_block);
    }
}
static void http_down_task_init(void)
{
    int ret = luat_rtos_queue_create(&http_down_queue, 100, sizeof(http_down));
    if (0 == ret)
    {
        LUAT_DEBUG_PRINT("mqtt_payload_queue create sucessed\r\n");
    }
    luat_rtos_task_create(&http_down_task_handle, 20 * 1024, 80, "http_down", http_down_task, NULL, 16);
}
INIT_HW_EXPORT(Task_netinfo_call, "0");
INIT_HW_EXPORT(fdb_init, "1");
INIT_TASK_EXPORT(mqttclient_task_init, "1");
INIT_TASK_EXPORT(Mqtt_payload_task_Init, "1");
INIT_TASK_EXPORT(audio_task_init, "2");
INIT_TASK_EXPORT(key_task_init, "3");
INIT_TASK_EXPORT(charge_task_init, "4");
INIT_TASK_EXPORT(NET_LED_Task, "5");
INIT_TASK_EXPORT(loop_Playback_task_init, "6");
INIT_TASK_EXPORT(http_down_task_init,"7");
