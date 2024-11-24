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

#include "bsp.h"
#include "bsp_custom.h"
#include "common_api.h"
#include "cJSON.h"
#include "audio_task.h"
#include "audio_file.h"
#include "math.h"
#include "luat_rtos.h"
#include "luat_mobile.h"
#include "luat_network_adapter.h"
#include "networkmgr.h"
#include "libemqtt.h"
#include "luat_mqtt.h"
#include "luat_debug.h"
#include "luat_pm.h"
#include "mqtt_task.h"
uint8_t g_s_is_link_up = 0;

luat_rtos_task_handle mqtt_publish_task_handle = NULL;

static luat_rtos_task_handle mqtt_task_handle = NULL;
extern luat_rtos_queue_t audio_queue_handle;

static luat_mqtt_ctrl_t *luat_mqtt_ctrl = NULL;


#define MQTT_HOST    	"lbsmqtt.airm2m.com"   				//MQTT server address and port number
#define MQTT_PORT		 1884

#define MQTT_SEND_BUFF_LEN       (1024)
#define MQTT_RECV_BUFF_LEN       (1024)

#define CLIENTID    "12345678"
const static char mqtt_sub_topic_head[] = "/sub/topic/money/";    //Subscribed topic header, to be spliced   with device imei
static char mqtt_sub_topic[40];                            //The subscribed topic, the subscribed topic header and device imei to be stored, a total of 17+15,32 characters

static char mqtt_topic_1[] = "/mqtt_pub_topic_1";
static char mqtt_topic_2[] = "/mqtt_pub_topic_2";
static char mqtt_topic_3[] = "/mqtt_pub_topic_3";

uint8_t get_net_status()
{
    return g_s_is_link_up;
}
uint8_t get_server_status()
{
    return luat_mqtt_ctrl->mqtt_state;
}

int fomatMoney(int num, audioQueueData *data, int *index, BOOL flag)
{
     uint32_t  audioArray[10][2] = 
     {
         {audio0,   sizeof(audio0)},
         {audio1,   sizeof(audio1)},
         {audio2,   sizeof(audio2)},
         {audio3,   sizeof(audio3)},
         {audio4,   sizeof(audio4)},
         {audio5,   sizeof(audio5)},
         {audio6,   sizeof(audio6)},
         {audio7,   sizeof(audio7)},
         {audio8,   sizeof(audio8)},
         {audio9,   sizeof(audio9)}
    };
    int thousand = (num - num % 1000) / 1000;
    int hundred = ((num % 1000) - ((num % 1000) % 100)) / 100;
    int ten = ((num % 100) - ((num % 100) % 10)) / 10;
    int unit = num % 10;
    if (thousand == 0)
    {
        thousand = -1;
        if (hundred == 0)
        {
            hundred = -1;
            if (ten == 0)
            {
                ten = -1;
                if (unit == 0)
                {
                    unit = -1;
                }
            }
        }
    }
    if (unit == 0)
    {
        unit = -1;
        if (ten == 0)
        {
            ten = -1;
            if (hundred == 0)
            {
                hundred = -1;
                if (thousand == 0)
                {
                    thousand = -1;
                }
            }
        }
    }
    if (ten == 0 && hundred == 0)
    {
        ten = -1;
    }
    if (thousand != -1)
    {
        if (flag)
        {
            data->message.file.info[*index].path = NULL; 
            if(2 == thousand)
            {
                data->message.file.info[*index].address = audioliang;
                data->message.file.info[*index].rom_data_len = audioliangSize;
            }
            else
            {
                data->message.file.info[*index].address = audioArray[thousand][0];
                data->message.file.info[*index].rom_data_len = audioArray[thousand][1];
            }
        }
        *index += 1;
        if (flag)
        {
            data->message.file.info[*index].path = NULL; 
            data->message.file.info[*index].address = audio1000;
            data->message.file.info[*index].rom_data_len = sizeof(audio1000);
        }
        *index += 1;
    }
    if (hundred != -1)
    {
        if(flag)
        {
            data->message.file.info[*index].path = NULL; 
            if(2 == hundred)
            {
                data->message.file.info[*index].address = audioliang;
                data->message.file.info[*index].rom_data_len = audioliangSize;
            }
            else
            {
                data->message.file.info[*index].address = audioArray[hundred][0];
                data->message.file.info[*index].rom_data_len = audioArray[hundred][1];
        }
        }
        *index += 1;
        if(flag)
        {
            data->message.file.info[*index].path = NULL; 
            data->message.file.info[*index].address = audio100;
            data->message.file.info[*index].rom_data_len = sizeof(audio100);
        }
        *index += 1;
    }
    if (ten != -1)
    {
        if (!(ten == 1 && hundred == -1 && thousand == -1))
        {
            if(flag)
            {
                data->message.file.info[*index].path = NULL; 
                data->message.file.info[*index].address = audioArray[ten][0];
                data->message.file.info[*index].rom_data_len = audioArray[ten][1];
            }
            *index += 1;
        }
        if (ten != 0)
        {
            if(flag)
            {
                data->message.file.info[*index].path = NULL; 
                data->message.file.info[*index].address = audio10;
                data->message.file.info[*index].rom_data_len = sizeof(audio10);
            }
            *index += 1;
        }
    }
    if (unit != -1)
    {
        if(flag)
        {
            data->message.file.info[*index].path = NULL; 
            data->message.file.info[*index].address = audioArray[unit][0];
            data->message.file.info[*index].rom_data_len = audioArray[unit][1];
        }
        *index += 1;
    }
    return 0;
}
static int strToFile(char *money, audioQueueData *data, int *index, bool flag)
{
    if (flag)
    {
        data->message.file.info[*index].address  = audiozhifubao;
        data->message.file.info[*index].rom_data_len  = sizeof(audiozhifubao);
    }
    *index += 1;
     uint32_t  audioArray[10][2] = 
     {
         {audio0, sizeof(audio0)},
         {audio1, sizeof(audio1)},
         {audio2, sizeof(audio2)},
         {audio3, sizeof(audio3)},
         {audio4, sizeof(audio4)},
         {audio5, sizeof(audio5)},
         {audio6, sizeof(audio6)},
         {audio7, sizeof(audio7)},
         {audio8, sizeof(audio8)},
         {audio9, sizeof(audio9)}
    };
    int count = 0;
    int integer = 0;
    char *str = NULL;
    char intStr[8] = {0};
    char decStr[3] = {0};
    str = strstr(money, ".");
    if (str != NULL)
    {
        memcpy(intStr, money, str - money);
        str = str + 1;
        memcpy(decStr, str, 2);
        integer = atoi(intStr);
    }
    else
    {
        integer = atoi(money);
    }
    if (integer >= 10000)
    {
        int filecount = fomatMoney(integer / 10000, data, index, flag);
        if (flag)
        {
            if((2 == *index) && (data->message.file.info[1].address == audio2))
            {
                data->message.file.info[1].address = audioliang;
                data->message.file.info[1].rom_data_len = sizeof(audioliang);
            }
            data->message.file.info[*index].path = NULL; 
            data->message.file.info[*index].address = audio10000;
            data->message.file.info[*index].rom_data_len = sizeof(audio10000);
        }
        *index += 1;
        if (((integer % 10000) < 1000) && ((integer % 10000) != 0))
        {
            if (flag)
            {
                data->message.file.info[*index].path = NULL; 
                data->message.file.info[*index].address = audioArray[0][0];
                data->message.file.info[*index].rom_data_len = audioArray[0][1];
            }
            *index += 1;
        }
    }
    if ((integer % 10000) > 0)
    {
        int filecount = fomatMoney(integer % 10000, data, index, flag);
    }
    if (*index == 1)
    {
        if (flag)
        {
            data->message.file.info[*index].path = NULL; 
            data->message.file.info[*index].address = audioArray[0][0];
            data->message.file.info[*index].rom_data_len = audioArray[0][1];
        }
        *index += 1;
    }
    int fraction = atoi(decStr);
    if (fraction > 0)
    {
        if (flag)
        {
            data->message.file.info[*index].path = NULL; 
            data->message.file.info[*index].address = audiodot;
            data->message.file.info[*index].rom_data_len = sizeof(audiodot);
        }
        *index += 1;
        if (fraction >= 10)
        {
            int ten = fraction / 10;
            int unit = fraction % 10;
            if (ten != 0 && unit != 0)
            {
                if (flag)
                {
                    data->message.file.info[*index].path = NULL; 
                    data->message.file.info[*index].address = audioArray[ten][0];
                    data->message.file.info[*index].rom_data_len = audioArray[ten][1];
                }
                *index += 1;
                if(flag)
                {
                    data->message.file.info[*index].path = NULL; 
                    data->message.file.info[*index].address = audioArray[unit][0];
                    data->message.file.info[*index].rom_data_len = audioArray[unit][1];
                }
                *index += 1;
            }
            else if(ten == 0 && unit!=0)
            {
                if (flag)
                {
                    data->message.file.info[*index].path = NULL; 
                    data->message.file.info[*index].address = audioArray[0][0];
                    data->message.file.info[*index].rom_data_len = audioArray[0][1];
                }
                *index += 1;
                if(flag)
                {
                    data->message.file.info[*index].path = NULL; 
                    data->message.file.info[*index].address = audioArray[unit][0];
                    data->message.file.info[*index].rom_data_len = audioArray[0][1];
                }
                *index += 1;
            }
            else if(ten !=0 && unit == 0)
            {
                if (flag)
                {
                    data->message.file.info[*index].path = NULL; 
                    data->message.file.info[*index].address = audioArray[ten][0];
                    data->message.file.info[*index].rom_data_len = audioArray[ten][1];
                }
                *index += 1;
            }
        }
        else
        {
            if(decStr[0] == 0x30)
            {
                if (flag)
                {
                    data->message.file.info[*index].path = NULL; 
                    data->message.file.info[*index].address = audioArray[0][0];
                    data->message.file.info[*index].rom_data_len = audioArray[0][1];
                }
                *index += 1;
            }
            if (flag)
            {
                data->message.file.info[*index].path = NULL; 
                data->message.file.info[*index].address = audioArray[fraction][0];
                data->message.file.info[*index].rom_data_len = audioArray[fraction][1];
            }
            *index += 1;
        }
    }
    if (flag)
    {
        data->message.file.info[*index].path = NULL; 
        data->message.file.info[*index].address = audioyuan;
        data->message.file.info[*index].rom_data_len = sizeof(audioyuan);
    }
    *index += 1;
    return count;
}

static void luat_mqtt_cb(luat_mqtt_ctrl_t *luat_mqtt_ctrl, uint16_t event){
	switch (event)
	{
	case MQTT_MSG_CONNACK:{
		LUAT_DEBUG_PRINT("mqtt_subscribe");
		uint16_t msgid = 0;
		if(1 == mqtt_subscribe(&(luat_mqtt_ctrl->broker), mqtt_sub_topic, &msgid, 1))
        {
            audioQueueData welcome = {0};
            welcome.playType = TTS_PLAY;
            welcome.priority = MONEY_PLAY;
            char str[] = "Server connection successful"; 
            welcome.message.tts.data = luat_heap_malloc(sizeof(str));
            memcpy(welcome.message.tts.data, str, sizeof(str));
            welcome.message.tts.len = sizeof(str);
            if (-1 == luat_rtos_queue_send(audio_queue_handle, &welcome, NULL, 0)){
                free(welcome.message.tts.data);
                LUAT_DEBUG_PRINT("cloud_speaker_mqtt sub audio queue send error");
            }
        }
		break;
	}
	case MQTT_MSG_PUBLISH : {
		const uint8_t* topic;
		uint16_t topic_len = mqtt_parse_pub_topic_ptr(luat_mqtt_ctrl->mqtt_packet_buffer, &topic);
		LUAT_DEBUG_PRINT("pub_topic: %.*s",topic_len, topic);
        const uint8_t* payload;
		uint16_t payload_len = mqtt_parse_pub_msg_ptr(luat_mqtt_ctrl->mqtt_packet_buffer, &payload);
		LUAT_DEBUG_PRINT("pub_msg: %.*s",payload_len, payload);
        if (strncmp(mqtt_sub_topic, topic, strlen(mqtt_sub_topic)) == 0)
        {
            cJSON *boss = NULL;
            boss = cJSON_Parse((const char *)payload);
            if (boss == NULL){
                LUAT_DEBUG_PRINT("cloud_speaker_mqtt cjson parse fail");
            }
            else
            {
                LUAT_DEBUG_PRINT("cloud_speaker_mqtt cjson parse success");
                cJSON *money = cJSON_GetObjectItem(boss, "money");
                if(money == NULL)
                {
                    LUAT_DEBUG_PRINT("cloud_speaker_mqtt Missing amount field %d", money);
                    cJSON_Delete(boss);
                    return;
                }
                if (cJSON_IsString(money))
                {
                    audioQueueData moneyPlay = {0};
                    moneyPlay.priority = MONEY_PLAY;
                    moneyPlay.playType = FILE_PLAY;
                    char* str = NULL;
                    str = strstr(money->valuestring, ".");
                    //Determine whether the amount length is greater than 8, which is an amount of tens of millions. If so, the successful collection will be broadcast. If not, the corresponding amount will be broadcast. There is no legality judgment on the amount field here.
                    if (str != NULL)
                    {
                        if((str - money->valuestring) > 8)
                        {
                            moneyPlay.message.file.info = (audio_play_info_t *)calloc(1, sizeof(audio_play_info_t));
                            moneyPlay.message.file.info->address = audioshoukuanchenggong;
                            moneyPlay.message.file.info->rom_data_len = audioshoukuanchenggongSize;
                            moneyPlay.message.file.count = 1;
                        }
                        else
                        {
                            str++;
                            //If there are more than two decimal places, it means the digital amount is illegal and the payment is reported successfully.
                            if(strlen(str) > 2)
                            {
                                moneyPlay.message.file.info = (audio_play_info_t *)calloc(1, sizeof(audio_play_info_t));
                                moneyPlay.message.file.info->address = audioshoukuanchenggong;
                                moneyPlay.message.file.info->rom_data_len = audioshoukuanchenggongSize;
                                moneyPlay.message.file.count = 1;
                            }
                            else
                            {
                                //Call strToFile to format the amount into the corresponding file broadcast data. It needs to be called twice. The first time to obtain the space that requires malloc, and the second time to put the file data into the space.
                                int index = 0;
                                strToFile(money->valuestring, &moneyPlay, &index, false);
                                moneyPlay.message.file.info = (audio_play_info_t *)calloc(index, sizeof(audio_play_info_t));
                                index = 0;
                                strToFile(money->valuestring, &moneyPlay, &index, true);
                                moneyPlay.message.file.count = index;
                            }
                        }
                    }
                    else
                    {
                        if(strlen(money->valuestring) > 8)
                        {
                            moneyPlay.message.file.info = (audio_play_info_t *)calloc(1, sizeof(audio_play_info_t));
                            moneyPlay.message.file.info->address = audioshoukuanchenggong;
                            moneyPlay.message.file.info->rom_data_len = audioshoukuanchenggongSize;
                            moneyPlay.message.file.count = 1;
                        }
                        else
                        {
                            //Call strToFile to format the amount into the corresponding file broadcast data. It needs to be called twice. The first time to obtain the space that requires malloc, and the second time to put the file data into the space.
                            int index = 0;
                            strToFile(money->valuestring, &moneyPlay, &index, false);
                            moneyPlay.message.file.info = (audio_play_info_t *)calloc(index, sizeof(audio_play_info_t));
                            index = 0;
                            strToFile(money->valuestring, &moneyPlay, &index, true);
                            moneyPlay.message.file.count = index;
                        }
                    }
                    if (-1 == luat_rtos_queue_send(audio_queue_handle, &moneyPlay, NULL, 0)){
                        free(moneyPlay.message.file.info);
                        LUAT_DEBUG_PRINT("cloud_speaker_mqtt sub queue send error");
                    }
                }
                else
                {
                    LUAT_DEBUG_PRINT("cloud_speaker_mqtt money data is invalid %d", cJSON_IsString(money));
                }
            }
            cJSON_Delete(boss);
        }
		break;
	}
	case MQTT_MSG_PUBACK : 
	case MQTT_MSG_PUBCOMP : {
		LUAT_DEBUG_PRINT("msg_id: %d",mqtt_parse_msg_id(luat_mqtt_ctrl->mqtt_packet_buffer));
		break;
	}
	default:
		break;
	}
	return;
}

static void luat_mqtt_task(void *param)
{
	int ret = -1;
	luat_mqtt_ctrl = (luat_mqtt_ctrl_t *)luat_heap_malloc(sizeof(luat_mqtt_ctrl_t));
	ret = luat_mqtt_init(luat_mqtt_ctrl, NW_ADAPTER_INDEX_LWIP_GPRS);
	if (ret) {
		LUAT_DEBUG_PRINT("mqtt init FAID ret %d", ret);
		return 0;
	}
	luat_mqtt_ctrl->ip_addr.type = 0xff;
	luat_mqtt_connopts_t opts = {0};

#if (MQTT_DEMO_SSL == 1)
	opts.is_tls = 1;
	opts.server_cert = testCaCrt;
	opts.server_cert_len = strlen(testCaCrt);
	opts.client_cert = testclientCert;
	opts.client_cert_len = strlen(testclientCert);
	opts.client_key = testclientPk;
	opts.client_key_len = strlen(testclientPk);
#else
	opts.is_tls = 0;
#endif 
	opts.host = MQTT_HOST;
	opts.port = MQTT_PORT;
	ret = luat_mqtt_set_connopts(luat_mqtt_ctrl, &opts);

    char str[32] = {0};
    char clientId[17] = {0};
    char username[17] = {0};
    char password[17] = {0};
    ret = luat_fskv_get("clientId", str, 17);             //Read clientId from database
    if(ret > 0 )
    {
        memcpy(clientId, str, 16);                      //Leave one bit to ensure that there is 0x00 at the end of the string
        mqtt_init(&(luat_mqtt_ctrl->broker), clientId);
    }
    else                                                //The clientId has not been written in the database, get the setting
    {
        int result = 0;
        result = luat_mobile_get_imei(0, clientId, 15); //imei is 15 bits, leaving a space for 0x00
        if(result <= 0)
        {
            mqtt_init(&(luat_mqtt_ctrl->broker), CLIENTID);
            LUAT_DEBUG_PRINT("cloud_speaker_mqtt clientid get fail");
        }
        else
        {
            mqtt_init(&(luat_mqtt_ctrl->broker), clientId);
            LUAT_DEBUG_PRINT("cloud_speaker_mqtt clientid get success %s", clientId);
        }       
    }
    memset(str, 0, 32);
    ret = luat_fskv_get("username", str, 17);             //Read username from the database, if not read
    if(ret > 0 )
    {
        memcpy(username, str, 16);                      //Leave one bit to ensure that there is 0x00 at the end of the string
        // connectData.username.cstring = username;
    }
    memset(str, 0, 32);
    ret = luat_fskv_get("password", str, 17);             //Read the password from the database, if not read
    if(ret > 0 )
    {
        memcpy(password, str, 16);                      //Leave one bit to ensure that there is 0x00 at the end of the string
        // connectData.password.cstring = password;
    }

    memset(mqtt_sub_topic, 0x00, sizeof(mqtt_sub_topic));
    snprintf(mqtt_sub_topic, 40, "%s%s", mqtt_sub_topic_head, clientId);
    LUAT_DEBUG_PRINT("cloud_speaker_mqtt subscribe_topic %s %s %s %s", mqtt_sub_topic, clientId, username, password);

	mqtt_init_auth(&(luat_mqtt_ctrl->broker), username, password);

	// luat_mqtt_ctrl->netc->is_debug = 1;// debug信息
	luat_mqtt_ctrl->broker.clean_session = 1;
	luat_mqtt_ctrl->keepalive = 240;

	luat_mqtt_ctrl->reconnect = 1;
	luat_mqtt_ctrl->reconnect_time = 3000;
	
	luat_mqtt_set_cb(luat_mqtt_ctrl,luat_mqtt_cb);

	luat_rtos_task_sleep(3000);
	LUAT_DEBUG_PRINT("mqtt_connect");
	ret = luat_mqtt_connect(luat_mqtt_ctrl);
	if (ret) {
		LUAT_DEBUG_PRINT("mqtt connect ret=%d\n", ret);
		luat_mqtt_close_socket(luat_mqtt_ctrl);
		return;
	}
	LUAT_DEBUG_PRINT("mqtt_connect ok");

	while(1){
        LUAT_DEBUG_PRINT("get_server_status link status %d, %d", get_server_status(), get_net_status());
		luat_rtos_task_sleep(5000);
	}
}

static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	if (LUAT_MOBILE_EVENT_NETIF == event)
	{
		if (LUAT_MOBILE_NETIF_LINK_ON == status)
		{
            g_s_is_link_up = 1;
            luat_socket_check_ready(index, NULL);
		}
        else if(LUAT_MOBILE_NETIF_LINK_OFF == status || LUAT_MOBILE_NETIF_LINK_OOS == status)
        {
            g_s_is_link_up = 0;
        }
	}
}

static void luat_mqtt_publish_task(void *args)
{
    uint32_t id = 0;
    mqtt_publish_para_t *para = NULL;
	while(1)
	{
		if(0 == luat_rtos_message_recv(mqtt_publish_task_handle, &id, (void **)&para, LUAT_WAIT_FOREVER))
		{
            uint8_t result = 0;
			if(para != NULL)
            {
                if (1 == g_s_is_link_up)
                {
                    if (luat_mqtt_ctrl->mqtt_state == MQTT_STATE_READY)
                    {
                        int ret = -2;
                        uint32_t message_id = 0;
                        switch (id)
                        {
                        case TOPIC_1:
                            ret = mqtt_publish_with_qos(&(luat_mqtt_ctrl->broker), mqtt_topic_1, para->msg, para->msg_len, para->retain, para->qos, &message_id);
                            break;
                        case TOPIC_2:
                            ret = mqtt_publish_with_qos(&(luat_mqtt_ctrl->broker), mqtt_topic_2, para->msg, para->msg_len, para->retain, para->qos, &message_id);
                            break;
                        case TOPIC_3:
                            ret = mqtt_publish_with_qos(&(luat_mqtt_ctrl->broker), mqtt_topic_3, para->msg, para->msg_len, para->retain, para->qos, &message_id);
                            break;
                        default:
                            break;
                        }
                        if (1 == ret)
                            ret = MQTT_PUBLISH_SUCCESS;
                        else
                            ret = MQTT_PUBLISH_FAIL;
                    }
                else
                {
                    result = MQTT_NOT_READY;
                }
                }
                else
                {
                    result = NETWORK_NOT_READY;
                }
                if(para->callback != NULL)
                {
                    para->callback(result);
                }

                luat_heap_free(para->msg);
                para->msg = NULL;
                
                luat_heap_free(para);
                para = NULL;
            }
        }
	}
}

static void luat_libemqtt_init(void)
{
	luat_mobile_event_register_handler(luatos_mobile_event_callback);
	net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
	luat_rtos_task_create(&mqtt_task_handle, 2 * 1024, 30, "libemqtt", luat_mqtt_task, NULL, 16);
    luat_rtos_task_create(&mqtt_publish_task_handle, 2 * 1024, 20, "libemqtt pulish task", luat_mqtt_publish_task, NULL, 16);
}

extern void fdb_init(void);
extern void led_task_init(void);
extern void key_task_init(void);
extern void charge_task_init(void);
extern void usb_uart_init(void);
extern void mqtt_send_message_init(void);

INIT_HW_EXPORT(fdb_init, "1");
INIT_TASK_EXPORT(luat_libemqtt_init, "1");
INIT_TASK_EXPORT(audio_task_init, "2");
INIT_TASK_EXPORT(led_task_init, "2");
INIT_TASK_EXPORT(key_task_init, "3");
INIT_TASK_EXPORT(charge_task_init, "2");
INIT_TASK_EXPORT(usb_uart_init, "2");
INIT_TASK_EXPORT(mqtt_send_message_init, "2");


