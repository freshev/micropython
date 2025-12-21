/** This routine is suitable for POSIX devices such as `Linux` that support tasks. It demonstrates using the SDK to configure MQTT parameters and establish a connection, and then create 2 threads
 *
 * + One thread for keep-alive connections
 * + A thread is used to receive messages, and enters the default data callback when a message arrives, and enters the event callback when the connection status changes
 *
 * The parts that require user attention or modification have been marked in comments with TODO
 **/
#include "common_api.h"
#include <stdio.h>
#include <string.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "luat_mobile.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "cJSON.h"
#include "http_queue.h"
#include "audio_task.h"

// Alibaba Cloud system comes with digital corpus
const char *tone_0 = "SYS_TONE_0.amr";
const char *tone_1 = "SYS_TONE_1.amr";
const char *tone_2 = "SYS_TONE_2.amr";
const char *tone_3 = "SYS_TONE_3.amr";
const char *tone_4 = "SYS_TONE_4.amr";
const char *tone_5 = "SYS_TONE_5.amr";
const char *tone_6 = "SYS_TONE_6.amr";
const char *tone_7 = "SYS_TONE_7.amr";
const char *tone_8 = "SYS_TONE_8.amr";
const char *tone_9 = "SYS_TONE_9.amr";

/*TODO: Replace with the triplet of your own device*/
char *product_key       = "${YourProductKey}";
char *device_name       = "${YourDeviceName}";
char *device_secret     = "${YourDeviceSecret}";
extern luat_rtos_queue_t http_queue_handle;
extern luat_rtos_queue_t audio_queue_handle;
extern bool http_get_status;  // This is just an example. There is no processing for multiple dynamic messages. Simply use a variable to control it.
/*TODO: Replace with the access point of your own instance

    For enterprise instances, or public instances under the Internet of Things platform service opened after July 30, 2021 (including that day)
    The format of mqtt_host is "${YourInstanceId}.mqtt.iothub.aliyuncs.com"
    Where ${YourInstanceId}: Please replace it with the Id of your enterprise/public instance

    For public instances under the Internet of Things platform service opened before July 30, 2021 (not including the same day)
    You need to modify mqtt_host to: mqtt_host = "${YourProductKey}.iot-as-mqtt.${YourRegionId}.aliyuncs.com"
    Among them, ${YourProductKey}: Please replace it with the ProductKey of the product to which the device belongs. You can log in to the IoT platform console and obtain it from the device details page of the corresponding instance.
    ${YourRegionId}: Please replace it with the region code where your IoT platform device is located, such as cn-shanghai, etc.
    Complete mqtt_host example in this case: a1TTmBPIChA.iot-as-mqtt.cn-shanghai.aliyuncs.com

    For details, please see: https://help.aliyun.com/document_detail/147356.html*/
char  *mqtt_host = "${YourInstanceId}.mqtt.iothub.aliyuncs.com";

/*A collection of system adaptation functions located in the portfiles/aiot_port folder*/
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/*Server certificate located in external/ali_ca_cert.c*/
extern const char *ali_ca_cert;

static luat_rtos_task_handle g_mqtt_process_thread = NULL;
static luat_rtos_task_handle g_mqtt_recv_thread = NULL;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

luat_rtos_semaphore_t net_semaphore_handle;

static luat_rtos_task_handle linksdk_task_handle;

// Format the amount string into a single file
int fomatMoney(int num, audioQueueData *data, int *index, bool flag)
{
    uint32_t audioArray[10] =
        {
            tone_0,
            tone_1,
            tone_2,
            tone_3,
            tone_4,
            tone_5,
            tone_6,
            tone_7,
            tone_8,
            tone_9,
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
            data->file.info[*index].path = audioArray[thousand];
        }
        *index += 1;
        if (flag)
        {
            data->file.info[*index].path = "SYS_TONE_MEASURE_WORD_qian.amr";
        }
        *index += 1;
    }
    if (hundred != -1)
    {
        if (flag)
        {
            data->file.info[*index].path = audioArray[hundred];
        }
        *index += 1;
        if (flag)
        {
            data->file.info[*index].path = "SYS_TONE_MEASURE_WORD_bai.amr";
        }
        *index += 1;
    }
    if (ten != -1)
    {
        if (!(ten == 1 && hundred == -1 && thousand == -1))
        {
            if (flag)
            {
                data->file.info[*index].path = audioArray[ten];
            }
            *index += 1;
        }
        if (ten != 0)
        {
            if (flag)
            {
                data->file.info[*index].path = "SYS_TONE_MEASURE_WORD_shi.amr";
            }
            *index += 1;
        }
    }
    if (unit != -1)
    {
        if (flag)
        {
            data->file.info[*index].path = audioArray[unit];
        }
        *index += 1;
    }
    return 0;
}
// Format the amount string into a single file
static int strToFile(char *money, audioQueueData *data, int *index, bool flag)
{
    uint32_t audioArray[10] =
        {
            tone_0,
            tone_1,
            tone_2,
            tone_3,
            tone_4,
            tone_5,
            tone_6,
            tone_7,
            tone_8,
            tone_9,
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
            data->file.info[*index].path = "SYS_TONE_MEASURE_WORD_wan.amr";
        }
        *index += 1;
        if (((integer % 10000) < 1000) && ((integer % 10000) != 0))
        {
            if (flag)
            {
                data->file.info[*index].path = audioArray[0];
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
            data->file.info[*index].path = audioArray[0];
        }
        *index += 1;
    }
    int decial = atoi(decStr);
    if (decial > 0)
    {
        if (flag)
        {
            data->file.info[*index].path = "SYS_TONE_dian.amr";
        }
        *index += 1;
        if (decial > 10)
        {

            int ten = decial / 10;
            int unit = decial % 10;
            LUAT_DEBUG_PRINT("this is decial %d, %d, %d", decial, ten, unit);
            if (ten != 0 && unit != 0)
            {
                if (flag)
                {
                    data->file.info[*index].path = audioArray[ten];
                }
                *index += 1;
                if (flag)
                {
                    data->file.info[*index].path = audioArray[unit];
                }
                *index += 1;
            }
            else if (ten == 0 && unit != 0)
            {
                if (flag)
                {
                    data->file.info[*index].path = audioArray[0];
                }
                *index += 1;
                if (flag)
                {
                    data->file.info[*index].path = audioArray[unit];
                }
                *index += 1;
            }
            else if (ten != 0 && unit == 0)
            {
                if (flag)
                {
                    data->file.info[*index].path = audioArray[0];
                }
                *index += 1;
            }
        }
        else
        {
            if (flag)
            {
                data->file.info[*index].path = audioArray[decial];
            }
            *index += 1;
        }
    }
    if (flag)
    {
        data->file.info[*index].path = "SYS_TONE_MONETARY_yuan.amr";
    }
    *index += 1;
    return count;
}

void mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
    if (event == LUAT_MOBILE_EVENT_NETIF && status == LUAT_MOBILE_NETIF_LINK_ON)
    {
        LUAT_DEBUG_PRINT("netif acivated");
        luat_rtos_semaphore_release(net_semaphore_handle);
    }
}

/*TODO: If you want to close the log, make this function empty. If you want to reduce the log, you can choose not to print according to the code.
 *
 * For example: [1577589489.033][LK-0317] mqtt_basic_demo&gb80sFmX7yX
 *
 * The code of the above log is 0317 (hexadecimal). For the definition of code value, see core/aiot_state_api.h
 **/

/*Log callback function, SDK logs will be output from here*/
int32_t demo_state_logcb(int32_t code, char *message)
{
    LUAT_DEBUG_PRINT("linksdk message: %s", message);
    return 0;
}

/*MQTT event callback function, which is triggered when the network is connected/reconnected/disconnected. For the event definition, see core/aiot_mqtt_api.h*/
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type)
    {
    /*SDK has successfully established a connection with the mqtt server because the user called the aiot_mqtt_connect() interface.*/
    case AIOT_MQTTEVT_CONNECT:
    {
        LUAT_DEBUG_PRINT("AIOT_MQTTEVT_CONNECT\n");
        /*TODO: When processing the SDK connection establishment successfully, long time-consuming blocking functions cannot be called here.*/
    }
    break;

    /*After the SDK was passively disconnected due to network conditions, it automatically initiated reconnection successfully.*/
    case AIOT_MQTTEVT_RECONNECT:
    {
        LUAT_DEBUG_PRINT("AIOT_MQTTEVT_RECONNECT\n");
        /*TODO: Handle SDK reconnection successfully, long time-consuming blocking functions cannot be called here*/
    }
    break;

    /*The SDK passively disconnected due to network conditions. Network failed to read and write at the bottom layer. Heartbeat did not get the server heartbeat response as expected.*/
    case AIOT_MQTTEVT_DISCONNECT:
    {
        char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") : ("heartbeat disconnect");
        LUAT_DEBUG_PRINT("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
        /*TODO: Handle SDK passive disconnection, long time-consuming blocking functions cannot be called here*/
    }
    break;

    default:
    {
    }
    }
}

/*MQTT default message processing callback, which is called when the SDK receives an MQTT message from the server and there is no corresponding user callback processing*/
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type)
    {
    case AIOT_MQTTRECV_HEARTBEAT_RESPONSE:
    {
        LUAT_DEBUG_PRINT("heartbeat response\n");
        /*TODO: Process the server's response to the heartbeat, generally not processed*/
    }
    break;

    case AIOT_MQTTRECV_SUB_ACK:
    {
        LUAT_DEBUG_PRINT("suback, res: -0x%04X, packet id: %d, max qos: %d\n",
                         -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        /*TODO: Process the server's response to the subscription request, generally not processed*/
    }
    break;

    case AIOT_MQTTRECV_PUB:
    {
        LUAT_DEBUG_PRINT("pub, qos: %d, topic: %.*s\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
        LUAT_DEBUG_PRINT("pub, payload: %.*s\n", packet->data.pub.payload_len, packet->data.pub.payload);
        /*Process business messages sent by the server*/
        // TODO: Replace with your own project key and device name
        // This topic is a corpus download topic. When Alibaba Cloud pushes the corpus, it will issue a URL to download the json file. This json contains the corpus download address.
        if (strcmp("/sys/${YourProductKey}/${YourDeviceName}/thing/service/SpeechPost", packet->data.pub.topic) == 0)
        {
            cJSON *boss = NULL;
            boss = cJSON_Parse((const char *)packet->data.pub.payload);
            if (boss == NULL)
            {
                LUAT_DEBUG_PRINT("PARES FAIL");
            }
            else
            {
                LUAT_DEBUG_PRINT("this is boss type %d", boss->type);
                cJSON *method = cJSON_GetObjectItem(boss, "method");
                cJSON *id = cJSON_GetObjectItem(boss, "id");
                cJSON *version = cJSON_GetObjectItem(boss, "version");
                cJSON *params = cJSON_GetObjectItem(boss, "params");

                cJSON *url = cJSON_GetObjectItem(params, "url");
                http_queue_t send_http_url = {0};
                send_http_url.url = malloc(strlen(url->valuestring) + 1);
                send_http_url.type = SPEECH_POST;
                memset(send_http_url.url, 0x00, strlen(url->valuestring) + 1);
                memcpy(send_http_url.url, url->valuestring, strlen(url->valuestring));
                if (-1 == luat_rtos_queue_send(http_queue_handle, &send_http_url, NULL, 0))
                {
                    free(send_http_url.url);
                    LUAT_DEBUG_PRINT("http send requet fail");
                }
                LUAT_DEBUG_PRINT("this is url %s", url->valuestring);
            }
            cJSON_Delete(boss);
        }
        // TODO: Replace with your own project key and device name
        /*This topic is an audio broadcast topic. Alibaba Cloud pushes audio broadcast and will send a message to this topic. Only the amount is analyzed here.
        The delivered content must be "{$number}", where number is a string not greater than 99999999.99
        For example, "{$10000.11}", the device will broadcast ten thousand points and one dollar.*/
        else if (strcmp("/sys/${YourProductKey}/${YourDeviceName}/thing/service/SpeechBroadcast", packet->data.pub.topic) == 0)
        {
            cJSON *boss = NULL;
            boss = cJSON_Parse((const char *)packet->data.pub.payload);
            if (boss == NULL)
            {
                LUAT_DEBUG_PRINT("PARES FAIL");
            }
            else
            {
                cJSON *method = cJSON_GetObjectItem(boss, "method");
                cJSON *params = cJSON_GetObjectItem(boss, "params");
                cJSON *speech = cJSON_GetObjectItem(params, "speechs");
                int array_cnt = cJSON_GetArraySize(speech);
                char *head = NULL;
                char *tail = NULL;
                for (int i = 0; i < array_cnt; i++)
                {
                    cJSON *money = cJSON_GetArrayItem(speech, i);
                    head = strstr(money->valuestring, "{$");
                    if (head != NULL)
                    {
                        head++;
                        head++;
                        tail = strstr(head, "}");
                    }
                }
                if (tail != NULL)
                {
                    audioQueueData moneyPlay = {0};
                    int index = 0;
                    char moneyValue[20] = {0};
                    memcpy(moneyValue, head, tail - head);
                    strToFile(moneyValue, &moneyPlay, &index, false);
                    moneyPlay.file.info = (audio_play_info_t *)calloc(index, sizeof(audio_play_info_t));
                    index = 0;
                    strToFile(moneyValue, &moneyPlay, &index, true);
                    moneyPlay.file.count = index;
                    if (-1 == luat_rtos_queue_send(audio_queue_handle, &moneyPlay, NULL, 0))
                    {
                        free(moneyPlay.file.info);
                        LUAT_DEBUG_PRINT("cloud_speaker_mqtt sub queue send error");
                    }
                }
            }
            cJSON_Delete(boss);
        }
        // TODO: Replace with your own project key and device name
        /*This topic is a dynamic audio broadcast topic. Alibaba Cloud will send an audio download URL to this topic when pushing the audio broadcast.
        Parse the url here and send it to httptask to download the audio
        This example will delete the audio after playing it.*/
        else if (strcmp("/sys/${YourProductKey}/${YourDeviceName}/thing/service/AudioPlayback", packet->data.pub.topic) == 0)
        {
            if (!http_get_status)
            {
                cJSON *boss = NULL;
                boss = cJSON_Parse((const char *)packet->data.pub.payload);
                if (boss == NULL)
                {
                    LUAT_DEBUG_PRINT("PARES FAIL");
                }
                else
                {
                    cJSON *params = cJSON_GetObjectItem(boss, "params");
                    cJSON *format = cJSON_GetObjectItem(params, "format");
                    cJSON *id = cJSON_GetObjectItem(params, "id");
                    cJSON *url = cJSON_GetObjectItem(params, "url");
                    http_queue_t send_http_url = {0};
                    send_http_url.type = SPEECH_BY_SYNTHESIS;
                    send_http_url.filename = malloc(strlen(id->valuestring) + strlen(format->valuestring) + 3);
                    memset(send_http_url.filename, 0x00, strlen(id->valuestring) + strlen(format->valuestring) + 3);
                    snprintf(send_http_url.filename, strlen(id->valuestring) + strlen(format->valuestring) + 3, "%s%s%s", id->valuestring, ".", format->valuestring);

                    send_http_url.url = malloc(strlen(url->valuestring) + 1);

                    memset(send_http_url.url, 0x00, strlen(url->valuestring) + 1);
                    memcpy(send_http_url.url, url->valuestring, strlen(url->valuestring) + 1);

                    if (-1 == luat_rtos_queue_send(http_queue_handle, &send_http_url, NULL, 0))
                    {
                        free(send_http_url.filename);
                        free(send_http_url.url);
                        LUAT_DEBUG_PRINT("http send requet fail");
                    }

                    LUAT_DEBUG_PRINT("this is url %s", url->valuestring);
                    http_get_status = true;
                }
                cJSON_Delete(boss);
            }
        }
    }
    break;

    case AIOT_MQTTRECV_PUB_ACK:
    {
        LUAT_DEBUG_PRINT("puback, packet id: %d\n", packet->data.pub_ack.packet_id);
        /*TODO: Process the server's response to the QoS1 reported message, generally not processed*/
    }
    break;

    default:
    {
    }
    }
}

/*Thread that executes aiot_mqtt_process, including heartbeat sending and QoS1 message resending*/
void *demo_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running)
    {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED)
        {
            break;
        }
        luat_rtos_task_sleep(1000);
    }
    luat_rtos_task_delete(g_mqtt_process_thread);
    return NULL;
}

/*The thread that executes aiot_mqtt_recv, including automatic network reconnection and receiving MQTT messages from the server*/
void *demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running)
    {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS)
        {
            if (res == STATE_USER_INPUT_EXEC_DISABLED)
            {
                break;
            }
            luat_rtos_task_sleep(1);
        }
    }
    luat_rtos_task_delete(g_mqtt_recv_thread);
    return NULL;
}

int linksdk_mqtt_task(void *param)
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;
    uint16_t port = 443;             /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
    aiot_sysdep_network_cred_t cred; /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/

    luat_rtos_semaphore_create(&net_semaphore_handle, 1);

    luat_mobile_event_register_handler(mobile_event_callback);

    luat_rtos_semaphore_take(net_semaphore_handle, LUAT_WAIT_FOREVER);
    /*Configure the underlying dependencies of the SDK*/
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /*Configure the log output of the SDK*/
    aiot_state_set_logcb(demo_state_logcb);

    /*Create security credentials for the SDK, used to establish TLS connections*/
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA; /*Use RSA certificate to verify MQTT server*/
    cred.max_tls_fragment = 16384;                     /*The maximum fragment length is 16K, other optional values     are 4K, 2K, 1K, 0.5K*/
    cred.sni_enabled = 1;                              /*When establishing a TLS connection, support Server Name Indicator*/
    cred.x509_server_cert = ali_ca_cert;               /*RSA root certificate used to verify the MQTT server*/
    cred.x509_server_cert_len = strlen(ali_ca_cert);   /*RSA root certificate length used to verify the MQTT server*/

    /*Create 1 MQTT client instance and initialize default parameters internally*/
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL)
    {
        LUAT_DEBUG_PRINT("aiot_mqtt_init failed\n");
        luat_rtos_task_delete(linksdk_task_handle);
        return -1;
    }

    /*TODO: If the following code is not commented, the routine will use TCP instead of TLS to connect to the cloud platform*/

    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }

    /*Configure MQTT server address*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)mqtt_host);
    /*Configure MQTT server port*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /*Configure device productKey*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /*Configure device deviceName*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /*Configure device deviceSecret*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /*Configure the security credentials for the network connection, which have been created above*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /*Configure MQTT default message receiving callback function*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /*Configure MQTT event callback function*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /*Establish an MQTT connection with the server*/
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS)
    {
        /*The attempt to establish a connection failed, the MQTT instance was destroyed, and the resources were recycled.*/
        aiot_mqtt_deinit(&mqtt_handle);
        LUAT_DEBUG_PRINT("aiot_mqtt_connect failed: -0x%04X\n\r\n", -res);
        LUAT_DEBUG_PRINT("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        luat_rtos_task_delete(linksdk_task_handle);
        return -1;
    }

    /*MQTT subscription topic function example, please use it according to your business needs*/
    /* {
        char *sub_topic = "/sys/${YourProductKey}/${YourDeviceName}/thing/event/+/post_reply";
        res = aiot_mqtt_sub(mqtt_handle, sub_topic, NULL, 1, NULL);
        if (res < 0) {
            DBG("aiot_mqtt_sub failed, res: -0x%04X\n", -res);
            vTaskDelete(NULL);
            return -1;
        }
    } */

    /*MQTT message publishing function example, please use it according to your business needs*/
    /* {
        char *pub_topic = "/sys/${YourProductKey}/${YourDeviceName}/thing/event/property/post";
        char *pub_payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";
        res = aiot_mqtt_pub(mqtt_handle, pub_topic, (uint8_t *)pub_payload, (uint32_t)strlen(pub_payload), 0);
        if (res < 0) {
            LUAT_DEBUG_PRINT("aiot_mqtt_sub failed, res: -0x%04X\n", -res);
            luat_rtos_task_delete(linksdk_task_handle);
            return -1;
        }
    } */

    /*Create a separate thread dedicated to executing aiot_mqtt_process. It will automatically send heartbeat keep-alives and resend QoS1 unanswered messages.*/
    g_mqtt_process_thread_running = 1;
    luat_rtos_task_create(&g_mqtt_process_thread, 4096, 20, "", demo_mqtt_process_thread, mqtt_handle, NULL);
    if (g_mqtt_process_thread == NULL)
    {
        LUAT_DEBUG_PRINT("task_create demo_mqtt_process_thread failed: %d\n", res);
        luat_rtos_task_delete(linksdk_task_handle);
        return -1;
    }

    /*Create a separate thread for executing aiot_mqtt_recv, which will cycle through the MQTT messages sent by the server and automatically reconnect when disconnected*/
    g_mqtt_recv_thread_running = 1;
    luat_rtos_task_create(&g_mqtt_recv_thread, 4096, 20, "", demo_mqtt_recv_thread, mqtt_handle, NULL);
    if (g_mqtt_recv_thread == NULL)
    {
        LUAT_DEBUG_PRINT("task_create demo_mqtt_recv_thread failed: %d\n", res);
        luat_rtos_task_delete(linksdk_task_handle);
        return -1;
    }

    /*The main loop goes to sleep*/
    while (1)
    {
        luat_rtos_task_sleep(1000);
    }

    /*Disconnect the MQTT connection, generally it will not run here*/
    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    luat_rtos_task_sleep(1);

    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS)
    {
        aiot_mqtt_deinit(&mqtt_handle);
        LUAT_DEBUG_PRINT("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        luat_rtos_task_delete(linksdk_task_handle);
        return -1;
    }

    /*Destroy the MQTT instance, generally it will not run here*/
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS)
    {
        LUAT_DEBUG_PRINT("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        luat_rtos_task_delete(linksdk_task_handle);
        return -1;
    }
    luat_rtos_task_delete(linksdk_task_handle);
    return 0;
}

static void task_demo_init(void)
{
    luat_rtos_task_create(&linksdk_task_handle, 4096, 20, "", linksdk_mqtt_task, NULL, NULL);
}
extern void task_demo_https(void);
extern void audio_task_init(void);
INIT_TASK_EXPORT(audio_task_init, "1");
INIT_TASK_EXPORT(task_demo_init, "1");
INIT_TASK_EXPORT(task_demo_https, "1");
