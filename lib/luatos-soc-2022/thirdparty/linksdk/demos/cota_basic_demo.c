/** This example is mainly used to demonstrate how to obtain the remote configuration of this device stored in the cloud from the device
 *
 * Parts that require user attention or modification have been marked in comments with `TODO`
 **/

/*TODO: This example uses the sleep function, so unistd.h is included. If the user has an alternative function in his own library, unistd.h can be replaced.
 **/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
void *g_dl_handle = NULL;

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
    /*When downloading remote configuration, there will be a large number of HTTP packet collection logs, which can be filtered out and closed by code.*/
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
/*TODO: After the user receives the remotely configured data, he or she needs to decide how to process it.*/
void demo_download_recv_handler(void *handle, const aiot_download_recv_t *packet, void *userdata)
{
    /*Currently only supports the case where packet->type is AIOT_DLRECV_HTTPBODY*/
    if (!packet || AIOT_DLRECV_HTTPBODY != packet->type) {
        return;
    }
    int32_t percent = packet->data.percent;
    uint8_t *src_buffer = packet->data.buffer;
    uint32_t src_buffer_len = packet->data.len;

    /*If percent is a negative number, it means that a packet receiving exception or digest verification error occurred.*/
    if (percent < 0) {
        /*Packet receiving exception or digest verification error*/
        printf("exception happend, percent is %d\r\n", percent);
        return;
    }
    aiot_download_report_progress(handle, percent);
    printf("config len is %d, config content is %.*s\r\n", src_buffer_len, src_buffer_len, (char *)src_buffer);
}

/*The OTA message processing callback registered by the user through aiot_ota_setopt(). If the SDK receives an OTA-related MQTT message, it will automatically identify and call this callback function.*/
void demo_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    switch (ota_msg->type) {
    case AIOT_OTARECV_COTA: {
        uint16_t port = 443;
        uint32_t max_buffer_len = 2048;
        aiot_sysdep_network_cred_t cred;
        void *dl_handle = aiot_download_init();

        if (NULL == dl_handle || NULL == ota_msg->task_desc || ota_msg->task_desc->protocol_type != AIOT_OTA_PROTOCOL_HTTPS) {
            return;
        }

        printf("configId: %s, configSize: %u Bytes\r\n", ota_msg->task_desc->version,
               ota_msg->task_desc->size_total);

        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;
        cred.max_tls_fragment = 16384;
        cred.x509_server_cert = ali_ca_cert;
        cred.x509_server_cert_len = strlen(ali_ca_cert);

        /*Set download to TLS when downloading*/
        if ((STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred))) ||
                /*Set the server port number accessed when downloading*/
                (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_NETWORK_PORT, (void *)(&port))) ||
                /*Set the downloaded task information, which is obtained by inputting the task_desc member in the parameter ota_msg, including the download address, remote configuration size and version number.*/
                (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc))) ||
                /*Set the callback function that the SDK will call when the downloaded content arrives.*/
                (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_RECV_HANDLER, (void *)(demo_download_recv_handler))) ||
                /*Set the maximum buffer length for a single download. The user will be notified whenever the memory of this length is full.*/
                (STATE_SUCCESS != aiot_download_setopt(dl_handle, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len))) ||
                /*Send an http GET request to the http server*/
                (STATE_SUCCESS != aiot_download_send_request(dl_handle))) {
            aiot_download_deinit(&dl_handle);
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
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_RECV_HANDLER, demo_ota_recv_handler);

    /*Establish an MQTT connection with the server*/
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_connect failed: -0x%04X\r\n\r\n", -res);
        printf("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        /*The attempt to establish a connection failed, the MQTT instance was destroyed, and the resources were recycled.*/
        goto exit;
    }


    /*Send a request to obtain the remote configuration from the cloud. TODO: Replace the productKey and deviceName of your own device*/
    /*
     {
         char *topic_string = "/sys/{YourProductKey}/{YourDeviceName}/thing/config/get";
         char *payload_string = "{\"id\":\"123\",\"params\":{\"configScope\":\"product\",\"getType\":\"file\"}}";

         res = aiot_mqtt_pub(mqtt_handle, topic_string, (uint8_t *)payload_string, strlen(payload_string), 0);
         if (res < STATE_SUCCESS) {
             printf("aiot_mqtt_pub failed: -0x%04X\r\n", -res);
             goto exit;
         }
     }
     */

    while (1) {
        aiot_mqtt_process(mqtt_handle);
        res = aiot_mqtt_recv(mqtt_handle);

        if (res == STATE_SYS_DEPEND_NWK_CLOSED) {
            sleep(1);
        }
        if (NULL != g_dl_handle) {
            /*Before completing the reception of remote configuration, adjust the mqtt packet reception timeout to 100ms to reduce the interval between downloads of two remote configurations.*/
            int32_t ret = aiot_download_recv(g_dl_handle);
            timeout_ms = 100;

            if (STATE_DOWNLOAD_FINISHED == ret) {
                aiot_download_deinit(&g_dl_handle);
                /*After completing the reception of remote configuration, adjust the mqtt packet reception timeout back to the default value of 5000ms.*/
                timeout_ms = 5000;
            }
            aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
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


