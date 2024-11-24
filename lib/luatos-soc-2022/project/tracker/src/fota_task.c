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

#define PROJECT_VERSION "1.0.0"                  // If you use Hezhou iot to upgrade, this field must exist, and the mandatory fixed format is x.x.x, x can be any number
#define PROJECT_KEY "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" // Modify it to PRODUCT_KEY on your own iot. This is an error. This field must exist if you use Hezhou iot to upgrade.
#define PROJECT_NAME "LUAT_TRACKER"                 // If you use Hezhou iot to upgrade, this field must exist and can be modified at will, but it must be consistent with the upgrade package.

/*PROJECT_VERSION: used to distinguish different software versions, and also used to distinguish the differential basic version of firmware
    Assumptions:
        There are now two batches of Moduless running different base versions, one with version number 1.0.0 and the other with version number 2.0.0
        Now that both versions need to be upgraded to 3.0.0, two differential packages need to be made, one is to upgrade from 1.0.0 to 3.0.0, and the other is to upgrade from 2.0.0 to 3.0.0
        However, because Hezhou IOT uses firmware to distinguish different versions of firmware, as long as the firmware is the same at the time of request and the version number is higher than what the device is running, the upgrade file will be issued.
        Therefore, we need to do a little operation on the firmware and add a field to distinguish the differential packages with different basic versions.
        The firmware field generated when upgrading from 1.0.0 to 3.0.0 is 1.0.0_TEST_FOTA_CSDK_EC618
        The firmware field generated when upgrading from 2.0.0 to 3.0.0 is 2.0.0_TEST_FOTA_CSDK_EC618

        After doing this, when the two upgrade packages are put in, even if 1.0.0 is placed in the upgrade list of 2.0.0_TEST_FOTA_CSDK_EC618, the upgrade will be rejected by the server because the fields reported by itself are inconsistent with the firmware names in the upgrade list.

    Important note! ! ! ! ! ! ! ! ! ! ! ! ! !
    If the device is upgraded, the version running on the device must be consistent with the old version of firmware selected during the differential file.
    For example, software using the difference between 1.0.0 and 3.0.0 must and can only be used by devices running 1.0.0 software.*/

static char g_s_fota_server[200] = {0};
#define FOTA_URL "http://iot.openluat.com"
#define HTTP_RECV_BUF_SIZE (1501)
#define HTTP_HEAD_BUF_SIZE (800)

static HttpClientContext gHttpClient = {0};
luat_fota_img_proc_ctx_ptr luat_fota_img_handle = NULL;

const char *soc_get_sdk_type(void) // Users can reimplement this function and customize the version name
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
    HttpClientData clientData = {0};
    UINT32 count = 0;
    uint16_t headerLen = 0;
    int result1 = 0;

    LUAT_DEBUG_ASSERT(buf != NULL, 0, 0, 0);

    clientData.headerBuf = malloc(HTTP_HEAD_BUF_SIZE);
    clientData.headerBufLen = HTTP_HEAD_BUF_SIZE;
    clientData.respBuf = buf;
    clientData.respBufLen = len;

    result = httpSendRequest(&gHttpClient, getUrl, HTTP_GET, &clientData);
    LUAT_DEBUG_PRINT("send request result=%d", result);
    if (result != HTTP_OK)
        goto exit;
    do
    {
        LUAT_DEBUG_PRINT("recvResponse loop.");
        memset(clientData.headerBuf, 0, clientData.headerBufLen);
        memset(clientData.respBuf, 0, clientData.respBufLen);
        result = httpRecvResponse(&gHttpClient, &clientData);
        if (result == HTTP_OK || result == HTTP_MOREDATA)
        {
            headerLen = strlen(clientData.headerBuf);
            if (headerLen > 0)
            {
                LUAT_DEBUG_PRINT("total content length=%d", clientData.recvContentLength);
                luat_fota_img_handle = luat_fota_init();
            }

            if (clientData.blockContentLen > 0)
            {
                LUAT_DEBUG_PRINT("response content:{%s}", (uint8_t *)clientData.respBuf);
                result1 = luat_fota_write(luat_fota_img_handle, clientData.respBuf, clientData.blockContentLen);
                if (result1 == 0)
                {
                    LUAT_DEBUG_PRINT("fota update success");
                }
                else
                {
                    LUAT_DEBUG_PRINT("fota update error");
                }
            }
            count += clientData.blockContentLen;

            LUAT_DEBUG_PRINT("has recv=%d", count);
        }
    } while (result == HTTP_MOREDATA || result == HTTP_CONN);

    LUAT_DEBUG_PRINT("result=%d", result);
    if (gHttpClient.httpResponseCode < 200 || gHttpClient.httpResponseCode > 404)
    {
        LUAT_DEBUG_PRINT("invalid http response code=%d", gHttpClient.httpResponseCode);
    }
    else if (count == 0 || count != clientData.recvContentLength)
    {
        LUAT_DEBUG_PRINT("data not receive complete");
    }
    else
    {
        LUAT_DEBUG_PRINT("receive success");
    }
exit:
    free(clientData.headerBuf);
    return result;
}

luat_rtos_semaphore_t net_semaphore_handle;
luat_rtos_task_handle fota_task_handle;

static void fota_task(void *param)
{

    while (1)
    {
        while (network_service_is_ready() != 1)
        {
            luat_rtos_task_sleep(1000);
        }

        char *recvBuf = malloc(HTTP_RECV_BUF_SIZE);
        HTTPResult result = HTTP_INTERNAL;
        luat_rtos_task_sleep(3000);
        LUAT_DEBUG_PRINT("version = %s", PROJECT_VERSION);

        gHttpClient.timeout_s = 2;
        gHttpClient.timeout_r = 20;
        gHttpClient.seclevel = 1;
        gHttpClient.ciphersuite[0] = 0xFFFF;
        gHttpClient.ignore = 1;
        char imei[16] = {0};
        luat_mobile_get_imei(0, imei, 15);
        snprintf(g_s_fota_server, 200, "%s/api/site/firmware_upgrade?project_key=%s&imei=%s&device_key=&firmware_name=%s_%s_%s_%s&version=%s", FOTA_URL, PROJECT_KEY, imei, PROJECT_VERSION, PROJECT_NAME, soc_get_sdk_type(), "EC618", PROJECT_VERSION);
        LUAT_DEBUG_PRINT("print fota url %s", g_s_fota_server);
        result = httpConnect(&gHttpClient, FOTA_URL);
        if (result == HTTP_OK)
        {
            httpGetData(g_s_fota_server, recvBuf, HTTP_RECV_BUF_SIZE);
            httpClose(&gHttpClient);
            LUAT_DEBUG_PRINT("verify start");
            int verify = luat_fota_done(luat_fota_img_handle);
            if (verify != 0)
            {
                LUAT_DEBUG_PRINT("image_verify error");
                goto exit;
            }
            LUAT_DEBUG_PRINT("image_verify ok");
            ResetStartPorReset(RESET_REASON_FOTA);
        }
        else
        {
            LUAT_DEBUG_PRINT("http client connect error");
        }
    exit:
        free(recvBuf);


    luat_rtos_task_sleep(12 * 60 * 60 * 1000);
    }
    luat_rtos_task_delete(fota_task_handle);
}

void fota_task_init(void)
{
    luat_rtos_task_create(&fota_task_handle, 2 * 1024, 20, "fota", fota_task, NULL, NULL);
}
