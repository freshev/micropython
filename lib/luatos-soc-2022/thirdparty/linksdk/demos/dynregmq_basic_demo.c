/*This routine demonstrates using the SDK to configure the parameters of a dynamic registration session instance, initiate a request and receive a response, and then
 *
 * + If receiving the response fails, destroy the instance, recycle resources, and end the program to exit.
 * + If the response is received successfully, in the response processing callback function of `demo_dynregmq_recv_handler()`, demonstrate parsing and obtaining the content of the server response
 *
 * Parts that require user attention or modification have been marked in comments with `TODO`
 **/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_dynregmq_api.h"


/*TODO: Replace with the triplet of your own device*/
char *product_key       = "${YourProductKey}";
char *device_name       = "${YourDeviceName}";
char *product_secret    = "${YourProductSecret}";

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

uint8_t skip_pre_regist =
           1;        /*TODO: If you want to avoid pre-registration, you need to set this value to 1; if you need to pre-register the device on the console, set it to 0*/


/*Structure definition used to save deviceSecret in whitelist mode*/
typedef struct {
    char device_secret[64];
} demo_devinfo_wl_t;

/*Structural definition used to save mqtt connection establishment information clientid, username and password in whitelist-free mode*/
typedef struct {
    char conn_clientid[128];
    char conn_username[128];
    char conn_password[64];
} demo_devinfo_nwl_t;

/*A collection of system adaptation functions located in the portfiles/aiot_port folder*/
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/*Server certificate located in external/ali_ca_cert.c*/
extern const char *ali_ca_cert;

/*User saves the whitelist mode for dynamic registration, and the deviceSecret returned by the server*/
static demo_devinfo_wl_t demo_devinfo_wl;

/*User saves dynamic registration in whitelist-free mode, and the mqtt connection establishment information returned by the server*/
static demo_devinfo_nwl_t demo_devinfo_nwl;

/*TODO: If you want to close the log, make this function empty. If you want to reduce the log, you can choose not to print according to the code.
 *
 * For example: [1580995015.811][LK-040B] > POST /auth/register/device HTTP/1.1
 *
 * The code of the above log is 040B (hexadecimal). For the definition of code value, see components/dynregmq/aiot_dynregmq_api.h
 **/

/*Log callback function, SDK logs will be output from here*/
static int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/*Data processing callback, called when the SDK receives a dynregmq message from the network*/
void demo_dynregmq_recv_handler(void *handle, const aiot_dynregmq_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        /*TODO: In the callback, you need to copy and save the space contents pointed to by the packet, because after the callback returns, these spaces will be released by the SDK.*/
        case AIOT_DYNREGMQRECV_DEVICEINFO_WL: {
            if (strlen(packet->data.deviceinfo_wl.device_secret) >= sizeof(demo_devinfo_wl.device_secret)) {
                break;
            }

            /*In whitelist mode, users must persist device_secret.*/
            memset(&demo_devinfo_wl, 0, sizeof(demo_devinfo_wl_t));
            memcpy(demo_devinfo_wl.device_secret, packet->data.deviceinfo_wl.device_secret,
                   strlen(packet->data.deviceinfo_wl.device_secret));
        }
        break;
        /*TODO: In the callback, you need to copy and save the space contents pointed to by the packet, because after the callback returns, these spaces will be released by the SDK.*/
        case AIOT_DYNREGMQRECV_DEVICEINFO_NWL: {
            if (strlen(packet->data.deviceinfo_nwl.clientid) >= sizeof(demo_devinfo_nwl.conn_clientid) ||
                strlen(packet->data.deviceinfo_nwl.username) >= sizeof(demo_devinfo_nwl.conn_username) ||
                strlen(packet->data.deviceinfo_nwl.password) >= sizeof(demo_devinfo_nwl.conn_password)) {
                break;
            }

            /*In the whitelist-free mode, users must persist the MQTT connection establishment information clientid, username and password.*/
            memset(&demo_devinfo_nwl, 0, sizeof(demo_devinfo_nwl_t));
            memcpy(demo_devinfo_nwl.conn_clientid, packet->data.deviceinfo_nwl.clientid,
                   strlen(packet->data.deviceinfo_nwl.clientid));
            memcpy(demo_devinfo_nwl.conn_username, packet->data.deviceinfo_nwl.username,
                   strlen(packet->data.deviceinfo_nwl.username));
            memcpy(demo_devinfo_nwl.conn_password, packet->data.deviceinfo_nwl.password,
                   strlen(packet->data.deviceinfo_nwl.password));
        }
        break;
        default: {

        }
        break;
    }
}

int main(int argc, char *argv[])
{
    int32_t     res = STATE_SUCCESS;
    void       *dynregmq_handle = NULL;
    uint16_t    port = 443;            /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
    aiot_sysdep_network_cred_t
    cred;    /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/


    /*Configure the underlying dependencies of the SDK*/
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /*Configure the log output of the SDK*/
    aiot_state_set_logcb(demo_state_logcb);

    /*Create security credentials for the SDK, used to establish TLS connections*/
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /*Use RSA certificate to verify DYNREGMQ server*/
    cred.max_tls_fragment = 16384; /*The maximum fragment length is 16K, other optional values     are 4K, 2K, 1K, 0.5K*/
    cred.sni_enabled = 1;                               /*When establishing a TLS connection, support Server Name Indicator*/
    cred.x509_server_cert = ali_ca_cert;                 /*RSA root certificate used to verify the server*/
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /*The length of the RSA root certificate used to verify the server*/

    /*Create 1 dynregmq client instance and initialize default parameters internally*/
    dynregmq_handle = aiot_dynregmq_init();
    if (dynregmq_handle == NULL) {
        printf("aiot_dynregmq_init failed\n");
        return -1;
    }

    /*Configure the connected server address*/
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_HOST, (void *)mqtt_host);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_setopt AIOT_DYNREGMQOPT_HOST failed, res: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Configure the server port to connect to*/
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_PORT, (void *)&port);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_setopt AIOT_DYNREGMQOPT_PORT failed, res: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Configure device productKey*/
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_PRODUCT_KEY, (void *)product_key);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_setopt AIOT_DYNREGMQOPT_PRODUCT_KEY failed, res: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Configure device productSecret*/
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_PRODUCT_SECRET, (void *)product_secret);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_setopt AIOT_DYNREGMQOPT_PRODUCT_SECRET failed, res: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Configure device deviceName*/
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_DEVICE_NAME, (void *)device_name);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_setopt AIOT_DYNREGMQOPT_DEVICE_NAME failed, res: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Configure the security credentials for the network connection, which have been created above*/
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_NETWORK_CRED, (void *)&cred);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_setopt AIOT_DYNREGMQOPT_NETWORK_CRED failed, res: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Configure DYNREGMQ default message receiving callback function*/
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_RECV_HANDLER, (void *)demo_dynregmq_recv_handler);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_setopt AIOT_DYNREGMQOPT_RECV_HANDLER failed, res: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Configure DYNREGMQ dynamic registration mode,
    1. If configured as 0, it is whitelist mode. The user must enter the deviceName in the console in advance. After the dynamic registration is completed, the service will return deviceSecret. The user can pass
       AIOT_DYNREGMQRECV_DEVICEINFO_WL type data callback obtains deviceSecret.
    2. If configured to 1, it is whitelist-free mode. Users do not need to enter deviceName in the console in advance. After dynamic registration is completed, the service will return MQTT connection establishment information. Users can
       AIOT_DYNREGMQRECV_DEVICEINFO_NWL type data callback obtains clientid, username, password. The user needs to pass these three parameters
       The aiot_mqtt_setopt interface uses AIOT_MQTTOPT_CLIENTID, AIOT_MQTTOPT_USERNAME, AIOT_MQTTOPT_PASSWORD configuration options
       Configured into the MQTT handle.*/
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_NO_WHITELIST, (void *)&skip_pre_regist);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_setopt AIOT_DYNREGMQOPT_NO_WHITELIST failed, res: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Send dynamic registration request*/
    res = aiot_dynregmq_send_request(dynregmq_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_send_request failed: -0x%04X\n\r\n", -res);
        printf("please check variables like mqtt_host, produt_key, device_name, product_secret in demo\r\n");
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Receive dynamic registration requests*/
    res = aiot_dynregmq_recv(dynregmq_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_recv failed: -0x%04X\n", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /*Print the information in the service response*/
    if (skip_pre_regist == 0) {
        printf("device secret: %s\n", demo_devinfo_wl.device_secret);
    } else {
        printf("clientid: %s\n", demo_devinfo_nwl.conn_clientid);
        printf("username: %s\n", demo_devinfo_nwl.conn_username);
        printf("password: %s\n", demo_devinfo_nwl.conn_password);
    }

    /*Destroy dynamic registration session instance*/
    res = aiot_dynregmq_deinit(&dynregmq_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynregmq_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    return 0;
}

