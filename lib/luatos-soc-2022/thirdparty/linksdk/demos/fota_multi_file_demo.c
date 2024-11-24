/** This routine is suitable for POSIX devices such as `Linux` that support pthread. It demonstrates using the SDK to configure MQTT parameters and establish a connection, and then create 3 threads
 *
 * + One thread for keep-alive connections
 * + A thread is used to receive messages, and enters the default data callback when a message arrives, and enters the event callback when the connection status changes
 * + A thread is used to download the firmware to be upgraded via HTTP from the network. This thread is started by the receiving message thread after getting the MQTT message of the OTA upgrade.
 *
 * This example describes how to upgrade the device when there are multiple URLs in a single upgrade package.
 * Parts that require user attention or modification have been marked in comments with `TODO`
 **/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_ota_api.h"
#include "aiot_mqtt_api.h"

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

/*The action of sending and receiving packets is about to end*/
int should_stop = 0;

/*A collection of system adaptation functions located in the portfiles/aiot_port folder*/
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/*Server certificate located in external/ali_ca_cert.c*/
extern const char *ali_ca_cert;

static pthread_t g_mqtt_process_thread; /*Long connection keep-alive thread for MQTT*/
static pthread_t g_mqtt_recv_thread;    /*Cyclic message receiving thread for MQTT*/
static pthread_t g_download_thread;     /*Firmware download thread for HTTP*/

/*TODO: If you want to close the log, make this function empty. If you want to reduce the log, you can choose not to print according to the code.
 *
 * For example: [1578463098.611][LK-0309] pub: /ota/device/upgrade/a13FN5TplKq/ota_demo
 *
 * The code of the above log is 0309 (hexadecimal). For the definition of code value, see core/aiot_state_api.h
 **/

/*Description of the overall task progress in the case of a single upgrade package with multiple files*/
typedef struct {
    int32_t last_percent;
    uint32_t file_id;
    uint32_t file_num;
} multi_download_status_t;

int32_t g_finished_task_num = 0;

/*Log callback function, SDK logs will be output from here*/
int32_t demo_state_logcb(int32_t code, char *message)
{
    /*When downloading firmware, there will be a large number of HTTP packet collection logs, which can be filtered out and closed by code.*/
    if (STATE_HTTP_LOG_RECV_CONTENT != code) {
        printf("%s", message);
    }
    return 0;
}

/*Download and receive package callback. After the user calls aiot_download_recv(), the SDK will enter this function after receiving the data and hand the downloaded data to the user.*/
/*TODO: Generally speaking, when the device is upgraded, the downloaded data will be written to Flash in this callback.*/
void demo_download_recv_handler(void *handle, const aiot_download_recv_t *packet, void *userdata)
{
    uint32_t data_buffer_len = 0;
    int32_t last_percent = 0;
    int32_t  percent = 0;
    multi_download_status_t *download_status = (multi_download_status_t *)userdata;

    /*Currently only supports the case where packet->type is AIOT_DLRECV_HTTPBODY*/
    if (!packet || AIOT_DLRECV_HTTPBODY != packet->type) {
        return;
    }
    percent = packet->data.percent;

    /*userdata can store data that needs to be shared between different entries of demo_download_recv_handler()*/
    /*This is used to store the download progress percentage of the firmware when this callback function was last entered.*/
    if (userdata) {
        last_percent = (download_status->last_percent);
    }

    data_buffer_len = packet->data.len;

    /*If percent is a negative number, it means that a packet receiving exception or digest verification error occurred.*/
    if (percent < 0) {
        printf("exception: percent = %d\r\n", percent);
        if (userdata) {
            free(userdata);
        }
        return;
    }

    /** TODO: Download a piece of firmware successfully. At this time, the user should
     * The starting address is packet->data.buffer, the length is packet->data.len, and the memory is saved to flash.
     *
     * If flash programming fails, aiot_download_report_progress(handle, -4) should also be called to report the failure to the cloud platform
     * Note: In the protocol, the error code agreed with the cloud platform is in the aiot_ota_protocol_errcode_t type, for example
     * -1: Indicates upgrade failure
     * -2: Indicates download failure
     * -3: Indicates verification failure
     * -4: Indicates programming failure
     *
     * For details, please visit https://help.aliyun.com/document_detail/85700.html*/

    /*When the value of the percent input parameter is 100, it means that the SDK has completed downloading the firmware content.*/
    if (percent == 100) {
        g_finished_task_num++;
        /** TODO: At this time, the average user should complete all firmware burning, save the current work, restart the device, and switch to the new firmware to start.
                 Also, the new firmware must be

                 aiot_ota_report_version(ota_handle, new_version);

                 Such an operation will report the upgraded new version number (for example, 1.0.0 to 1.1.0, then the value of new_version is "1.1.0") to the cloud platform
                 The cloud platform will determine that the upgrade was successful only after receiving the new version number report, otherwise it will consider that the upgrade failed.
                 If the upgrade fails after the download is successful, you should also call aiot_download_report_progress(handle, -1) to report the failure to the cloud platform*/
    }

    /*Simplify the output. Only when the progress of the last download has increased by more than 5%, the progress will be printed and reported to the server.*/
    if (percent - last_percent >= 5 || percent == 100) {
        if (NULL != download_status) {
            printf("file_id %d, download %03d%% done, +%d bytes\r\n", download_status->file_id, percent, data_buffer_len);
            download_status->last_percent = percent;
            if (g_finished_task_num == download_status->file_num) {
                /*Considering the concurrent downloading of multiple threads, 100% progress will only be reported after all files are downloaded.*/
                aiot_download_report_progress(handle, 100);
            }
        }

        if (percent == 100 && userdata) {
            free(userdata);
        }
    }
}

/*Thread that executes aiot_download_recv to implement request and reception of firmware content*/
void *demo_ota_download_thread(void *dl_handle)
{
    int32_t     ret = 0;

    printf("\r\nstarting download thread in 2 seconds ......\r\n");
    sleep(2);

    /*Request download from firmware server*/
    /** TODO: The following way of writing is to obtain all the firmware content with one request
     * When the device resources are relatively small or the network is poor, you can also download in parts and need to be combined.
     *
     * aiot_download_setopt(dl_handle, AIOT_DLOPT_RANGE_START, ...);
     * aiot_download_setopt(dl_handle, AIOT_DLOPT_RANGE_END, ...);
     * aiot_download_send_request(dl_handle);
     *
     * Implementation, in this case, the above combined statement needs to be placed in a loop, send_request and recv multiple times
     **/
    aiot_download_send_request(dl_handle);
    while (should_stop == 0) {
        /*Receive the firmware content of the server response from the network*/
        ret = aiot_download_recv(dl_handle);

        /*When all firmware is downloaded, the return value of aiot_download_recv() will be equal to STATE_DOWNLOAD_FINISHED, otherwise it will be the number of bytes obtained that time*/
        if (STATE_DOWNLOAD_FINISHED == ret) {
            printf("download completed\r\n");
            break;
        }
        if (STATE_DOWNLOAD_RENEWAL_REQUEST_SENT == ret) {
            printf("download renewal request has been sent successfully\r\n");
            continue;
        }
        if (ret <= STATE_SUCCESS) {
            printf("download failed, error code is %d, try to send renewal request\r\n", ret);
            continue;
        }
    }

    /*Downloading all firmware content is completed, the download session is destroyed, and the thread exits by itself*/
    aiot_download_deinit(&dl_handle);
    printf("download thread exit\r\n");

    return NULL;
}


/*The OTA message processing callback registered by the user through aiot_ota_setopt(). If the SDK receives an OTA-related MQTT message, it will automatically identify and call this callback function.*/
void demo_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
    case AIOT_OTARECV_FOTA: {
        uint32_t res = 0;
        uint16_t port = 443;
        uint32_t max_buffer_len = (8 * 1024);
        aiot_sysdep_network_cred_t cred;
        void *dl_handle = NULL;
        multi_download_status_t *download_status = NULL;

        if (NULL == ota_msg->task_desc || ota_msg->task_desc->protocol_type != AIOT_OTA_PROTOCOL_HTTPS) {
            break;
        }

        dl_handle = aiot_download_init();
        if (NULL == dl_handle) {
            break;
        }

        if (NULL != ota_msg->task_desc->file_name) {
            printf("\r\nTotal file number is %d, current file id is %d, with file_name %s\r\n", ota_msg->task_desc->file_num,
                   ota_msg->task_desc->file_id, ota_msg->task_desc->file_name);
        }

        printf("OTA target firmware version: %s,  size: %u Bytes \r\n", ota_msg->task_desc->version,
               ota_msg->task_desc->size_total);


        if (NULL != ota_msg->task_desc->extra_data) {
            printf("extra data: %s\r\n", ota_msg->task_desc->extra_data);
        }

        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;
        cred.max_tls_fragment = 16384;
        cred.x509_server_cert = ali_ca_cert;
        cred.x509_server_cert_len = strlen(ali_ca_cert);

        /*Set download to TLS when downloading*/
        aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred));
        /*Set the server port number accessed when downloading*/
        aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
        /*Set the download task information, which is obtained by inputting the task_desc member in the parameter ota_msg, including the download address, firmware size, firmware signature, etc.*/
        aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
        /*Set the callback function that the SDK will call when the downloaded content arrives.*/
        aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(demo_download_recv_handler));
        /*Set the maximum buffer length for a single download. The user will be notified whenever the memory of this length is full.*/
        aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
        /*Set the data shared between different calls of AIOT_DLOPT_RECV_HANDLER, for example, the routine stores the progress here*/
        download_status = malloc(sizeof(multi_download_status_t));
        if (NULL == download_status) {
            aiot_download_deinit(&dl_handle);
            break;
        }
        memset(download_status, 0, sizeof(multi_download_status_t));
        download_status->file_id = ota_msg->task_desc->file_id;
        download_status->file_num = ota_msg->task_desc->file_num;
        aiot_download_setopt(dl_handle, AIOT_DLOPT_USERDATA, (void *)download_status);

        /*If it is the first download task, report progress 0*/
        if (0 == ota_msg->task_desc->file_id) {
            aiot_download_report_progress(dl_handle, 0);
        }

        /*Start a dedicated download thread to complete the download of firmware content*/
        res = pthread_create(&g_download_thread, NULL, demo_ota_download_thread, dl_handle);
        if (res != 0) {
            printf("pthread_create demo_ota_download_thread failed: %d\r\n", res);
            aiot_download_deinit(&dl_handle);
            free(download_status);
        } else {
            /*The download thread is set to detach type and can exit independently after the firmware content is obtained.*/
            pthread_detach(g_download_thread);
        }
        break;
    }

    default:
        break;
    }
}

/*MQTT event callback function, which is triggered when the network is connected/reconnected/disconnected. For the event definition, see core/aiot_mqtt_api.h*/
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
    /*SDK has successfully established a connection with the mqtt server because the user called the aiot_mqtt_connect() interface.*/
    case AIOT_MQTTEVT_CONNECT: {
        printf("AIOT_MQTTEVT_CONNECT\r\n");
        /*TODO: When processing the SDK connection establishment successfully, long time-consuming blocking functions cannot be called here.*/
    }
    break;

    /*After the SDK was passively disconnected due to network conditions, it automatically initiated reconnection successfully.*/
    case AIOT_MQTTEVT_RECONNECT: {
        printf("AIOT_MQTTEVT_RECONNECT\r\n");
        /*TODO: Handle SDK reconnection successfully, long time-consuming blocking functions cannot be called here*/
    }
    break;

    /*The SDK passively disconnected due to network conditions. Network failed to read and write at the bottom layer. Heartbeat did not get the server heartbeat response as expected.*/
    case AIOT_MQTTEVT_DISCONNECT: {
        char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                      ("heartbeat disconnect");
        printf("AIOT_MQTTEVT_DISCONNECT: %s\r\n", cause);
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
        printf("heartbeat response\r\n");
        /*TODO: Process the server's response to the heartbeat, generally not processed*/
    }
    break;

    case AIOT_MQTTRECV_SUB_ACK: {
        printf("suback, res: -0x%04X, packet id: %d, max qos: %d\r\n",
               -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        /*TODO: Process the server's response to the subscription request, generally not processed*/
    }
    break;

    case AIOT_MQTTRECV_PUB: {
        printf("pub, qos: %d, topic: %.*s\r\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
        printf("pub, payload: %.*s\r\n", packet->data.pub.payload_len, packet->data.pub.payload);
        /*TODO: Process the business messages sent by the server*/
    }
    break;

    case AIOT_MQTTRECV_PUB_ACK: {
        printf("puback, packet id: %d\r\n", packet->data.pub_ack.packet_id);
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
    while (should_stop == 0) {
        aiot_mqtt_process(args);
        sleep(1);
    }
    return NULL;
}

/*The thread that executes aiot_mqtt_recv, including automatic network reconnection and receiving MQTT messages from the server*/
void *demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (should_stop == 0) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            sleep(1);
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    uint16_t    port = 443;      /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
    aiot_sysdep_network_cred_t cred; /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/
    void       *ota_handle = NULL;
    char       *cur_version = NULL;

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
    if (NULL == mqtt_handle) {
        printf("aiot_mqtt_init failed\r\n");
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
    /*Configure MQTT default message receiving callback function*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /*Configure MQTT event callback function*/
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /*Different from the MQTT routine, here you need to add the statement to create an OTA session instance*/
    ota_handle = aiot_ota_init();
    if (NULL == ota_handle) {
        printf("aiot_ota_init failed\r\n");
        aiot_mqtt_deinit(&mqtt_handle);
        return -2;
    }

    /*Use the following statement to associate the OTA session with the MQTT session*/
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_MQTT_HANDLE, mqtt_handle);
    /*Use the following statement to set the data receiving callback of the OTA session. When the SDK receives OTA-related push, it will enter this callback function.*/
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_RECV_HANDLER, demo_ota_recv_handler);

    /*Establish an MQTT connection with the server*/
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /*The attempt to establish a connection failed, the MQTT instance was destroyed, and the resources were recycled.*/
        aiot_mqtt_deinit(&mqtt_handle);
        aiot_ota_deinit(&ota_handle);
        printf("aiot_mqtt_connect failed: -0x%04X\r\n\r\n", -res);
        printf("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        return -3;
    }

    /*TODO: Very important!!!
     *
     * cur_version should be changed to be obtained from the configuration area of   the device according to the actual situation of the user. It should reflect the real version number and cannot be written as a fixed value like the example.
     *
     * 1. If the device has never reported a version number, the upgrade task cannot be deployed on the console web page.
     * 2. If after the device upgrade is completed, the version number reported is not the new version number, the upgrade failed will be displayed on the console web page.
     **/

    /*After the demonstration MQTT connection is established, the version number of the current device can be reported.*/
    cur_version = "1.0.0";
    res = aiot_ota_report_version(ota_handle, cur_version);
    if (res < STATE_SUCCESS) {
        printf("aiot_ota_report_version failed: -0x%04X\r\n", -res);
    }

    /*Create a separate thread dedicated to executing aiot_mqtt_process. It will automatically send heartbeat keep-alives and resend QoS1 unanswered messages.*/
    res = pthread_create(&g_mqtt_process_thread, NULL, demo_mqtt_process_thread, mqtt_handle);
    if (res != 0) {
        printf("pthread_create demo_mqtt_process_thread failed: %d\r\n", res);
        return -1;
    }

    /*Create a separate thread for executing aiot_mqtt_recv, which will cycle through the MQTT messages sent by the server and automatically reconnect when disconnected*/
    res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, mqtt_handle);
    if (res != 0) {
        printf("pthread_create demo_mqtt_recv_thread failed: %d\r\n", res);
        return -1;
    }

    /*The main loop goes to sleep*/
    while (1) {
        sleep(1);
    }

    should_stop = 1;
    sleep(5);

    /*Disconnect the MQTT connection, generally it will not run here*/
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        aiot_ota_deinit(&ota_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04X\r\n", -res);
        return -1;
    }

    /*Destroy the OTA instance, generally it will not run here.*/
    aiot_ota_deinit(&ota_handle);
    return 0;
}

