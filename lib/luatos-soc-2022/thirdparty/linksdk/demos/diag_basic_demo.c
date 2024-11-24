/** This routine is suitable for POSIX devices such as `Linux` that support pthread. It demonstrates...
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
#include "aiot_dm_api.h"
#include "aiot_diag_api.h"


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
 * For example: [1577589489.033][LK-0317] diag_basic_demo&gb80sFmX7yX
 *
 * The code of the above log is 0317 (hexadecimal). For the definition of code value, see core/aiot_state_api.h
 **/

/*Log callback function, SDK logs will be output from here*/
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
            printf("AIOT_MQTTEVT_CONNECT\n");
        }
        break;

        /*After the SDK was passively disconnected due to network conditions, it automatically initiated reconnection successfully.*/
        case AIOT_MQTTEVT_RECONNECT: {
            printf("AIOT_MQTTEVT_RECONNECT\n");
        }
        break;

        /*The SDK passively disconnected due to network conditions. Network failed to read and write at the bottom layer. Heartbeat did not get the server heartbeat response as expected.*/
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            printf("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
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
            printf("heartbeat response\n");
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            printf("suback, res: -0x%04X, packet id: %d, max qos: %d\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            printf("pub, qos: %d, topic: %.*s\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            printf("pub, payload: %.*s\n", packet->data.pub.payload_len, packet->data.pub.payload);
        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            printf("puback, packet id: %d\n", packet->data.pub_ack.packet_id);
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

int32_t demo_mqtt_start(void *handle, char *product_key, char *device_name, char *device_secret)
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = handle;
    uint16_t    port = 443;      /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
    aiot_sysdep_network_cred_t cred; /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/

    /*Create security credentials for the SDK, used to establish TLS connections*/
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /*Use RSA certificate to verify MQTT server*/
    cred.max_tls_fragment = 16384; /*The maximum fragment length is 16K, other optional values     are 4K, 2K, 1K, 0.5K*/
    cred.sni_enabled = 1;                               /*When establishing a TLS connection, support Server Name Indicator*/
    cred.x509_server_cert = ali_ca_cert;                 /*RSA root certificate used to verify the MQTT server*/
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /*RSA root certificate length used to verify the MQTT server*/

    /*TODO: If the following code is not commented, the routine will use TCP instead of TLS to connect to the cloud platform*/
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
        printf("aiot_mqtt_connect failed: -0x%04X\n\r\n", -res);
        printf("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        return -1;
    }

    /*Create a separate thread dedicated to executing aiot_mqtt_process. It will automatically send heartbeat keep-alives and resend QoS1 unanswered messages.*/
    g_mqtt_process_thread_running = 1;
    res = pthread_create(&g_mqtt_process_thread, NULL, demo_mqtt_process_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_process_thread failed: %d\n", res);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

    /*Create a separate thread for executing aiot_mqtt_recv, which will cycle through the MQTT messages sent by the server and automatically reconnect when disconnected*/
    g_mqtt_recv_thread_running = 1;
    res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_recv_thread failed: %d\n", res);
        g_mqtt_process_thread_running = 0;
        pthread_join(g_mqtt_process_thread, NULL);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

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
        printf("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        return -1;
    }

    /*Destroy the MQTT instance*/
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    return 0;
}

/*User data receiving and processing callback function*/
static void demo_dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    printf("demo_dm_recv_handler, type = %d\r\n", recv->type);

    switch (recv->type) {

        /*Attribute reporting, event reporting, obtaining the expected attribute value or deleting the response of the expected attribute value*/
        case AIOT_DMRECV_GENERIC_REPLY: {
            printf("msg_id = %d, code = %d, data = %.*s, message = %.*s\r\n",
                   recv->data.generic_reply.msg_id,
                   recv->data.generic_reply.code,
                   recv->data.generic_reply.data_len,
                   recv->data.generic_reply.data,
                   recv->data.generic_reply.message_len,
                   recv->data.generic_reply.message);
        }
        break;

        /*Property settings*/
        case AIOT_DMRECV_PROPERTY_SET: {
            printf("msg_id = %ld, params = %.*s\r\n",
                   (unsigned long)recv->data.property_set.msg_id,
                   recv->data.property_set.params_len,
                   recv->data.property_set.params);

            /*TODO: The following code demonstrates how to respond to attribute setting instructions from the cloud platform. Users can uncomment to view the demonstration effect.*/
            /*
            {
                aiot_dm_msg_t msg;

                memset(&msg, 0, sizeof(aiot_dm_msg_t));
                msg.type = AIOT_DMMSG_PROPERTY_SET_REPLY;
                msg.data.property_set_reply.msg_id = recv->data.property_set.msg_id;
                msg.data.property_set_reply.code = 200;
                msg.data.property_set_reply.data = "{}";
                int32_t res = aiot_dm_send(dm_handle, &msg);
                if (res < 0) {
                    printf("aiot_dm_send failed\r\n");
                }
            }
            */
        }
        break;

        /*Asynchronous service call*/
        case AIOT_DMRECV_ASYNC_SERVICE_INVOKE: {
            printf("msg_id = %ld, service_id = %s, params = %.*s\r\n",
                   (unsigned long)recv->data.async_service_invoke.msg_id,
                   recv->data.async_service_invoke.service_id,
                   recv->data.async_service_invoke.params_len,
                   recv->data.async_service_invoke.params);

            /*TODO: The following code demonstrates how to respond to asynchronous service calls from the cloud platform. Users can uncomment to view the demonstration effect.
             *
             * Note: If the user responds outside the callback function, he needs to save the msg_id himself, because the callback function input parameters will be destroyed by the SDK after exiting the callback function and can no longer be accessed.*/

            /*
            {
                aiot_dm_msg_t msg;

                memset(&msg, 0, sizeof(aiot_dm_msg_t));
                msg.type = AIOT_DMMSG_ASYNC_SERVICE_REPLY;
                msg.data.async_service_reply.msg_id = recv->data.async_service_invoke.msg_id;
                msg.data.async_service_reply.code = 200;
                msg.data.async_service_reply.service_id = "ToggleLightSwitch";
                msg.data.async_service_reply.data = "{\"dataA\": 20}";
                int32_t res = aiot_dm_send(dm_handle, &msg);
                if (res < 0) {
                    printf("aiot_dm_send failed\r\n");
                }
            }
            */
        }
        break;

        /*Synchronous service calls*/
        case AIOT_DMRECV_SYNC_SERVICE_INVOKE: {
            printf("msg_id = %ld, rrpc_id = %s, service_id = %s, params = %.*s\r\n",
                   (unsigned long)recv->data.sync_service_invoke.msg_id,
                   recv->data.sync_service_invoke.rrpc_id,
                   recv->data.sync_service_invoke.service_id,
                   recv->data.sync_service_invoke.params_len,
                   recv->data.sync_service_invoke.params);

            /*TODO: The following code demonstrates how to respond to synchronization service calls from the cloud platform. Users can uncomment to view the demonstration effect.
             *
             * Note: If the user responds outside the callback function, the msg_id and rrpc_id strings need to be saved by themselves, because the callback function input parameters will be destroyed by the SDK after exiting the callback function and can no longer be accessed.*/

            /*
            {
                aiot_dm_msg_t msg;

                memset(&msg, 0, sizeof(aiot_dm_msg_t));
                msg.type = AIOT_DMMSG_SYNC_SERVICE_REPLY;
                msg.data.sync_service_reply.rrpc_id = recv->data.sync_service_invoke.rrpc_id;
                msg.data.sync_service_reply.msg_id = recv->data.sync_service_invoke.msg_id;
                msg.data.sync_service_reply.code = 200;
                msg.data.sync_service_reply.service_id = "SetLightSwitchTimer";
                msg.data.sync_service_reply.data = "{}";
                int32_t res = aiot_dm_send(dm_handle, &msg);
                if (res < 0) {
                    printf("aiot_dm_send failed\r\n");
                }
            }
            */
        }
        break;

        /*Downstream binary data*/
        case AIOT_DMRECV_RAW_DATA: {
            printf("raw data len = %d\r\n", recv->data.raw_data.data_len);
            /*TODO: The following code demonstrates how to send data in binary format. If used, a corresponding data transparent transmission script needs to be deployed in the cloud.*/
            /*
            {
                aiot_dm_msg_t msg;
                uint8_t raw_data[] = {0x01, 0x02};

                memset(&msg, 0, sizeof(aiot_dm_msg_t));
                msg.type = AIOT_DMMSG_RAW_DATA;
                msg.data.raw_data.data = raw_data;
                msg.data.raw_data.data_len = sizeof(raw_data);
                aiot_dm_send(dm_handle, &msg);
            }
            */
        }
        break;

        /*The synchronous service call in binary format has one more rrpc_id than the simple binary data message.*/
        case AIOT_DMRECV_RAW_SYNC_SERVICE_INVOKE: {
            printf("raw sync service rrpc_id = %s, data_len = %d\r\n",
                   recv->data.raw_service_invoke.rrpc_id,
                   recv->data.raw_service_invoke.data_len);
        }
        break;

        default:
            break;
    }
}

int32_t demo_dm_start(void **handle, void *mqtt_handle, char *product_key, char *device_name)
{
    void *dm_handle = NULL;

    dm_handle = aiot_dm_init();
    if (dm_handle == NULL) {
        return -1;
    }

    /*Configure MQTT instance handle*/
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_MQTT_HANDLE, mqtt_handle);
    /*Configure message reception processing callback function*/
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_RECV_HANDLER, (void *)demo_dm_recv_handler);

    *handle = dm_handle;

    return STATE_SUCCESS;
}

int32_t demo_dm_stop(void **handle)
{
    int32_t res = STATE_SUCCESS;

    /*Destroy the DATA-MODEL instance, generally it will not run here*/
    res = aiot_dm_deinit(handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_dm_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    return STATE_SUCCESS;
}

/*Event callback function, which is triggered when the network is connected/reconnected/disconnected. For the event definition, see core/aiot_diag_api.h*/
void demo_diag_event_handler(void *handle, const aiot_diag_event_t *event, void *userdata)
{
    switch (event->type) {
        case AIOT_DIAGEVT_ALERT: {
            printf("AIOT_DIAGEVT_ALERT, module name: %s, level: %s, desc: %s\n",
                   event->data.alert.module_name, event->data.alert.level, event->data.alert.desc);
        }
        break;
        default: {

        }
    }
}

/*Data processing callback, called when the SDK receives a diag message from the network*/
void demo_diag_recv_handler(void *handle, const aiot_diag_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_DIAGRECV_DIAG_CONTROL: {
        }
        break;
        default: {

        }
    }
}

int main(int argc, char *argv[])
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL, *diag_handle = NULL, *dm_handle = NULL;

    /*Configure the underlying dependencies of the SDK*/
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /*Configure the log output of the SDK*/
    aiot_state_set_logcb(demo_state_logcb);

    /*Create 1 MQTT client instance and initialize default parameters internally*/
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        printf("aiot_mqtt_init failed\n");
        return -1;
    }

    diag_handle = aiot_diag_init();
    if (diag_handle == NULL) {
        printf("aiot_diag_init failed\n");
        return -1;
    }

    res = aiot_diag_setopt(diag_handle, AIOT_DIAGOPT_MQTT_HANDLE, mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_diag_deinit(&diag_handle);
        printf("aiot_diag_setopt failed\n");
        return -1;
    }

    res = aiot_diag_setopt(diag_handle, AIOT_DIAGOPT_EVENT_HANDLER, demo_diag_event_handler);
    if (res < STATE_SUCCESS) {
        aiot_diag_deinit(&diag_handle);
        printf("aiot_diag_setopt failed\n");
        return -1;
    }

    res = aiot_diag_start(diag_handle);
    if (res < STATE_SUCCESS) {
        aiot_diag_deinit(&diag_handle);
        printf("aiot_diag_start failed\n");
        return -1;
    }

    /*Establish an MQTT connection and start the keepalive thread and receiving thread*/
    res = demo_mqtt_start(mqtt_handle, product_key, device_name, device_secret);
    if (res < STATE_SUCCESS) {
        aiot_diag_deinit(&diag_handle);
        printf("demo_mqtt_start failed\n");
        return -1;
    }

    res = demo_dm_start(&dm_handle, mqtt_handle, product_key, device_name);
    if (res < STATE_SUCCESS) {
        aiot_diag_deinit(&diag_handle);
        demo_mqtt_stop(&mqtt_handle);
        printf("demo_dm_start failed\n");
        return -1;
    }

    /* {
        aiot_dm_msg_t msg;

        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_PROPERTY_POST;
        msg.data.property_post.params = "{\"LightSwitch\":1}";

        res = aiot_dm_send(dm_handle, &msg);
    } */

    while (1) {
        sleep(1);
    }

    /*Destroy the dm instance, generally it will not run here*/
    demo_dm_stop(&dm_handle);

    /*Destroy the diag instance, generally it will not run here.*/
    aiot_diag_deinit(&diag_handle);

    /*Destroy the MQTT instance and exit the thread. Generally, it will not run here.*/
    demo_mqtt_stop(&mqtt_handle);

    return 0;
}

