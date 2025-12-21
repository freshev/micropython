#include "common_api.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_mobile.h"
#include "luat_network_adapter.h"
#include "networkmgr.h"
#include "luat_http.h"
#include "luat_fota.h"

/*For FOTA application development, please refer to: https://doc.openluat.com/wiki/37?wiki_page_id=4727*/


#define USE_CUSTOM_URL 0

luat_fota_img_proc_ctx_ptr test_luat_fota_handle = NULL;

#if USE_CUSTOM_URL == 0

#define IOT_FOTA_URL "http://iot.openluat.com"
#define PROJECT_VERSION  "1.0.1"                            //If you use Hezhou iot to upgrade, this field must exist, and the mandatory fixed format is x.x.x, x can be any number
#define PROJECT_KEY "ABCDEFGHIJKLMNOPQRSTUVWXYZ"            //Modify it to PRODUCT_KEY on your own iot. This is an error. This field must exist if you use Hezhou iot to upgrade.
#define PROJECT_NAME "TEST_FOTA"                            //If you use Hezhou iot to upgrade, this field must exist and can be modified at will, but it must be consistent with the upgrade package.

#else

#define OTA_URL        "http://airtest.openluat.com:2900/download/csdk_delta_test.par"  //If you use a custom url to upgrade, just modify this field.

#endif

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 ) {
    mbedtls_sha256_starts(ctx, is224);
}
int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen ) {
    mbedtls_sha256_update(ctx, input, ilen);
}
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32]) {
    mbedtls_sha256_finish(ctx, output);
}
#endif


/*Things to note! ! ! ! ! ! ! ! ! ! ! ! !

    Important note! ! ! ! ! ! ! ! ! ! ! ! ! !
    If the device is upgraded, the version running on the device must be consistent with the old version of firmware selected during the differential file.
    For example, the difference package generated using the difference between 1.0.0 and 3.0.0 must and can only be used by devices running 1.0.0 software.*/

/*The first way to upgrade! ! ! ! ! ! ! ! ! !
    This method modifies the version name reported by the device itself, and does not require special version management of the device and upgrade package. This upgrade method is recommended when upgrading with the Hezhou IoT platform.

    PROJECT_VERSION: used to distinguish different software versions, and also used to distinguish the differential basic version of firmware

    Assumptions:
        There are now two batches of Moduless running different base versions, one with version number 1.0.0 and the other with version number 2.0.0
        Now that both versions need to be upgraded to 3.0.0, two differential packages need to be made, one is to upgrade from 1.0.0 to 3.0.0, and the other is to upgrade from 2.0.0 to 3.0.0
        
        However, because Hezhou IOT uses firmware to distinguish different versions of firmware, as long as the firmware is the same at the time of request and the version number is higher than what the device is running, the upgrade file will be issued.

        Therefore, we need to do a little operation on the firmware field and add PROJECT_VERSION to the field to distinguish differential packages with different basic versions.

        Fireware field before adding field
                The firmware field generated when upgrading from 1.0.0 to 3.0.0 is TEST_FOTA_CSDK_EC618
                The firmware field generated when upgrading from 2.0.0 to 3.0.0 is TEST_FOTA_CSDK_EC618

        Fireware field after adding field
                The firmware field generated when upgrading from 1.0.0 to 3.0.0 is 1.0.0_TEST_FOTA_CSDK_EC618
                The firmware field generated when upgrading from 2.0.0 to 3.0.0 is 2.0.0_TEST_FOTA_CSDK_EC618

        After doing this, when the two upgrade packages are put in, even if 1.0.0 is placed in the upgrade list of 2.0.0_TEST_FOTA_CSDK_EC618, the upgrade will be rejected by the server because the fields reported by itself are inconsistent with the firmware names in the upgrade list.*/



/*The second way to upgrade! ! ! ! ! ! ! ! ! !
    This method can count the number of successfully upgraded devices on the Hezhou IoT platform, but it requires users to do version management of the devices and upgrade packages themselves. This upgrade method is not recommended when upgrading with the Hezhou IoT platform.

    PROJECT_VERSION: used to distinguish between different software versions

    Assumptions:
        There are now two batches of Moduless running different base versions, one with version number 1.0.0 and the other with version number 2.0.0
        Now that both versions need to be upgraded to 3.0.0, two differential packages need to be made, one is to upgrade from 1.0.0 to 3.0.0, and the other is to upgrade from 2.0.0 to 3.0.0

        Hezhou IOT uses firmware to distinguish different versions of firmware. As long as the firmware is the same at the time of request and the version number is higher than the one running on the device, the upgrade file will be issued.
        
        The firmware field generated when upgrading from 1.0.0 to 3.0.0 is TEST_FOTA_CSDK_EC618
        The firmware field generated when upgrading from 2.0.0 to 3.0.0 is TEST_FOTA_CSDK_EC618

        If imei of a device running 1.0.0 is put into the upgrade list of 2.0.0-3.0.0, because the firmware field reported by the device is the same, the server will also deliver the differential software of 2.0.0-3.0.0 to the running 1.0.0 Software Devices
        Therefore, users must distinguish device versions themselves, otherwise they will keep requesting the wrong differential packets, resulting in traffic loss.*/


/*Compared with the first method, the second upgrade method has the function of counting the number of successfully upgraded devices on the Hezhou IoT platform, but it requires the user to manage the equipment and software versions themselves.
    
    Overall it does more harm than good

    It is recommended to use the first method for upgrading. This demo also uses the first method for upgrade demonstration by default! ! ! ! ! !*/

static luat_rtos_task_handle g_s_task_handle;

enum
{
    OTA_HTTP_GET_HEAD_DONE = 1,
    OTA_HTTP_GET_DATA,
    OTA_HTTP_GET_DATA_DONE,
    OTA_HTTP_FAILED,
};
static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
    if (LUAT_MOBILE_EVENT_NETIF == event)
    {
        if (LUAT_MOBILE_NETIF_LINK_ON == status)
        {
            luat_socket_check_ready(index, NULL);
        }
    }
}

static void luatos_http_cb(int status, void *data, uint32_t len, void *param)
{
    uint8_t *ota_data;
    if(status < 0) 
    {
        LUAT_DEBUG_PRINT("http failed! %d", status);
        luat_rtos_event_send(param, OTA_HTTP_FAILED, 0, 0, 0, 0);
        return;
    }
    switch(status)
    {
    case HTTP_STATE_GET_BODY:
        if (data)
        {
            ota_data = malloc(len);
            memcpy(ota_data, data, len);
            luat_rtos_event_send(param, OTA_HTTP_GET_DATA, ota_data, len, 0, 0);
        }
        else
        {
            luat_rtos_event_send(param, OTA_HTTP_GET_DATA_DONE, 0, 0, 0, 0);
        }
        break;
    case HTTP_STATE_GET_HEAD:
        if (data)
        {
            LUAT_DEBUG_PRINT("%s", data);
        }
        else
        {
            luat_rtos_event_send(param, OTA_HTTP_GET_HEAD_DONE, 0, 0, 0, 0);
        }
        break;
    case HTTP_STATE_IDLE:
        break;
    case HTTP_STATE_SEND_BODY_START:
        //If it is POST, send the POST body data here. If it cannot be sent completely at one time, you can continue to send it in the HTTP_STATE_SEND_BODY callback.
        break;
    case HTTP_STATE_SEND_BODY:
        //If it is POST, you can send the remaining body data of POST here
        break;
    default:
        break;
    }
}


static void luat_test_task(void *param)
{
    luat_event_t event;
    int result, is_error;
    uint8_t is_end = 0;
    /*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! ! 1*/
    luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET);
    uint32_t all,now_free_block,min_free_block,done_len;
    luat_http_ctrl_t *http = luat_http_client_create(luatos_http_cb, luat_rtos_get_current_handle(), -1);
    const char remote_domain[200];
#if USE_CUSTOM_URL == 0
    char imei[16] = {0};
    luat_mobile_get_imei(0, imei, 15);
    //The first upgrade method
    snprintf(remote_domain, 200, "%s/api/site/firmware_upgrade?project_key=%s&imei=%s&device_key=&firmware_name=%s_%s_%s_%s&version=%s", IOT_FOTA_URL, PROJECT_KEY, imei, PROJECT_VERSION, PROJECT_NAME, soc_get_sdk_type(), "EC618", PROJECT_VERSION);

    // The second upgrade method
    // snprintf(remote_domain, 200, "%s/api/site/firmware_upgrade?project_key=%s&imei=%s&device_key=&firmware_name=%s_%s_%s&version=%s", IOT_FOTA_URL, PROJECT_KEY, imei, PROJECT_NAME, soc_get_sdk_type(), "EC618", PROJECT_VERSION);
#else
    snprintf(remote_domain, 200, "%s", OTA_URL);
#endif
    
    LUAT_DEBUG_PRINT("print url %s", remote_domain);
    test_luat_fota_handle = luat_fota_init();
    luat_http_client_start(http, remote_domain, 0, 0, 1);

    while (!is_end)
    {
        luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
        switch(event.id)
        {
        case OTA_HTTP_GET_HEAD_DONE:
            done_len = 0;
            LUAT_DEBUG_PRINT("status %d total %u", luat_http_client_get_status_code(http), http->total_len);
            break;
        case OTA_HTTP_GET_DATA:
            done_len += event.param2;
            result = luat_fota_write(test_luat_fota_handle, event.param1, event.param2);
            free(event.param1);
            break;
        case OTA_HTTP_GET_DATA_DONE:
            is_end = 1;
            break;
        case OTA_HTTP_FAILED:
            is_end = 1;
            break;
        default:
            break;
        }
    }

    is_error = luat_fota_done(test_luat_fota_handle);
    if(is_error != 0)
    {
        LUAT_DEBUG_PRINT("image_verify error");
        LUAT_DEBUG_PRINT("ota test failed");
    }
    else
    {
        luat_pm_reboot();
    }
    luat_http_client_close(http);
    luat_http_client_destroy(&http);
    luat_meminfo_sys(&all, &now_free_block, &min_free_block);
    LUAT_DEBUG_PRINT("meminfo %d,%d,%d",all,now_free_block,min_free_block);
    while(1)
    {
        luat_rtos_task_sleep(60000);
    }
}

static void luat_test_init(void)
{
    luat_mobile_event_register_handler(luatos_mobile_event_callback);
    luat_mobile_set_period_work(0, 5000, 0);
    net_lwip_init();
    net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
    network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
    luat_rtos_task_create(&g_s_task_handle, 4 * 1024, 50, "test", luat_test_task, NULL, 16);

}

INIT_TASK_EXPORT(luat_test_init, "1");
