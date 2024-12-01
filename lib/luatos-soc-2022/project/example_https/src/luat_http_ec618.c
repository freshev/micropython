#include "common_api.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_mobile.h"
#include "luat_network_adapter.h"
#include "networkmgr.h"
#include "luat_http.h"
#define IS_used_HTTP 0
//The demo demonstrates the basic functions of http. For the post function, please refer to example_luatos_htpp_post.

static luat_http_ctrl_t *g_s_http_client;
static luat_rtos_task_handle luatos_http_task_handle;

enum
{
	LUAT_HTTP_GET_HEAD_DONE = 1,
	LUAT_HTTP_GET_DATA,
	LUAT_HTTP_GET_DATA_DONE,
	LUAT_HTTP_FAILED,
};
#if IS_used_HTTP==0
	char *http_url="http://airtest.openluat.com:2900/download/hiluatos.txt";
#else
	char *http_url="https://tools.openluat.com/api/resource/audio/hiluatos.txt";
#endif

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
	uint32_t *http_down_data;
    if(status < 0) 
    {
        LUAT_DEBUG_PRINT("http failed! %d", status);
		luat_rtos_event_send(param, LUAT_HTTP_FAILED, 0, 0, 0, 0);
		return;
    }
	switch(status)
	{
	case HTTP_STATE_GET_BODY:
		if (data)
		{
            LUAT_DEBUG_PRINT("http_down_data len%d\r\n",len);
			http_down_data = malloc(len);
			memcpy(http_down_data, data, len);
			luat_rtos_event_send(param, LUAT_HTTP_GET_DATA, http_down_data, len, 0, 0);
		}
        else
		{
			luat_rtos_event_send(param, LUAT_HTTP_GET_DATA_DONE, 0, 0, 0, 0);
		}
		break;
	case HTTP_STATE_GET_HEAD:
		if (data)
		{
			LUAT_DEBUG_PRINT("%s", data);
		}
        else
		{
			luat_rtos_event_send(param, LUAT_HTTP_GET_HEAD_DONE, 0, 0, 0, 0);
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

static void luatos_http_task(void *param)
{
	luat_event_t event;
	int done_len = 0;
	while (luat_mobile_get_register_status()!=1)
	{
		luat_rtos_task_sleep(500);
		LUAT_DEBUG_PRINT("Waiting for online registration");
	}
	LUAT_DEBUG_PRINT("Online registration successful");
	luat_http_ctrl_t *http = luat_http_client_create(luatos_http_cb, luat_rtos_get_current_handle(), -1);
	if(IS_used_HTTP==1)
	{
		//SSL control syntax
		luat_http_client_ssl_config(http, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	} 
    luat_http_client_start(http, http_url, 0, 0, 0);
	while (1)
	{
		luat_rtos_event_recv(luatos_http_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		switch (event.id)
		{
		case LUAT_HTTP_GET_HEAD_DONE:
			done_len = 0;
			LUAT_DEBUG_PRINT("status %d total %u", luat_http_client_get_status_code(http), http->total_len);
			break;
		case LUAT_HTTP_GET_DATA:
			if (done_len <= 0)
			{
				FILE *fp1 = luat_fs_fopen("hiluatos.txt", "wb+");
				uint32_t status = luat_fs_fwrite((uint32_t *)event.param1, event.param2, 1, fp1);
				if (0 == status)
				{
					while (1)
						;
				}
				luat_fs_fclose(fp1);
				done_len = done_len + event.param2;
			}
			else if ((done_len >= 0) && (done_len < http->total_len))
			{
				FILE *fp1 = luat_fs_fopen("hiluatos.txt", "ab+");
				uint32_t status = luat_fs_fwrite((uint32_t *)event.param1, event.param2, 1, fp1);
				if (0 == status)
				{
					while (1);
				}
				luat_fs_fclose(fp1);
				done_len = done_len + event.param2;
			}
			free(event.param1);
			break;
		case LUAT_HTTP_GET_DATA_DONE:
		    LUAT_DEBUG_PRINT("Data download completed");
			break;
		case LUAT_HTTP_FAILED:
			break;
		}
		LUAT_DEBUG_PRINT("http(s) demo \r\t");
	}	
}



static void luat_test_init(void)
{
	luat_mobile_event_register_handler(luatos_mobile_event_callback);
	net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
	luat_rtos_task_create(&luatos_http_task_handle, 4 * 1024, 50, "test", luatos_http_task, NULL, 16);

}
INIT_TASK_EXPORT(luat_test_init, "1");
