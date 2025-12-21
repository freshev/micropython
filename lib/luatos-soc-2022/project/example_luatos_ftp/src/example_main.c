#include "common_api.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_mobile.h"
#include "luat_network_adapter.h"
#include "networkmgr.h"
#include "luat_ftp.h"

#define FTP_OTA_TEST 1

static luat_rtos_task_handle luatos_http_task_handle;

static uint8_t MOBILE_LINK = 0;

static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	if (LUAT_MOBILE_EVENT_NETIF == event)
	{
		if (LUAT_MOBILE_NETIF_LINK_ON == status)
		{
			luat_socket_check_ready(index, NULL);
			MOBILE_LINK = 1;
		}
	}
}

static void luat_ftp_cb(luat_ftp_ctrl_t *luat_ftp_ctrl, FTP_SUCCESS_STATE_e event){
	LUAT_DEBUG_PRINT("%d", event);
	char *data = NULL;
	if(event == FTP_SUCCESS_DATA)
	{
		data = malloc(luat_ftp_ctrl->result_buffer.Pos + 1);
		memcpy(data, luat_ftp_ctrl->result_buffer.Data, luat_ftp_ctrl->result_buffer.Pos + 1);
	}
	luat_rtos_event_send(luatos_http_task_handle, event, data, NULL, NULL, 0);
}


static uint8_t ftpclient_recv_response(luat_rtos_task_handle handle, luat_event_t event)
{
	luat_rtos_event_recv(luatos_http_task_handle, 0, &event, NULL, 0);
	if(FTP_SUCCESS_DATA == event.id)
	{
		free(event.param1);
	}
	LUAT_DEBUG_PRINT("event id: %d", event.id);
	return event.id;
}

static void luatos_http_task(void *param)
{
	while (MOBILE_LINK!=1){
		luat_rtos_task_sleep(500);
		LUAT_DEBUG_PRINT("Waiting for network registration");
	}
	luat_event_t event;
	uint8_t ftp_state = FTP_ERROR;
#if	FTP_OTA_TEST == 1
	uint8_t ota_result = 0;
#endif
	int result = luat_ftp_login(NW_ADAPTER_INDEX_LWIP_GPRS,"121.43.224.154", 21, "ftp_user", "3QujbiMG", NULL, luat_ftp_cb);
	if(result)
	{
		luat_ftp_release();
		while (1)
		{
			LUAT_DEBUG_PRINT("ftp login fail 1");
			luat_rtos_task_sleep(1000);
		}
	}
	ftp_state = ftpclient_recv_response(luatos_http_task_handle, event);
	if (FTP_ERROR == ftp_state)
	{
		while (1)
		{
			LUAT_DEBUG_PRINT("ftp login fail 2");
			luat_rtos_task_sleep(1000);
		}
	}

	if(luat_ftp_command("LIST") == 0)
	{
		ftp_state = ftpclient_recv_response(luatos_http_task_handle, event);
	}

	if(luat_ftp_pull("123123.txt", "/1234567.txt", 0) == 0)
	{
		ftp_state = ftpclient_recv_response(luatos_http_task_handle, event);
	}

#if	FTP_OTA_TEST == 1
	if(luat_ftp_pull(NULL, "/ota_test.bin", 1) == 0)
	{
		ota_result = ftpclient_recv_response(luatos_http_task_handle, event);
	}
#endif

	if(luat_ftp_close() == 0)
	{
		ftp_state = ftpclient_recv_response(luatos_http_task_handle, event);
	}

#if	FTP_OTA_TEST == 1
	if (FTP_SUCCESS_NO_DATA == ota_result)
	{
		luat_pm_reboot();
	}
#endif
	while (1) 
	{
		LUAT_DEBUG_PRINT("end");
		luat_rtos_task_sleep(5000);
	}
	// luat_ftp_command("NOOP");
	// luat_rtos_task_sleep(5000);
    // luat_ftp_command("SYST");
	// luat_rtos_task_sleep(5000);
    // luat_ftp_command("TYPE I");
	// luat_rtos_task_sleep(5000);
    // luat_ftp_command("PWD");
	// luat_rtos_task_sleep(5000);
    // luat_ftp_command("MKD QWER");
	// luat_rtos_task_sleep(5000);
    // luat_ftp_command("CWD /QWER");
	// luat_rtos_task_sleep(5000);
    // luat_ftp_command("CDUP");
	// luat_rtos_task_sleep(5000);
    // luat_ftp_command("RMD QWER");
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
