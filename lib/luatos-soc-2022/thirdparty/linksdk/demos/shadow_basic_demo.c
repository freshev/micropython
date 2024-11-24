/** This routine is suitable for POSIX devices such as `Linux` that support pthread. It demonstrates using the SDK to configure MQTT parameters and establish a connection, and then create 2 threads
 *
 * + One thread for keep-alive connections
 * + A thread is used to receive messages, and enters the default data callback when a message arrives, and enters the event callback when the connection status changes
 *
 * Then it demonstrates the device shadow update, delete, pull and other operations on the MQTT connection. Uncomment these code paragraphs to observe the running effect.
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
#include "aiot_shadow_api.h"


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
 * For example: [1577589489.033][LK-0317] shadow_basic_demo&a13FN5TplKq
 *
 * The code of the above log is 0317 (hexadecimal). For the definition of code value, see core/aiot_state_api.h
 **/

/*Log callback function, SDK logs will be output from here*/
int32_t demo_state_logcb(int32_t code, char *message)
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

/*Data processing callback, called when the SDK receives a shadow message from the network*/
void demo_shadow_recv_handler(void *handle, const aiot_shadow_recv_t *recv, void *userdata)
{
    printf("demo_shadow_recv_handler, type = %d, productKey = %s, deviceName = %s\r\n",
           recv->type, recv->product_key, recv->device_name);

    switch (recv->type) {
        /*When the device sends the AIOT_SHADOWMSG_UPDATE, AIOT_SHADOWMSG_CLEAN_DESIRED or AIOT_SHADOWMSG_DELETE_REPORTED message, it will receive this response message*/
        case AIOT_SHADOWRECV_GENERIC_REPLY: {
            const aiot_shadow_recv_generic_reply_t *generic_reply = &recv->data.generic_reply;

            printf("payload = \"%.*s\", status = %s, timestamp = %ld\r\n",
                   generic_reply->payload_len,
                   generic_reply->payload,
                   generic_reply->status,
                   (unsigned long)generic_reply->timestamp);
        }
        break;
        /*When the device is online, if the user application calls the cloud API to actively update the device shadow, the device will receive this message.*/
        case AIOT_SHADOWRECV_CONTROL: {
            const aiot_shadow_recv_control_t *control = &recv->data.control;

            printf("payload = \"%.*s\", version = %ld\r\n",
                   control->payload_len,
                   control->payload,
                   (unsigned long)control->version);
        }
        break;
        /*This message will be received when the device sends the AIOT_SHADOWMSG_GET message to actively obtain the device shadow.*/
        case AIOT_SHADOWRECV_GET_REPLY: {
            const aiot_shadow_recv_get_reply_t *get_reply = &recv->data.get_reply;

            printf("payload = \"%.*s\", version = %ld\r\n",
                   get_reply->payload_len,
                   get_reply->payload,
                   (unsigned long)get_reply->version);
        }
        default:
            break;
    }
}

/*Send a request to update the device shadow reported value*/
int32_t demo_update_shadow(void *shadow_handle, char *reported_data, int64_t version)
{
    aiot_shadow_msg_t message;

    memset(&message, 0, sizeof(aiot_shadow_msg_t));
    message.type = AIOT_SHADOWMSG_UPDATE;
    message.data.update.reported = reported_data;
    message.data.update.version = version;

    return aiot_shadow_send(shadow_handle, &message);
}

/*Send a request to delete a specific reported value in the device shadow*/
int32_t demo_delete_shadow_report(void *shadow_handle, char *reported, int64_t version)
{
    aiot_shadow_msg_t message;

    memset(&message, 0, sizeof(aiot_shadow_msg_t));
    message.type = AIOT_SHADOWMSG_DELETE_REPORTED;
    message.data.delete_reporte.reported = reported;
    message.data.delete_reporte.version = version;

    return aiot_shadow_send(shadow_handle, &message);
}

/*Send a request to clear all desired values   in the device shadow*/
int32_t demo_clean_shadow_desired(void *shadow_handle, int64_t version)
{
    aiot_shadow_msg_t message;

    memset(&message, 0, sizeof(aiot_shadow_msg_t));
    message.type = AIOT_SHADOWMSG_CLEAN_DESIRED;
    message.data.clean_desired.version = version;

    return aiot_shadow_send(shadow_handle, &message);
}

/*Send a request to get the device shadow*/
int32_t demo_get_shadow(void *shadow_handle)
{
    aiot_shadow_msg_t message;

    memset(&message, 0, sizeof(aiot_shadow_msg_t));
    message.type = AIOT_SHADOWMSG_GET;

    return aiot_shadow_send(shadow_handle, &message);
}


int main(int argc, char *argv[])
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    void       *shadow_handle = NULL;
    uint16_t    port = 1883;      /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
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
        printf("aiot_mqtt_init failed\n");
        return -1;
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
    /*Configure MQTT event callback function*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /*Create a shadow client instance and initialize default parameters internally*/
    shadow_handle = aiot_shadow_init();
    if (shadow_handle == NULL) {
        printf("aiot_shadow_init failed\n");
        return -1;
    }

    /*Configure MQTT instance handle*/
    aiot_shadow_setopt(shadow_handle, AIOT_SHADOWOPT_MQTT_HANDLE, mqtt_handle);
    /*Configure SHADOW default message receiving callback function*/
    aiot_shadow_setopt(shadow_handle, AIOT_SHADOWOPT_RECV_HANDLER, (void *)demo_shadow_recv_handler);

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
        return -1;
    }

    /*Create a separate thread for executing aiot_mqtt_recv, which will cycle through the MQTT messages sent by the server and automatically reconnect when disconnected*/
    g_mqtt_recv_thread_running = 1;
    res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, mqtt_handle);
    if (res < 0) {
        printf("pthread_create demo_mqtt_recv_thread failed: %d\n", res);
        return -1;
    }

    /*The main loop goes to sleep*/
    while (1) {
        /*TODO: The following code demonstrates sending a request to pull the device shadow. The device shadow will be returned in the demo_shadow_recv_handler callback function. The user can uncomment to observe the demonstration effect.*/
        /*
        res = demo_get_shadow(shadow_handle);
        if (res < 0) {
            printf("demo_get_shadow failed, res = -0x%04x\r\n", -res);
        }
        */
        sleep(2);

        /*TODO: The following code demonstrates how to clear the desired value in the device shadow and set the version to 1. The user can uncomment to observe the demonstration effect.*/
        /*
        res = demo_clean_shadow_desired(shadow_handle, 1);
        if (res < 0) {
            printf("demo_clean_shadow_desired failed, res = -0x%04x\r\n", -res);
        }
        */
        sleep(2);

        /*TODO: The following code demonstrates how to delete a specific reported value in the device shadow and set the version to 2. The user can uncomment to observe the demonstration effect.*/
        /*
        res = demo_delete_shadow_report(shadow_handle, "{\"LightSwitch\":\"null\"}", 2);
        if (res < 0) {
            printf("demo_delete_shadow_report failed, res = -0x%04x\r\n", -res);
        }
        */
        sleep(2);

        /*TODO: The following code demonstrates how to update the reported value in the device shadow and reset the version to 0. The user can uncomment to observe the demonstration effect.*/
        /*
        res = demo_update_shadow(shadow_handle, "{\"LightSwitch\":1}", 0);
        if (res < 0) {
            printf("demo_delete_shadow_report failed, res = -0x%04x\r\n", -res);
        }
        */

        sleep(10);
    }

    /*Disconnect the MQTT connection, generally it will not run here*/
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        return -1;
    }

    /*Destroy the SHADOW instance, generally it will not run here.*/
    res = aiot_shadow_deinit(&shadow_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_shadow_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    /*Destroy the MQTT instance, generally it will not run here*/
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    pthread_join(g_mqtt_process_thread, NULL);
    pthread_join(g_mqtt_recv_thread, NULL);

    return 0;
}

