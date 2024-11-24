/** This routine is suitable for POSIX devices such as `Linux` that support pthread. It demonstrates using the SDK to configure MQTT parameters and establish a connection, and then create 2 threads
 *
 * + One thread for keep-alive connections
 * + A thread is used to receive messages, and enters the default data callback when a message arrives, and enters the event callback when the connection status changes
 *
 * Then send the device-side log to the cloud on the MQTT connection. Note that the cloud channel on the log is closed by default. The user needs to go to the cloud console to turn on the device-side log service switch.
 *
 * The parts that require user attention or modification have been marked in comments with TODO
 **/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_logpost_api.h"


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

static pthread_t g_mqtt_process_thread;
static pthread_t g_mqtt_recv_thread;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

/*TODO: If you want to close the log, make this function empty. If you want to reduce the log, you can choose not to print according to the code.
 *
 * For example: [1581501698.455][LK-0309] pub: /sys/a13FN5TplKq/logpost_basic_demo/thing/deviceinfo/update
 *
 * The code of the above log is 0309 (hexadecimal). For the definition of code value, see core/aiot_state_api.h
 **/

/*Log callback function. SDK logs will be output from here. It is prohibited to call the SDK API in this function.*/
static int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/*MQTT event callback function, which is triggered when the network is connected/reconnected/disconnected. For the event definition, see core/aiot_mqtt_api.h*/
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /*SDK has successfully established a connection with the mqtt server because the user called the aiot_mqtt_connect() interface.*/
        case AIOT_MQTTEVT_CONNECT: {
            printf("AIOT_MQTTEVT_CONNECT\r\n");
        }
        break;

        /*After the SDK was passively disconnected due to network conditions, it automatically initiated reconnection successfully.*/
        case AIOT_MQTTEVT_RECONNECT: {
            printf("AIOT_MQTTEVT_RECONNECT\r\n");
        }
        break;

        /*The SDK passively disconnected due to network conditions. Network failed to read and write at the bottom layer. Heartbeat did not get the server heartbeat response as expected.*/
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            printf("AIOT_MQTTEVT_DISCONNECT: %s\r\n", cause);
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
            printf("heartbeat response\r\n");
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            printf("suback, res: -0x%04X, packet id: %d, max qos: %d\r\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            printf("pub, qos: %d, topic: %.*s\r\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            printf("pub, payload: %.*s\r\n", packet->data.pub.payload_len, packet->data.pub.payload);
        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            printf("puback, packet id: %d\r\n", packet->data.pub_ack.packet_id);
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
        sleep(1);
    }
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
            sleep(1);
        }
    }
    return NULL;
}

int32_t demo_mqtt_start(void **handle, char *product_key, char *device_name, char *device_secret)
{
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
        printf("aiot_mqtt_init failed\r\n");
        return -1;
    }

    /*If the following code is not commented, the routine will use TCP instead of TLS to connect to the cloud platform*/
    /*
    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }
    */

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
        printf("aiot_mqtt_connect failed: -0x%04X\r\n\r\n", -res);
        printf("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        return -1;
    }

    /*Create a separate thread dedicated to executing aiot_mqtt_process. It will automatically send heartbeat keep-alives and resend QoS1 unanswered messages.*/
    g_mqtt_process_thread_running = 1;
    res = pthread_create(&g_mqtt_process_thread, NULL, demo_mqtt_process_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_process_thread failed: %d\r\n", res);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

    /*Create a separate thread for executing aiot_mqtt_recv, which will cycle through the MQTT messages sent by the server and automatically reconnect when disconnected*/
    g_mqtt_recv_thread_running = 1;
    res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_recv_thread failed: %d\r\n", res);
        g_mqtt_process_thread_running = 0;
        pthread_join(g_mqtt_process_thread, NULL);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

    *handle = mqtt_handle;

    return 0;
}

int32_t demo_mqtt_stop(void **handle)
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;

    mqtt_handle = *handle;

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    pthread_join(g_mqtt_process_thread, NULL);
    pthread_join(g_mqtt_recv_thread, NULL);

    /*Disconnect MQTT connection*/
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04X\r\n", -res);
        return -1;
    }

    /*Destroy the MQTT instance*/
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04X\r\n", -res);
        return -1;
    }

    return 0;
}

/*Event processing callback, the user can obtain the switch status of the log reporting channel through this callback*/
void demo_logpost_event_handler(void *handle, const aiot_logpost_event_t *event, void *userdata)
{
    switch (event->type) {
        /*Log configuration event. This event will be received when the device is successfully connected to the cloud or when the user controls the log switch on the console page.*/
        case AIOT_LOGPOSTEVT_CONFIG_DATA: {
            printf("user log switch state is: %d\r\n", event->data.config_data.on_off);
            printf("toggle it using the switch in device detail page in https://iot.console.aliyun.com\r\n");
        }
        default:
            break;
    }
}

/*Report logs to the cloud*/
void demo_send_log(void *handle, char *log)
{
    int32_t res = 0;
    aiot_logpost_msg_t msg;

    memset(&msg, 0, sizeof(aiot_logpost_msg_t));
    msg.timestamp = 0;                          /*Timestamp in ms. If you fill in 0, the SDK will use the current timestamp.*/
    msg.loglevel = AIOT_LOGPOST_LEVEL_DEBUG;    /*Log level*/
    msg.module_name = "APP";                    /*The Modules corresponding to the log*/
    msg.code = 200;                             /*status code*/
    msg.msg_id = 0;                             /*The message identifier of the cloud downlink message. If there is no corresponding message, you can directly fill in 0.*/
    msg.content = log;                          /*Log content*/

    res = aiot_logpost_send(handle, &msg);
    if (res < 0) {
        printf("aiot_logpost_send failed: -0x%04X\r\n", -res);
    }
}


int main(int argc, char *argv[])
{
    int32_t     res = STATE_SUCCESS;

    void *mqtt_handle = NULL, *logpost_handle = NULL;
    uint8_t sys_log_switch = 1;

    /*Establish an MQTT connection and start the keepalive thread and receiving thread*/
    res = demo_mqtt_start(&mqtt_handle, product_key, device_name, device_secret);
    if (res < 0) {
        printf("demo_mqtt_start failed\r\n");
        return -1;
    }

    /*Create a logpost client instance and initialize default parameters internally*/
    logpost_handle = aiot_logpost_init();
    if (logpost_handle == NULL) {
        demo_mqtt_stop(&mqtt_handle);
        printf("aiot_logpost_init failed\r\n");
        return -1;
    }

    /*Configure the system log switch of logpost. When turned on, network delay information will be reported.*/
    res = aiot_logpost_setopt(logpost_handle, AIOT_LOGPOSTOPT_SYS_LOG, (void *)&sys_log_switch);
    if (res < STATE_SUCCESS) {
        printf("aiot_logpost_setopt AIOT_LOGPOSTOPT_SYS_LOG failed, res: -0x%04X\r\n", -res);
        aiot_logpost_deinit(&logpost_handle);
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    /*Configure the logpost session and associate it with the MQTT session handle*/
    res = aiot_logpost_setopt(logpost_handle, AIOT_LOGPOSTOPT_MQTT_HANDLE, mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_logpost_setopt AIOT_LOGPOSTOPT_MQTT_HANDLE failed, res: -0x%04X\r\n", -res);
        aiot_logpost_deinit(&logpost_handle);
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    res = aiot_logpost_setopt(logpost_handle, AIOT_LOGPOSTOPT_EVENT_HANDLER, (void *)demo_logpost_event_handler);
    if (res < STATE_SUCCESS) {
        printf("aiot_logpost_setopt AIOT_LOGPOSTOPT_EVENT_HANDLER failed, res: -0x%04X\r\n", -res);
        aiot_logpost_deinit(&logpost_handle);
        demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    /*The main thread goes to sleep, and when the logpost response from the cloud platform arrives, the receiving thread will call demo_logpost_recv_handler()*/
    while (1) {
        sleep(10);

        /*TODO: Users can uncomment and report logs to the cloud. Note: After the log Modules completes initialization, the reporting channel is closed by default. The log Modules will immediately synchronize the switch status of the cloud console after receiving internal events of device connection establishment.
        demo_send_log(logpost_handle, "log in while(1)");*/
    }

    /*Destroy the logpost instance, generally it will not run here.*/
    res = aiot_logpost_deinit(&logpost_handle);
    if (res < STATE_SUCCESS) {
        demo_mqtt_stop(&mqtt_handle);
        printf("aiot_logpost_deinit failed: -0x%04X\r\n", -res);
        return -1;
    }

    /*Destroy the MQTT instance and exit the thread. Generally, it will not run here.*/
    res = demo_mqtt_stop(&mqtt_handle);
    if (res < 0) {
        printf("demo_start_stop failed\r\n");
        return -1;
    }

    return 0;
}

