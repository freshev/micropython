/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "common_api.h"
#include "luat_mobile.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_fota.h"
#include "reset.h"
#include "HTTPClient.h"

/*For FOTA application development, please refer to: https://doc.openluat.com/wiki/37?wiki_page_id=4727*/

#define PROJECT_VERSION  "1.0.0"                  //If you use Hezhou iot to upgrade, this field must exist, and the mandatory fixed format is x.x.x, x can be any number
#define PROJECT_KEY "ABCDEFGHIJKLMNOPQRSTUVWXYZ"  //Modify it to PRODUCT_KEY on your own iot. This is an error. This field must exist if you use Hezhou iot to upgrade.
#define PROJECT_NAME "TEST_FOTA"                  //If you use Hezhou iot to upgrade, this field must exist and can be modified at will, but it must be consistent with the upgrade package.

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

char g_test_server_name[200] = {0};
#define IOT_FOTA_URL "http://iot.openluat.com"
#define HTTP_RECV_BUF_SIZE      (1501)
#define HTTP_HEAD_BUF_SIZE      (800)



static HttpClientContext        gHttpClient = {0};
luat_fota_img_proc_ctx_ptr test_luat_fota_handle = NULL;

static int stepLen = 0;
static int totalLen = 0;

const char *soc_get_sdk_type(void) //Users can reimplement this function and customize the version name
{
	return "CSDK";
}

/**
  \fn      INT32 httpGetData(CHAR *getUrl, CHAR *buf, UINT32 len)
  \brief
  \return
*/
static INT32 httpGetData(CHAR *getUrl, CHAR *buf, UINT32 len)
{
    HTTPResult result = HTTP_INTERNAL;
    HttpClientData    clientData = {0};
    UINT32 count = 0;
    uint16_t headerLen = 0;
    int result1 = 0;

    LUAT_DEBUG_ASSERT(buf != NULL,0,0,0);

    clientData.headerBuf = malloc(HTTP_HEAD_BUF_SIZE);
    clientData.headerBufLen = HTTP_HEAD_BUF_SIZE;
    clientData.respBuf = buf;
    clientData.respBufLen = len;
    if(stepLen != 0)
    {
        clientData.isRange = true;
        clientData.rangeHead = stepLen;
        clientData.rangeTail = -1;
    }
    result = httpSendRequest(&gHttpClient, getUrl, HTTP_GET, &clientData);
    LUAT_DEBUG_PRINT("send request result=%d", result);
    if (result != HTTP_OK)
        goto exit;
    do {
    	LUAT_DEBUG_PRINT("recvResponse loop.");
        memset(clientData.headerBuf, 0, clientData.headerBufLen);
        memset(clientData.respBuf, 0, clientData.respBufLen);
        result = httpRecvResponse(&gHttpClient, &clientData);
        if(result == HTTP_OK || result == HTTP_MOREDATA){
            headerLen = strlen(clientData.headerBuf);
            if(headerLen > 0)
            {
                if(stepLen == 0)
                {
                    totalLen = clientData.recvContentLength;
            	    LUAT_DEBUG_PRINT("total content length=%d", clientData.recvContentLength);
                }
            }
            if(clientData.blockContentLen > 0)
            {
            	LUAT_DEBUG_PRINT("response content:{%s}", (uint8_t*)clientData.respBuf);
                result1 = luat_fota_write(test_luat_fota_handle,clientData.respBuf,clientData.blockContentLen);
                if (result1==0)
                {
                    LUAT_DEBUG_PRINT("fota update success");
                }
                else{
                    LUAT_DEBUG_PRINT("fota update error");
                }
            }
            stepLen += clientData.blockContentLen;
            count += clientData.blockContentLen;
            LUAT_DEBUG_PRINT("has recv=%d", count);
        }
    } while (result == HTTP_MOREDATA || result == HTTP_CONN);

    LUAT_DEBUG_PRINT("result=%d", result);
    if (gHttpClient.httpResponseCode < 200 || gHttpClient.httpResponseCode > 404)
    {
    	LUAT_DEBUG_PRINT("invalid http response code=%d",gHttpClient.httpResponseCode);
    } else if (count == 0 || count != clientData.recvContentLength) {
    	LUAT_DEBUG_PRINT("data not receive complete");
    } else {
    	LUAT_DEBUG_PRINT("receive success");
    }
exit:
    free(clientData.headerBuf);
    return result;
}

luat_rtos_task_handle fota_task_handle;
luat_rtos_task_handle print_task_handle;

static uint8_t g_s_network_status = 0;

void mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status){
    if (event == LUAT_MOBILE_EVENT_NETIF)
    {
        if(status == LUAT_MOBILE_NETIF_LINK_ON)
        {
            g_s_network_status = 1;
            LUAT_DEBUG_PRINT("network ready");
        }
        else if(status == LUAT_MOBILE_NETIF_LINK_OFF)
        {
            g_s_network_status = 0;
            LUAT_DEBUG_PRINT("network off");
        }
    }
}

static void task_test_fota(void *param)
{
    luat_mobile_event_register_handler(mobile_event_callback);
    uint8_t retryTimes = 0;
	char *recvBuf = malloc(HTTP_RECV_BUF_SIZE);
	HTTPResult result = HTTP_INTERNAL;
    LUAT_DEBUG_PRINT("version = %s", PROJECT_VERSION);
    gHttpClient.timeout_s = 2;
    gHttpClient.timeout_r = 20;
    gHttpClient.ignore = 1;
    char imei[16] = {0};
    luat_mobile_get_imei(0, imei, 15);

    //The first upgrade method
    snprintf(g_test_server_name, 200, "%s/api/site/firmware_upgrade?project_key=%s&imei=%s&device_key=&firmware_name=%s_%s_%s_%s&version=%s", IOT_FOTA_URL, PROJECT_KEY, imei, PROJECT_VERSION, PROJECT_NAME, soc_get_sdk_type(), "EC618", PROJECT_VERSION);
   
    // The second upgrade method
    // snprintf(g_test_server_name, 200, "%s/api/site/firmware_upgrade?project_key=%s&imei=%s&device_key=&firmware_name=%s_%s_%s&version=%s", IOT_FOTA_URL, PROJECT_KEY, imei, PROJECT_NAME, soc_get_sdk_type(), "EC618", PROJECT_VERSION);
    LUAT_DEBUG_PRINT("test print url %s", g_test_server_name);
    test_luat_fota_handle = luat_fota_init();
    while(retryTimes < 30)
    {
        while(g_s_network_status != 1)
        {
            LUAT_DEBUG_PRINT("fota task wait network ready");
            luat_rtos_task_sleep(1000);
        }
        result = httpConnect(&gHttpClient, IOT_FOTA_URL);
        if (result == HTTP_OK)
        {
            httpGetData(g_test_server_name, recvBuf, HTTP_RECV_BUF_SIZE);
            httpClose(&gHttpClient);
            LUAT_DEBUG_PRINT("verify start");
            if (stepLen == totalLen)
            {
                int verify = luat_fota_done(test_luat_fota_handle);
                if(verify != 0)
                {
                    LUAT_DEBUG_PRINT("image_verify error");
                    goto exit;
                }
	            LUAT_DEBUG_PRINT("image_verify ok");
                ResetStartPorReset(RESET_REASON_FOTA);
            }
        }
        else
        {
            LUAT_DEBUG_PRINT("http client connect error");
        }
        retryTimes++;
        luat_rtos_task_sleep(5000);
    }
    exit:
    stepLen = 0;
    totalLen = 0;
    free(recvBuf);
    luat_rtos_task_delete(fota_task_handle);
}

static void print_version_task(void *param)
{
    while(1)
    {
        LUAT_DEBUG_PRINT("version: %s", PROJECT_VERSION);
        luat_rtos_task_sleep(1000);
    }
    luat_rtos_task_delete(print_task_handle);
}

static void task_demo_fota(void)
{
	luat_rtos_task_create(&fota_task_handle, 32 * 1024, 20, "task_test_fota", task_test_fota, NULL, NULL);
}
static void print_task_demo_fota(void)
{
	luat_rtos_task_create(&print_task_handle, 1024, 20, "print_version_task", print_version_task, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_fota, "1");
INIT_TASK_EXPORT(print_task_demo_fota, "1");

