#include "common_api.h"
#include "HTTPClient.h"
#include "luat_mobile.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_fs.h"
#include "luat_rtc.h"
#include "gpsmsg.h"
#include "osasys.h"

#define EPH_HOST "http://download.openluat.com/9501-xingli/HXXT_GPS_BDS_AGNSS_DATA.dat"
#define HTTP_RECV_BUF_SIZE      (1501)
#define HTTP_HEAD_BUF_SIZE      (800)

#define  EPH_TIME_FILE "/ephTime.txt"
#define  EPH_DATA_FILE "/ephData.bin"
#define  EPH_UPDATE_INTERVAL 4*3600

static luat_rtos_semaphore_t g_http_task_semaphore_handle;
static HttpClientContext gHttpClient = {0};
HANDLE gps_update_timer = NULL;
extern nmea_msg gpsx;

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
    FILE* fp1 = NULL;
    FILE* fp2 = NULL;

    LUAT_DEBUG_ASSERT(buf != NULL,0,0,0);

    clientData.headerBuf = malloc(HTTP_HEAD_BUF_SIZE);
    clientData.headerBufLen = HTTP_HEAD_BUF_SIZE;
    clientData.respBuf = buf;
    clientData.respBufLen = len;

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
            	LUAT_DEBUG_PRINT("total content length=%d", clientData.recvContentLength);
                fp1 = luat_fs_fopen(EPH_DATA_FILE,"w+");
                fp2 = luat_fs_fopen(EPH_TIME_FILE,"w+");
                utc_timer_value_t *timeutc = OsaSystemTimeReadUtc();
                luat_fs_fwrite((uint8_t *)&timeutc->UTCsecs, sizeof(timeutc->UTCsecs), 1, fp2);
                luat_fs_fclose(fp2);
                agps_start_timer(); 
            }

            if(clientData.blockContentLen > 0)
            {
            	LUAT_DEBUG_PRINT("response content:{%s}", (uint8_t*)clientData.respBuf);
                luat_fs_fwrite((uint8_t *)clientData.respBuf, clientData.blockContentLen, 1, fp1);
                // gps_writedata((uint8_t *)clientData.respBuf, clientData.blockContentLen);
            }
            count += clientData.blockContentLen;
            LUAT_DEBUG_PRINT("has recv=%d", count);
        }
    } while (result == HTTP_MOREDATA || result == HTTP_CONN);

    LUAT_DEBUG_PRINT("result=%d", result);
    luat_fs_fclose(fp1);
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

luat_rtos_task_handle agps_task_handle;

static void update_eph(void)
{
	char *recvBuf = (char *)luat_heap_malloc(HTTP_RECV_BUF_SIZE);
	HTTPResult result = HTTP_INTERNAL;
    gHttpClient.timeout_s = 2;
    gHttpClient.timeout_r = 20;
    while (!network_service_is_connect())
    {
        luat_rtos_task_sleep(1000);
    }
    result = httpConnect(&gHttpClient, EPH_HOST);
    if (result == HTTP_OK)
    {
        result = httpGetData(EPH_HOST, recvBuf, HTTP_RECV_BUF_SIZE);
        httpClose(&gHttpClient);
        luat_heap_free(recvBuf);
        if (result == HTTP_OK)
        {
            FILE* fp1 = luat_fs_fopen(EPH_DATA_FILE, "r");
            size_t size = luat_fs_fsize(EPH_DATA_FILE);
            uint8_t *data = NULL;
            data = (uint8_t *)luat_heap_malloc(size);
            luat_fs_fread(data, size, 1, fp1);
            for (size_t i = 0; i < size; i = i + 512)
            {
                if (i + 512 < size)
                    gps_writedata((uint8_t *)&data[i], 512);
                else
                    gps_writedata((uint8_t *)&data[i], size - i);
                luat_rtos_task_sleep(100);
            }
            luat_fs_fclose(fp1);
            luat_heap_free(data);
        }
    }
    else
    {
        LUAT_DEBUG_PRINT("http client connect error");
    }
}

static bool check_eph(int timeutc,int lstimeutc)
{   
    if((timeutc - lstimeutc) >= EPH_UPDATE_INTERVAL)
        return true;
    else
        return false;
}

static void luat_http_callback(void *param)
{
	luat_rtos_semaphore_release(g_http_task_semaphore_handle);
}

void agps_start_timer(void)
{
    luat_rtos_timer_start(gps_update_timer, EPH_UPDATE_INTERVAL * 1000, 0, luat_http_callback, NULL);  
}

static void agps_https_task(void *param)
{
    int lstimeutc = 0;
    size_t size = 0;
    uint8_t *data = NULL;
    if(gpsx.gpssta == 0)
    {
        FILE* fp = luat_fs_fopen(EPH_TIME_FILE, "r");
        if(fp)
        {
            luat_fs_fread(&lstimeutc, sizeof(lstimeutc), 1, fp);
            utc_timer_value_t *timeutc = OsaSystemTimeReadUtc();
            LUAT_DEBUG_PRINT("UTC TIME old %d now %d",lstimeutc,timeutc->UTCsecs);
            if(check_eph(timeutc->UTCsecs,lstimeutc))
            {
                update_eph();
            }
            else
            {
                FILE* fp1 = luat_fs_fopen(EPH_DATA_FILE, "r");
                size = luat_fs_fsize(EPH_DATA_FILE);
                data = (uint8_t *)luat_heap_malloc(size);
                luat_fs_fread(data, size, 1, fp1);
                for (size_t i = 0; i < size; i = i + 512)
                {
                    if (i + 512 < size)
                        gps_writedata((uint8_t *)&data[i], 512);
                    else
                        gps_writedata((uint8_t *)&data[i], size - i);
                    luat_rtos_task_sleep(100);
                }
                luat_fs_fclose(fp1);
                luat_heap_free(data);
            }
            luat_fs_fclose(fp);
        }
        else
        {
            update_eph();
        }
    }
    while(1)
	{
        luat_rtos_semaphore_take(g_http_task_semaphore_handle, LUAT_WAIT_FOREVER);
		update_eph();
	}
	luat_rtos_task_delete(agps_task_handle);
}

void agps_service_init(void)
{
    // The stack space required for https will be much larger
    luat_rtos_timer_create(&gps_update_timer);
    luat_rtos_semaphore_create(&g_http_task_semaphore_handle, 1);

	luat_rtos_task_create(&agps_task_handle, 2*1024, 30, "agps", agps_https_task, NULL, 0);
}

