#include "luat_network_adapter.h"
#include "common_api.h"
#include "luat_debug.h"
#include "luat_rtos.h"
#include "luat_mobile.h"
#include "networkmgr.h"
#include "platform_def.h"
#include "luat_pm.h"
typedef struct luat_socket_send_data
{
	uint8_t *data;
	uint32_t data_len;
} luat_socket_send_data_t;

enum
{
	SEND_HEART_MESSAGE = 0,
	SEND_WAKEUPPAD_MESSAGE,
	RESPONSE_MESSAGE,
	SOCKET_DISCONNECT
};

static luat_rtos_timer_t g_s_heart_timer_handle;

static luat_rtos_timer_t g_s_reconnect_timer_handle;
static luat_rtos_task_handle g_s_task_handle;
static luat_rtos_task_handle g_s_send_task_handle;
static luat_rtos_task_handle g_s_recv_task_handle;
static network_ctrl_t *g_s_network_ctrl;
static luat_rtos_semaphore_t g_s_wait_send_ok_semaphore_handle;
static luat_rtos_semaphore_t g_s_wait_network_link_semaphore_handle;
static luat_rtos_semaphore_t g_s_wait_connect_semaphore_handle;
static uint8_t g_s_socket_is_connected = 0;
static ip_addr_t g_s_server_ip;
static int  heartbeat_interval = 5*60*1000;  //Heartbeat interval
// Please visit https://netlab.luatos.com to get the new port number
const char remote_ip[] = "112.125.89.8";
int port = 44561;

static luat_rtos_timer_callback_t total_timer_callback(void *param)
{
	if (&g_s_reconnect_timer_handle == (luat_rtos_timer_t *)param)
	{
		network_connect(g_s_network_ctrl, remote_ip, sizeof(remote_ip) - 1, NULL, port, 30000);
	}
	else if (&g_s_heart_timer_handle == (luat_rtos_timer_t *)param)
	{
		luat_rtos_message_send(g_s_send_task_handle, SEND_HEART_MESSAGE, NULL);
	}
}

static void luat_socket_close(void)
{
	g_s_socket_is_connected = 0;
	luat_rtos_message_send(g_s_send_task_handle, SOCKET_DISCONNECT, NULL);
	network_close(g_s_network_ctrl, 3000);
	luat_rtos_timer_start(g_s_reconnect_timer_handle, 30000, 0, total_timer_callback, &g_s_reconnect_timer_handle);
}

static int32_t luat_test_socket_callback(void *pdata, void *param)
{
	OS_EVENT *event = (OS_EVENT *)pdata;
	LUAT_DEBUG_PRINT("socket event id %x", event->ID);
	if (event->ID == EV_NW_RESULT_LINK)
	{
		luat_rtos_semaphore_release(g_s_wait_network_link_semaphore_handle);
		return 0; // No need for wait_event here, return directly
	}
	else if (event->ID == EV_NW_RESULT_CONNECT)
	{
		g_s_socket_is_connected = 1;
		luat_rtos_semaphore_release(g_s_wait_connect_semaphore_handle);
		luat_rtos_timer_start(g_s_heart_timer_handle, heartbeat_interval, 1, total_timer_callback, &g_s_heart_timer_handle);
	}
	else if (event->ID == EV_NW_RESULT_CLOSE)
	{
		g_s_socket_is_connected = 0;
	}
	else if (event->ID == EV_NW_RESULT_TX)
	{
		luat_rtos_semaphore_release(g_s_wait_send_ok_semaphore_handle);
	}
	else if (event->ID == EV_NW_RESULT_EVENT)
	{
		if (event->Param1 == 0)
		{
			uint32_t id = 0;
			luat_rtos_message_send(g_s_recv_task_handle, id, NULL);
		}
		return 0; // No need for wait_event here, return directly
	}
	if (event->Param1)
	{

		LUAT_DEBUG_PRINT("closing socket", event->Param1);
		luat_socket_close();
	}
	int ret = network_wait_event(g_s_network_ctrl, NULL, 0, NULL);
	if (ret < 0)
	{
		luat_socket_close();
		return -1;
	}
	return 0;
}

static luat_pm_wakeup_pad_isr_callback_t wakeup_pad_callback(int num)
{
	if (LUAT_PM_WAKEUP_PAD_0 == num)
	{
		luat_rtos_message_send(g_s_send_task_handle, SEND_WAKEUPPAD_MESSAGE, NULL);
	}
}

static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	if (LUAT_MOBILE_EVENT_NETIF == event)
	{
		if (LUAT_MOBILE_NETIF_LINK_ON == status)
		{
			ip_addr_t dns_ip[2];
			uint8_t type, dns_num;
			dns_num = 2;
			soc_mobile_get_default_pdp_part_info(&type, NULL, NULL, &dns_num, dns_ip);

			if (type & 0x80)
			{
				if (index != 4)
				{
					return;
				}
				else
				{
					NmAtiNetifInfo *pNetifInfo = malloc(sizeof(NmAtiNetifInfo));
					NetMgrGetNetInfo(0xff, pNetifInfo);
					if (pNetifInfo->ipv6Cid != 0xff)
					{
						net_lwip_set_local_ip6(&pNetifInfo->ipv6Info.ipv6Addr);
						g_s_server_ip.u_addr.ip6 = pNetifInfo->ipv6Info.ipv6Addr;
						g_s_server_ip.type = IPADDR_TYPE_V6;
						LUAT_DEBUG_PRINT("%s", ipaddr_ntoa(&g_s_server_ip));
					}
					free(pNetifInfo);
				}
			}
			if (dns_num > 0)
			{
				network_set_dns_server(NW_ADAPTER_INDEX_LWIP_GPRS, 2, &dns_ip[0]);
				if (dns_num > 1)
				{
					network_set_dns_server(NW_ADAPTER_INDEX_LWIP_GPRS, 3, &dns_ip[1]);
				}
			}
			net_lwip_set_link_state(NW_ADAPTER_INDEX_LWIP_GPRS, 1);
		}
	}
}

static void luat_recv_data_task(void *param)
{
	uint32_t id;
	void *temp;
	int result;
	uint8_t *rx_data = malloc(8 * 1024);
	uint32_t rx_len;
	while (1)
	{
		if (0 == luat_rtos_message_recv(g_s_recv_task_handle, &id, &temp, LUAT_WAIT_FOREVER))
		{
			do
			{
				result = network_rx(g_s_network_ctrl, rx_data, 1024 * 8, 0, NULL, NULL, &rx_len);
				if (rx_len > 0)
				{
					LUAT_DEBUG_PRINT("rx %d", rx_len);
					luat_rtos_message_send(g_s_send_task_handle, RESPONSE_MESSAGE, NULL);
				}
			} while (!result && rx_len > 0);
		}
	}
}

static void luat_send_data_task(void *param)
{
	uint32_t id;
	void *temp;
	void *data = NULL;
	uint8_t *tx_buf = malloc(512);
	uint32_t tx_len, data_len;
	while (1)
	{
		if (g_s_socket_is_connected)
		{
			if (0 == luat_rtos_message_recv(g_s_send_task_handle, &id, &data, LUAT_WAIT_FOREVER))
			{
				switch (id)
				{
				case SEND_HEART_MESSAGE:
					data_len = snprintf(tx_buf, 512, "%s", "heart");
					break;
				case SEND_WAKEUPPAD_MESSAGE:
					data_len = snprintf(tx_buf, 512, "%s", "wakeup message");
					break;
				case RESPONSE_MESSAGE:
					data_len = snprintf(tx_buf, 512, "%s", "recv ok");
					break;
				case SOCKET_DISCONNECT:
					continue;
					break;
				default:
					continue;
					break;
				}
				if (!g_s_socket_is_connected)
				{
					continue;
				}
				int result = network_tx(g_s_network_ctrl, tx_buf, data_len, 0, NULL, 0, &tx_len, 0);
				if (result < 0)
				{
					luat_socket_close();
					continue;
				}
				if (0 == luat_rtos_semaphore_take(g_s_wait_send_ok_semaphore_handle, 15000))
				{
					LUAT_DEBUG_PRINT("send ok");
				}
				else
				{
					luat_socket_close();
					LUAT_DEBUG_PRINT("send fail");
					continue;
				}
			}
		}
		else
		{
			luat_rtos_semaphore_take(g_s_wait_connect_semaphore_handle, LUAT_WAIT_FOREVER);
		}
	}
}

static void luat_test_task(void *param)
{
	luat_pm_wakeup_pad_set_callback(wakeup_pad_callback);

	//Initialize wakeup0, pull-up by default, interrupt on falling edge
	luat_pm_wakeup_pad_cfg_t cfg = {0};
	cfg.pull_up_enable = 1;
	cfg.neg_edge_enable = 1;
	cfg.pos_edge_enable = 0;
	cfg.pull_down_enable = 0;
	luat_pm_wakeup_pad_set(true, LUAT_PM_WAKEUP_PAD_0, &cfg);

	/*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! ! 1*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET); 
	//Response first
	luat_pm_set_power_mode(LUAT_PM_POWER_MODE_HIGH_PERFORMANCE, 0);
	//Apply for socket resources
	g_s_network_ctrl = network_alloc_ctrl(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_init_ctrl(g_s_network_ctrl, NULL, luat_test_socket_callback, NULL);
	network_set_base_mode(g_s_network_ctrl, 1, 15000, 0, 300, 5, 9);
	g_s_network_ctrl->is_debug = 0;

	network_wait_link_up(g_s_network_ctrl, 0);

	luat_rtos_semaphore_take(g_s_wait_network_link_semaphore_handle, 60000);

	//Connect to server
	network_connect(g_s_network_ctrl, remote_ip, sizeof(remote_ip) - 1, NULL, port, 30000);
	luat_rtos_task_delete(g_s_task_handle);
}

static void luat_test_init(void)
{
	luat_mobile_event_register_handler(luatos_mobile_event_callback);
	net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
	//Create resources and create tasks
	luat_rtos_task_create(&g_s_task_handle, 2 * 1024, 50, "test", luat_test_task, NULL, 16);
	//Create receiving task
	luat_rtos_task_create(&g_s_recv_task_handle, 2 * 1024, 60, "test recv", luat_recv_data_task, NULL, 16);
	//Create sending task
	luat_rtos_task_create(&g_s_send_task_handle, 2 * 1024, 60, "test send", luat_send_data_task, NULL, 10);
	//Create software heartbeat sending timer
	luat_rtos_timer_create(&g_s_heart_timer_handle);
	//Create a disconnection reconnection timer
	luat_rtos_timer_create(&g_s_reconnect_timer_handle);
	//Create a message completion semaphore
	luat_rtos_semaphore_create(&g_s_wait_send_ok_semaphore_handle, 1);
	//Create a semaphore waiting for network connection
	luat_rtos_semaphore_create(&g_s_wait_network_link_semaphore_handle, 1);
	//Create a semaphore to wait for successful socket connection
	luat_rtos_semaphore_create(&g_s_wait_connect_semaphore_handle, 1);
}

INIT_TASK_EXPORT(luat_test_init, "1");
