/** This routine is suitable for devices that do not support opening an independent firmware download thread outside the main thread through the pthread function.
 * It demonstrates using the SDK to configure MQTT parameters and establish a connection, and start the process of downloading and upgrading the firmware after receiving the OTA mqtt message
 * Also, it demonstrates how to divide the size of the firmware in half and download one half at a time. The user can further divide the firmware into more smaller segments
 *
 * Parts that require user attention or modification have been marked in comments with `TODO`
 **/

/*TODO: This example uses the sleep function, so unistd.h is included. If the user has an alternative function in his own library, unistd.h can be replaced.
 *
 * This example uses the malloc/free function, so stdlib.h is used. If the user has an alternative function in his own library, he needs to replace stdlib.h.
 **/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_ota_api.h"


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

/*Server certificate located in external/ali_ca_cert.c*/
extern const char *ali_ca_cert;
void *g_ota_handle = NULL;
void *g_dl_handle = NULL;
uint32_t g_firmware_size = 0;

/*A collection of system adaptation functions located in the portfiles/aiot_port folder*/
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/*TODO: If you want to close the log, make this function empty. If you want to reduce the log, you can choose not to print according to the code.
 *
 * For example: [1578463098.611][LK-0309] pub: /ota/device/upgrade/a13FN5TplKq/ota_demo
 *
 * The code of the above log is 0309 (hexadecimal). For the definition of code value, see core/aiot_state_api.h
 **/

/*Log callback function, SDK logs will be output from here*/
int32_t demo_state_logcb(int32_t code, char *message)
{
    /*When downloading firmware, there will be a large number of HTTP packet collection logs, which can be filtered out and closed by code.*/
    if (STATE_HTTP_LOG_RECV_CONTENT != code) {
        printf("%s", message);
    }
    return 0;
}

/*MQTT event callback function, which is triggered when the network is connected/reconnected/disconnected. For the event definition, see core/aiot_mqtt_api.h*/
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *const event, void *userdata)
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
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *const packet, void *userdata)
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

/*Download and receive package callback. After the user calls aiot_download_recv(), the SDK will enter this function after receiving the data and hand the downloaded data to the user.*/
/*TODO: Generally speaking, when the device is upgraded, the downloaded data will be written to Flash in this callback.*/
void user_download_recv_handler(void *handle, const aiot_download_recv_t *packet, void *userdata)
{
    int32_t percent = 0;
    int32_t last_percent = 0;
    uint32_t data_buffer_len = 0;

    /*Currently only supports the case where packet->type is AIOT_DLRECV_HTTPBODY*/
    if (!packet || AIOT_DLRECV_HTTPBODY != packet->type) {
        return;
    }
    percent = packet->data.percent;

    /*If percent is a negative number, it means that a packet receiving exception or digest verification error occurred.*/
    if (percent < 0) {
        /*digest check error*/
        printf("exception happend, percent is %d\r\n", percent);
        if (userdata) {
            free(userdata);
        }
        return;
    }

    /*userdata can store data that needs to be shared between different entries of demo_download_recv_handler()*/
    /*This is used to store the download progress percentage of the firmware when this callback function was last entered.*/
    if (userdata) {
        last_percent = *((uint32_t *)(userdata));
    }
    data_buffer_len = packet->data.len;

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
        /*Report version number*/
        /** TODO: At this time, the average user should complete all firmware burning, save the current work, restart the device, and switch to the new firmware to start.
         * Also, the new firmware must be
         *
         * aiot_ota_report_version(g_ota_handle, new_version);
         *
         * This operation will report the upgraded new version number (for example, if 1.0.0 is upgraded to 1.1.0, then the value of new_version is "1.1.0") to the cloud platform
         * The cloud platform will determine that the upgrade is successful only after receiving the new version number report, otherwise it will consider that the upgrade has failed.
         **/
    }

    /*Simplify the output. Only when the progress of the last download has increased by more than 5%, the progress will be printed and reported to the server.*/
    if (percent - last_percent >= 5 || percent == 100) {
        printf("download %03d%% done, +%d bytes\r\n", percent, data_buffer_len);
        aiot_download_report_progress(handle, percent);

        if (userdata) {
            *((uint32_t *)(userdata)) = percent;
        }
        if (percent == 100 && userdata) {
            free(userdata);
        }
    }
}

/*The OTA message processing callback registered by the user through aiot_ota_setopt(). If the SDK receives an OTA-related MQTT message, it will automatically identify and call this callback function.*/
void user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
    case AIOT_OTARECV_FOTA: {
        uint16_t port = 443;
        uint32_t max_buffer_len = 2048;
        aiot_sysdep_network_cred_t cred;
        void *dl_handle = NULL;
        void *last_percent = NULL;

        if (NULL == ota_msg->task_desc || ota_msg->task_desc->protocol_type != AIOT_OTA_PROTOCOL_HTTPS) {
            break;
        }

        dl_handle = aiot_download_init();
        if (NULL == dl_handle) {
            break;
        }

        last_percent = malloc(sizeof(uint32_t));
        if (NULL == last_percent) {
            aiot_download_deinit(&dl_handle);
            break;
        }
        memset(last_percent, 0, sizeof(uint32_t));

        printf("OTA target firmware version: %s, size: %u Bytes\r\n", ota_msg->task_desc->version,
               ota_msg->task_desc->size_total);

        if (NULL != ota_msg->task_desc->extra_data) {
            printf("extra data: %s\r\n", ota_msg->task_desc->extra_data);
        }

        g_firmware_size = ota_msg->task_desc->size_total;

        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;
        cred.max_tls_fragment = 16384;
        cred.x509_server_cert = ali_ca_cert;
        cred.x509_server_cert_len = strlen(ali_ca_cert);
        uint32_t end = g_firmware_size / 2;
        /*Set download to TLS when downloading*/
        if ((STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred)))
                /*Set the server port number accessed when downloading*/
                || (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port)))
                /*Set the download task information, which is obtained by inputting the task_desc member in the parameter ota_msg, including the download address, firmware size, firmware signature, etc.*/
                || (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc)))
                /*Set the callback function that the SDK will call when the downloaded content arrives.*/
                || (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(user_download_recv_handler)))
                /*Set the maximum buffer length for a single download. The user will be notified whenever the memory of this length is full.*/
                || (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len)))
                /*Set the data shared between different calls of AIOT_DLOPT_RECV_HANDLER, for example, the routine stores the progress here*/
                || (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_USERDATA, (void *)last_percent))
                /*Indicates that the download method is to download according to the range, and only half of it is currently downloaded.*/
                || (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_RANGE_END, (void *)&end))
                /*Send an http GET request to the http server*/
                || (STATE_SUCCESS != aiot_download_send_request(dl_handle))) {
            aiot_download_deinit(&dl_handle);
            free(last_percent);
            break;
        }
        g_dl_handle = dl_handle;
        break;
    }
    default:
        break;
    }
}


int main(int argc, char *argv[])
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;
    uint16_t port = 443; /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
    aiot_sysdep_network_cred_t cred; /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/
    char *cur_version = NULL;
    void *ota_handle = NULL;
    uint32_t timeout_ms = 0;

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
        goto exit;
    }

    /*Use the following statement to associate the OTA session with the MQTT session*/
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_MQTT_HANDLE, mqtt_handle);
    /*Use the following statement to set the data receiving callback of the OTA session. When the SDK receives OTA-related push, it will enter this callback function.*/
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_RECV_HANDLER, user_ota_recv_handler);
    g_ota_handle = ota_handle;

    /*Establish an MQTT connection with the server*/
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_connect failed: -0x%04X\r\n\r\n", -res);
        printf("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        /*The attempt to establish a connection failed, the MQTT instance was destroyed, and the resources were recycled.*/
        goto exit;
    }

    /*TODO: Very important!!!
     *
     * cur_version should be changed to be obtained from the configuration area of   the device according to the actual situation of the user. It should reflect the real version number and cannot be written as a fixed value like the example.
     *
     * 1. If the device has never reported a version number, the upgrade task cannot be deployed on the console web page.
     * 2. If after the device upgrade is completed, the version number reported is not the new version number, the upgrade failed will be displayed on the console web page.
     **/

    cur_version = "1.0.0";
    /*After the demonstration MQTT connection is established, the version number of the current device can be reported.*/
    res = aiot_ota_report_version(ota_handle, cur_version);
    if (res < STATE_SUCCESS) {
        printf("report version failed, code is -0x%04X\r\n", -res);
    }

    while (1) {
        aiot_mqtt_process(mqtt_handle);
        res = aiot_mqtt_recv(mqtt_handle);

        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        if (NULL != g_dl_handle) {
            /*Before completing the reception of the firmware, adjust the mqtt packet reception timeout to 100ms to reduce the time interval between two firmware download actions.*/
            int32_t ret = aiot_download_recv(g_dl_handle);
            timeout_ms = 100;
            aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);

            /*In this example, the entire firmware is divided into 2 equal parts for downloading, half of which is downloaded each time. When the first half is downloaded, it will enter this branch.*/
            if (STATE_DOWNLOAD_RANGE_FINISHED == ret) {
                printf("the first half download finished\n");

                /*When the first download is completed, reset the start/end values   of the range and download the other half.*/

                /** Very important:
                 *
                 * The range of the last download is [0, g_firmware_size/2],
                 * Therefore, this download needs to be from [g_firmware_size/2 + 1, firmware end address],
                 * If the firmware end address is filled with 0, it means until the download is completed.
                 **/

                uint32_t start = g_firmware_size / 2 + 1;
                uint32_t end = 0;
                aiot_download_setopt(g_dl_handle, AIOT_DLOPT_RANGE_START, (void *)&start);
                /*If range_end is specified as 0, the server will understand that the download will end when it reaches the end of the file.*/
                aiot_download_setopt(g_dl_handle, AIOT_DLOPT_RANGE_END, (void *)&end);
                /*Initiate a request to the server for the next download*/
                aiot_download_send_request(g_dl_handle);
                continue;
            }

            /*The entire firmware download is completed*/
            if (ret == STATE_DOWNLOAD_FINISHED) {
                printf("down finished all\n");
                aiot_download_deinit(&g_dl_handle);
                /*After completing the reception of the firmware, adjust the mqtt packet reception timeout back to the default value of 5000ms.*/
                timeout_ms = 5000;
                aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
                continue;
            }

            /*The downloaded content exceeds the total size of the firmware. It may be that the user has divided the range wrongly and there is overlap between ranges.*/
            if (ret == STATE_DOWNLOAD_FETCH_TOO_MANY) {
                printf("downloaded more than expeced bytes, please check range start/end settings\n");
                aiot_download_deinit(&g_dl_handle);
                /*After completing the reception of the firmware, adjust the mqtt packet reception timeout back to the default value of 5000ms.*/
                timeout_ms = 5000;
                aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
                continue;
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
    }

    /*Disconnect the MQTT connection, generally it will not run here*/
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_disconnect failed: -0x%04X\r\n", -res);
        goto exit;
    }

exit:
    while (1) {
        /*Destroy the MQTT instance, generally it will not run here*/
        res = aiot_mqtt_deinit(&mqtt_handle);

        if (res < STATE_SUCCESS) {
            printf("aiot_mqtt_deinit failed: -0x%04X\r\n", -res);
            return -1;
        } else {
            break;
        }
    }

    /*Destroy the OTA instance, generally it will not run here.*/
    aiot_ota_deinit(&ota_handle);

    return 0;
}


