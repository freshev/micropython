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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ps_event_callback.h"
#include "ps_lib_api.h"
#include "cmisim.h"
#include "cmips.h"
#include "slpman.h"

#include "MQTTClient.h"

#include "cJSON.h"
#include "gpio.h"
#include "cmimm.h"


char g_mqtt_MQTT_HOST[192];    // "120.55.137.106" // MQTT server address and port number
uint32_t g_mqtt_MQTT_PORT = 1883;
char g_mqtt_CLIENT_ID[192];
char g_mqtt_USERNAME[192];
char g_mqtt_PASSWORD[192];

char g_mqtt_sub_topic[256];
char g_mqtt_pub_topic[256];

#define NETLED_ID 27
#define NETLED_PAD (NETLED_ID / 16)
#define NETLED_MARK (NETLED_ID % 16)
//char g_mqtt_send_payload[] = "hello mqtt_test!!!";

#define QMSG_ID_BASE               (0x160) 
#define QMSG_ID_NW_IPV4_READY      (QMSG_ID_BASE)
#define QMSG_ID_NW_IPV6_READY      (QMSG_ID_BASE + 1)
#define QMSG_ID_NW_IPV4_6_READY    (QMSG_ID_BASE + 2)
#define QMSG_ID_NW_DISCONNECT      (QMSG_ID_BASE + 3)
#define QMSG_ID_SOCK_SENDPKG       (QMSG_ID_BASE + 4)
#define QMSG_ID_SOCK_RECVPKG       (QMSG_ID_BASE + 5)


#define APP_EVENT_QUEUE_SIZE    (10)
static QueueHandle_t            psEventQueueHandle;
static QueueHandle_t            psMqttPubQueue;

int mqtt_uplink(void* payload, size_t len, int qos, int retained);

static void sendQueueMsg(UINT32 msgId, UINT32 xTickstoWait){
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (psEventQueueHandle){
        if (pdTRUE != xQueueSend(psEventQueueHandle, &queueMsg, xTickstoWait)){
            DBG("xQueueSend error");
        }
    }
}

static INT32 mqttPSUrcCallback(PsEventID eventID, void *param, UINT32 paramLen){
    CmiSimImsiStr *imsi = NULL;
    CmiPsCeregInd *creg = NULL;
    UINT8 rssi = 0;
    NmAtiNetInfoInd *netif = NULL;

    switch(eventID){
        case PS_URC_ID_SIM_READY:{
            imsi = (CmiSimImsiStr *)param;
            DBG("SIM ready(imsi=%s len=%d)", imsi->contents, imsi->length);
            break;
        }
        case PS_URC_ID_MM_SIGQ:{
            CmiMmCesqInd *pMmCesqInd = (CmiMmCesqInd *)param;
            rssi = mmGetCsqRssiFromCesq(pMmCesqInd->rsrp, pMmCesqInd->rsrq, pMmCesqInd->rssiCompensation);
            DBG("RSSI signal=%d", rssi);
            break;
        }
        case PS_URC_ID_PS_BEARER_ACTED:{
            DBG("Default bearer activated");
            break;
        }
        case PS_URC_ID_PS_BEARER_DEACTED:{
            DBG("Default bearer Deactivated");
            sendQueueMsg(QMSG_ID_NW_DISCONNECT, 0);
            break;
        }
        case PS_URC_ID_PS_CEREG_CHANGED:{
            creg = (CmiPsCeregInd *)param;
            DBG("CREG message act:%d celId:%d locPresent:%d state:%d", creg->act, creg->celId, creg->locPresent, creg->state);
            break;
        }
        case PS_URC_ID_PS_NETINFO:{
            netif = (NmAtiNetInfoInd *)param;
            if (netif->netifInfo.netStatus == NM_NETIF_ACTIVATED){
                DBG("netif acivated");
                sendQueueMsg(QMSG_ID_NW_IPV4_READY, 0);
            }else if (netif->netifInfo.netStatus == NM_NETIF_OOS){
                DBG("PSIF network OOS");
            }else if (netif->netifInfo.netStatus == NM_NO_NETIF_OR_DEACTIVATED ||
                     netif->netifInfo.netStatus == NM_NO_NETIF_NOT_DIAL){
                DBG("PSIF network deactive");
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

// Downstream information processing function
void messageArrived(MessageData* data)
{
    cJSON* dtop = NULL;
    cJSON* utop = cJSON_CreateObject();
    char* payload = data->message->payload;
    size_t payloadlen = data->message->payloadlen;
	DBG("mqtt Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len, data->topicName->lenstring.data,
		data->message->payloadlen, data->message->payload);
    if (data->message->payloadlen < 2) {
        return;
    }
    if (payload[0] == '{' && payload[payloadlen - 1] == '}') {
        // It is json data, start processing
        dtop = cJSON_ParseWithLength(payload, payloadlen);
        if (dtop != NULL) {
            //The most basic command, netled control
            cJSON* netled = cJSON_GetObjectItem(dtop, "netled");
            if (netled != NULL) {
                if (netled->type == cJSON_False) {
                    DBG("NETLED Off");
                    GPIO_pinWrite(NETLED_PAD, 1 << NETLED_MARK , 0);
                    cJSON_AddBoolToObject(utop, "netled", 0);
                }
                else if (netled->type == cJSON_True) {
                    DBG("NETLED On");
                    GPIO_pinWrite(NETLED_PAD, 1 << NETLED_MARK, 1 << NETLED_MARK);
                    cJSON_AddBoolToObject(utop, "netled", 1);
                }
            }

            // TODO advanced gpio control

            // TODO uart transparent transmission, consider demonstrating it

            // ADC reading, demonstrate

            // PWM output, demonstrate

            //End processing.
        }
    }

    // cJSON data will not be automatically recycled, please pay attention to cleaning it
    if (dtop != NULL) {
        cJSON_Delete(dtop);
    }

    //The data returned by Print also needs to be cleaned
    char* resp = cJSON_Print(utop);
    if (resp) {
        DBG("mqtt resp %s", resp);
        mqtt_uplink(resp, strlen(resp), 1, 0);
        cJSON_free(resp);
    }
}

//Upstream data, interrupt function cannot be called
int mqtt_uplink(void* payload, size_t len, int qos, int retained) {
    MQTTMessage *message = malloc(sizeof(MQTTMessage));
    
    message->qos = qos;
	message->retained = retained;
    message->payload = malloc(len);
    if (message->payload == NULL) {
        return -1;
    }
    memcpy(message->payload, payload, len);
    message->payloadlen = len;
    xQueueSend(psMqttPubQueue, &message, 0);
    return 0;
}

static uint8_t mqtt_ready = 0;

static void mqtt_demo(void){
	int rc = 0;
    MQTTClient mqttClient;
    Network mqttNetwork = {0};
    MQTTMessage *message;
    char imei[20] = {0};

    // TODO supports reconnection
    if (mqtt_ready) {
        DBG("mqtt is ready");
        return;
    }
    mqtt_ready = 1;
    
    // Get IMEI as identification code
    appGetImeiNumSync(imei);

    //Configure various parameters of mqtt
    // TODO should be changed to Hezhou or fdb configurable?
    memcpy(g_mqtt_MQTT_HOST, "broker-cn.emqx.io", strlen("broker-cn.emqx.io") + 1);
    g_mqtt_MQTT_PORT = 1883;
    memcpy(g_mqtt_CLIENT_ID, imei, strlen(imei) + 1);
    memcpy(g_mqtt_USERNAME, imei, strlen(imei) + 1);
    memcpy(g_mqtt_PASSWORD, "LuatOS", strlen("LuatOS") + 1);

    sprintf(g_mqtt_sub_topic, "/webled/%s/down", imei);
    sprintf(g_mqtt_pub_topic, "/webled/%s/up", imei);

    //Print the upstream and downstream topics for easy debugging
    DBG("uplink %s", g_mqtt_pub_topic);
    DBG("downlink %s", g_mqtt_sub_topic);

    // Start connecting
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    connectData.MQTTVersion = 4;
    connectData.clientID.cstring = g_mqtt_CLIENT_ID;
    connectData.username.cstring = g_mqtt_USERNAME;
    connectData.password.cstring = g_mqtt_PASSWORD;
    connectData.keepAliveInterval = 120;

    // Keep trying again until successful
    while (1) {
        DBG("mqtt connect ...");
        rc = mqtt_connect(&mqttClient, &mqttNetwork, g_mqtt_MQTT_HOST, g_mqtt_MQTT_PORT, &connectData);
        if (MQTTIsConnected(&mqttClient)) {
            DBG("mqtt connect ok");
            break;
        }
        DBG("mqtt connect fail, wait 10s");
        osDelay(15*1000);
    }
    
    //The connection is successful, please wait a moment
    osDelay(1000); //Temporarily position for 1 second, then shorten it later^_^

    //Report online notification
    mqtt_uplink("{\"hi\":\"LuatOS\"}", strlen("{\"hi\":\"LuatOS\"}"), 1, 0);

    //Subscribe to downstream topic. TODO encountered failure.
    if ((rc = MQTTSubscribe(&mqttClient, g_mqtt_sub_topic, 0, messageArrived)) != 0) {
        // Subscription failed, actually the problem is quite big
        DBG("mqtt Return code from MQTT subscribe is %d", rc);
    }

    // The rest is waiting for the upstream data. As for the downstream data, MqttTask will handle it
    while(1){
        //Receive queue information, and then go up
        if (xQueueReceive(psMqttPubQueue, &message, 15000)){
            MQTTPublish(&mqttClient, g_mqtt_pub_topic, message);
            free(message->payload);
            free(message);
        }
        
#if !defined(MQTT_TASK)
		if ((rc = MQTTYield(&mqttClient, 1000)) != 0)
			DBG("mqtt_demo Return code from yield is %d\n", rc);
#endif
        osDelay(10);
    }
}


static void mqttclient_task(void *param){
    eventCallbackMessage_t *queueItem = NULL;
    psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t*));
    if (psEventQueueHandle == NULL){
        DBG("psEventQueue create error!");
        return;
    }
    psMqttPubQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(MQTTMessage*));
    if (psMqttPubQueue == NULL){
        DBG("psMqttPubQueue create error!");
        return;
    }
    registerPSEventCallback(PS_GROUP_ALL_MASK, mqttPSUrcCallback);
    while(1){
        if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY)){
            switch(queueItem->messageId){
                case QMSG_ID_NW_IPV4_READY:
                case QMSG_ID_NW_IPV6_READY:
                case QMSG_ID_NW_IPV4_6_READY:
					mqtt_demo();
                    break;
                case QMSG_ID_NW_DISCONNECT:
                    break;
                default:
                    break;
            }
            free(queueItem);
        }
    }
	vTaskDelete(NULL);
}

static void mqttclient_task_init(void){

    //------------------------------------------------------
    // netled is the only visible GPIO of the development board and is initialized by default.
    slpManAONIOPowerOn();

    PadConfig_t padConfig = {0};
    PAD_getDefaultConfig(&padConfig);

    padConfig.mux = PAD_MUX_ALT0;
    PAD_setPinConfig(NETLED_ID, &padConfig);
    PAD_setPinPullConfig(NETLED_ID, PAD_INTERNAL_PULL_DOWN);
    GpioPinConfig_t config = {0};
    config.pinDirection = GPIO_DIRECTION_OUTPUT;
    config.misc.initOutput = 0;

    GPIO_pinConfig(NETLED_PAD, NETLED_MARK, &config);
    //------------------------------------------------------

    //Start mqtt task
	xTaskCreate(mqttclient_task, "mqtt", 4096, NULL, 20, NULL);
}

INIT_TASK_EXPORT(mqttclient_task_init, "2");

