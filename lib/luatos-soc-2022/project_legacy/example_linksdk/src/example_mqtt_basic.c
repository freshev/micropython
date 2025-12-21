/** This routine is suitable for POSIX devices such as `Linux` that support tasks. It demonstrates using the SDK to configure MQTT parameters and establish a connection, and then create 2 threads
 *
 * + One thread for keep-alive connections
 * + A thread is used to receive messages, and enters the default data callback when a message arrives, and enters the event callback when the connection status changes
 *
 * The parts that require user attention or modification have been marked in comments with TODO
 **/
#include <stdio.h>
#include <string.h>
#include "bsp_common.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "common_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"
#include "ps_event_callback.h"
#include "cmips.h"
#include "networkmgr.h"
#include "cmimm.h"
/*TODO: Replace with the triplet of your own device*/
char *product_key       = "${YourProductKey}";
char *device_name       = "${YourDeviceName}";
char *device_secret     = "${YourDeviceSecret}";

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

static TaskHandle_t g_mqtt_process_thread = NULL;
static TaskHandle_t g_mqtt_recv_thread = NULL;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;
static osEventFlagsId_t waitNetSuccessFlag = NULL;
#define NETSTATUS_NORMAL (0x1)


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
                osEventFlagsSet(waitNetSuccessFlag, NETSTATUS_NORMAL);
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


/*TODO: If you want to close the log, make this function empty. If you want to reduce the log, you can choose not to print according to the code.
 *
 * For example: [1577589489.033][LK-0317] mqtt_basic_demo&gb80sFmX7yX
 *
 * The code of the above log is 0317 (hexadecimal). For the definition of code value, see core/aiot_state_api.h
 **/

/*Log callback function, SDK logs will be output from here*/
int32_t demo_state_logcb(int32_t code, char *message)
{
    DBG("linksdk message: %s", message);
    return 0;
}

/*MQTT event callback function, which is triggered when the network is connected/reconnected/disconnected. For the event definition, see core/aiot_mqtt_api.h*/
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /*SDK has successfully established a connection with the mqtt server because the user called the aiot_mqtt_connect() interface.*/
        case AIOT_MQTTEVT_CONNECT: {
            DBG("AIOT_MQTTEVT_CONNECT\n");
            /*TODO: When processing the SDK connection establishment successfully, long time-consuming blocking functions cannot be called here.*/
        }
        break;

        /*After the SDK was passively disconnected due to network conditions, it automatically initiated reconnection successfully.*/
        case AIOT_MQTTEVT_RECONNECT: {
            DBG("AIOT_MQTTEVT_RECONNECT\n");
            /*TODO: Handle SDK reconnection successfully, long time-consuming blocking functions cannot be called here*/
        }
        break;

        /*The SDK passively disconnected due to network conditions. Network failed to read and write at the bottom layer. Heartbeat did not get the server heartbeat response as expected.*/
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            DBG("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
            /*TODO: Handle SDK passive disconnection, long time-consuming blocking functions cannot be called here*/
        }
        break;

        default: {

        }
    }
}

/*MQTT default message processing callback, which is called when the SDK receives an MQTT message from the server and there is no corresponding user callback processing*/
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            DBG("heartbeat response\n");
            /*TODO: Process the server's response to the heartbeat, generally not processed*/
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            DBG("suback, res: -0x%04X, packet id: %d, max qos: %d\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
            /*TODO: Process the server's response to the subscription request, generally not processed*/
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            DBG("pub, qos: %d, topic: %.*s\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            DBG("pub, payload: %.*s\n", packet->data.pub.payload_len, packet->data.pub.payload);
            /*TODO: Process the business messages sent by the server*/
        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            DBG("puback, packet id: %d\n", packet->data.pub_ack.packet_id);
            /*TODO: Process the server's response to the QoS1 reported message, generally not processed*/
        }
        break;

        default: {

        }
    }
}

/*Thread that executes aiot_mqtt_process, including heartbeat sending and QoS1 message resending*/
void *demo_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        osDelay(1);
    }
    vTaskDelete(NULL);
    return NULL;
}

/*The thread that executes aiot_mqtt_recv, including automatic network reconnection and receiving MQTT messages from the server*/
void *demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            osDelay(1);
        }
    }
    vTaskDelete(NULL);
    return NULL;
}

int linksdk_mqtt_task(void *param)
{
    registerPSEventCallback(PS_GROUP_ALL_MASK, mqttPSUrcCallback);
    waitNetSuccessFlag = osEventFlagsNew(NULL);
    if(waitNetSuccessFlag == NULL)
    {
        DBG("create eventFlag failed");
        vTaskDelete(NULL);
        return -1;
    }
    osEventFlagsWait(waitNetSuccessFlag, NETSTATUS_NORMAL, osFlagsWaitAll, osWaitForever);
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    uint16_t    port = 443;      /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
    aiot_sysdep_network_cred_t cred; /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/

    /*Configure the underlying dependencies of the SDK*/
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /*Configure the log output of the SDK*/
    aiot_state_set_logcb(demo_state_logcb);

    /*Create security credentials for the SDK, used to establish TLS connections*/
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /*Use RSA certificate to verify MQTT server*/
    cred.max_tls_fragment = 16384; /*The maximum fragment length is 16K, other optional values     are 4K, 2K, 1K, 0.5K*/
    cred.sni_enabled = 1;                               /*When establishing a TLS connection, support Server Name Indicator*/
    cred.x509_server_cert = ali_ca_cert;                 /*RSA root certificate used to verify the MQTT server*/
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /*RSA root certificate length used to verify the MQTT server*/

    /*Create 1 MQTT client instance and initialize default parameters internally*/
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        DBG("aiot_mqtt_init failed\n");
        vTaskDelete(NULL);
        return -1;
    }

    /*TODO: If the following code is not commented, the routine will use TCP instead of TLS to connect to the cloud platform*/
    
    /* {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    } */
   

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
    if (res < STATE_SUCCESS) {
        /*The attempt to establish a connection failed, the MQTT instance was destroyed, and the resources were recycled.*/
        aiot_mqtt_deinit(&mqtt_handle);
        DBG("aiot_mqtt_connect failed: -0x%04X\n\r\n", -res);
        DBG("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        vTaskDelete(NULL);
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
            DBG("aiot_mqtt_sub failed, res: -0x%04X\n", -res);
            vTaskDelete(NULL);
            return -1;
        }
    } */

    /*Create a separate thread dedicated to executing aiot_mqtt_process. It will automatically send heartbeat keep-alives and resend QoS1 unanswered messages.*/
    g_mqtt_process_thread_running = 1;
    xTaskCreate(demo_mqtt_process_thread, "", 4096, mqtt_handle, 20, &g_mqtt_process_thread);
    if (g_mqtt_process_thread == NULL) {
        DBG("task_create demo_mqtt_process_thread failed: %d\n", res);
        vTaskDelete(NULL);
        return -1;
    }

    /*Create a separate thread for executing aiot_mqtt_recv, which will cycle through the MQTT messages sent by the server and automatically reconnect when disconnected*/
    g_mqtt_recv_thread_running = 1;
     xTaskCreate(demo_mqtt_recv_thread, "", 4096, mqtt_handle, 20, &g_mqtt_recv_thread);
    if (g_mqtt_recv_thread == NULL) {
        DBG("task_create demo_mqtt_recv_thread failed: %d\n", res);
        vTaskDelete(NULL);
        return -1;
    }

    /*The main loop goes to sleep*/
    while (1) {
        osDelay(1);
    }

    /*Disconnect the MQTT connection, generally it will not run here*/
    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    osDelay(1);

    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        DBG("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        vTaskDelete(NULL);
        return -1;
    }

    /*Destroy the MQTT instance, generally it will not run here*/
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        DBG("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        vTaskDelete(NULL);
        return -1;
    }
    vTaskDelete(NULL);
    return 0;
}



static void task_demo_init(void)
{
	xTaskCreate(linksdk_mqtt_task, "", 4096, NULL, 20, NULL);
}

INIT_TASK_EXPORT(task_demo_init, "1");
