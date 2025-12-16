/*This routine demonstrates using the SDK to configure the parameters of a dynamic registration session instance, initiate a request and receive a response, and then
 *
 * + If receiving the response fails, destroy the instance, recycle resources, and end the program to exit.
 * + If the response is received successfully, in the response processing callback function of `demo_dynreg_recv_handler()`, demonstrate parsing and obtaining the content of the server response
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
#include "aiot_dynreg_api.h"

typedef struct {
    uint32_t code;
    char *device_secret;
} demo_info_t;

/*A collection of system adaptation functions located in the portfiles/aiot_port folder*/
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/*Server certificate located in external/ali_ca_cert.c*/
extern const char *ali_ca_cert;

/*TODO: If you want to close the log, make this function empty. If you want to reduce the log, you can choose not to print according to the code.
 *
 * For example: [1580995015.811][LK-040B] > POST /auth/register/device HTTP/1.1
 *
 * The code of the above log is 040B (hexadecimal). For the definition of code value, see components/dynreg/aiot_dynreg_api.h
 **/

/*Log callback function, SDK logs will be output from here*/
static int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/*Data processing callback, called when the SDK receives a dynreg message from the network*/
void demo_dynreg_recv_handler(void *handle, const aiot_dynreg_recv_t *packet, void *userdata)
{
    demo_info_t *demo_info = (demo_info_t *)userdata;

    switch (packet->type) {
        case AIOT_DYNREGRECV_STATUS_CODE: {
            demo_info->code = packet->data.status_code.code;
        }
        break;
        /*TODO: In the callback, you need to copy and save the space contents pointed to by the packet, because after the callback returns, these spaces will be released by the SDK.*/
        case AIOT_DYNREGRECV_DEVICE_INFO: {
            demo_info->device_secret = malloc(strlen(packet->data.device_info.device_secret) + 1);
            if (demo_info->device_secret != NULL) {
                memset(demo_info->device_secret, 0, strlen(packet->data.device_info.device_secret) + 1);
                memcpy(demo_info->device_secret, packet->data.device_info.device_secret,
                       strlen(packet->data.device_info.device_secret));
            }
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
    void       *dynreg_handle = NULL;
    char       *host = "iot-auth.cn-shanghai.aliyuncs.com"; /*Alibaba Cloud platform dynamically registers domestic sites*/
    uint16_t    port = 443;      /*Regardless of whether the device uses TLS to connect to the Alibaba Cloud platform, the destination port is 443*/
    aiot_sysdep_network_cred_t cred; /*Security credentials structure. If TLS is to be used, parameters such as the CA certificate are configured in this structure.*/
    demo_info_t demo_info;

    /*TODO: Replace with the productKey, productSecret and deviceName of your own device*/
    char *product_key       = "a13FN5TplKq";
    char *product_secret    = "y7GSILD480lBSsP8";
    char *device_name       = "dynreg_basic_demo";

    memset(&demo_info, 0, sizeof(demo_info_t));

    /*Configure the underlying dependencies of the SDK*/
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /*Configure the log output of the SDK*/
    aiot_state_set_logcb(demo_state_logcb);

    /*Create security credentials for the SDK, used to establish TLS connections*/
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /*Use RSA certificate to verify DYNREG server*/
    cred.max_tls_fragment = 16384; /*The maximum fragment length is 16K, other optional values     are 4K, 2K, 1K, 0.5K*/
    cred.sni_enabled = 1;                               /*When establishing a TLS connection, support Server Name Indicator*/
    cred.x509_server_cert = ali_ca_cert;                 /*RSA root certificate used to verify the server*/
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /*The length of the RSA root certificate used to verify the server*/

    /*Create 1 dynreg client instance and initialize default parameters internally*/
    dynreg_handle = aiot_dynreg_init();
    if (dynreg_handle == NULL) {
        printf("aiot_dynreg_init failed\n");
        return -1;
    }

    /*Configure the connected server address*/
    res = aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_HOST, (void *)host);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_setopt AIOT_DYNREGOPT_HOST failed, res: -0x%04X\n", -res);
        aiot_dynreg_deinit(&dynreg_handle);
        return -1;
    }

    /*Configure the server port to connect to*/
    res = aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_PORT, (void *)&port);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_setopt AIOT_DYNREGOPT_PORT failed, res: -0x%04X\n", -res);
        aiot_dynreg_deinit(&dynreg_handle);
        return -1;
    }

    /*Configure device productKey*/
    res = aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_PRODUCT_KEY, (void *)product_key);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_setopt AIOT_DYNREGOPT_PRODUCT_KEY failed, res: -0x%04X\n", -res);
        aiot_dynreg_deinit(&dynreg_handle);
        return -1;
    }

    /*Configure device productSecret*/
    res = aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_PRODUCT_SECRET, (void *)product_secret);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_setopt AIOT_DYNREGOPT_PRODUCT_SECRET failed, res: -0x%04X\n", -res);
        aiot_dynreg_deinit(&dynreg_handle);
        return -1;
    }

    /*Configure device deviceName*/
    res = aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_DEVICE_NAME, (void *)device_name);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_setopt AIOT_DYNREGOPT_DEVICE_NAME failed, res: -0x%04X\n", -res);
        aiot_dynreg_deinit(&dynreg_handle);
        return -1;
    }

    /*Configure the security credentials for the network connection, which have been created above*/
    res = aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_NETWORK_CRED, (void *)&cred);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_setopt AIOT_DYNREGOPT_NETWORK_CRED failed, res: -0x%04X\n", -res);
        aiot_dynreg_deinit(&dynreg_handle);
        return -1;
    }

    /*Configure DYNREG default message receiving callback function*/
    res = aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_RECV_HANDLER, (void *)demo_dynreg_recv_handler);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_setopt AIOT_DYNREGOPT_RECV_HANDLER failed, res: -0x%04X\n", -res);
        aiot_dynreg_deinit(&dynreg_handle);
        return -1;
    }

    /*Sets the user context that will be returned when demo_dynreg_recv_handler is called*/
    res = aiot_dynreg_setopt(dynreg_handle, AIOT_DYNREGOPT_USERDATA, (void *)&demo_info);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_setopt AIOT_DYNREGOPT_USERDATA failed, res: -0x%04X\n", -res);
        aiot_dynreg_deinit(&dynreg_handle);
        return -1;
    }

    /*Send dynamic registration request*/
    res = aiot_dynreg_send_request(dynreg_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_send_request failed: -0x%04X\n", -res);
        return -1;
    }

    /*Receive dynamic registration requests*/
    res = aiot_dynreg_recv(dynreg_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_recv failed: -0x%04X\n", -res);
        return -1;
    }

    printf("status code: %d\n", demo_info.code);

    /*Print the deviceSecret in the service response*/
    if (demo_info.device_secret != NULL) {
        printf("device secret: %s\n", demo_info.device_secret);
        free(demo_info.device_secret);
    }

    /*Destroy dynamic registration session instance*/
    res = aiot_dynreg_deinit(&dynreg_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_dynreg_deinit failed: -0x%04X\n", -res);
        return -1;
    }

    return 0;
}

