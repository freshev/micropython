/*The business logic implemented in this document is as follows:
1. Boot into IPv6
2. Report the imei, iccid, ipv6 address according to the "tardigrade" service document tardigrade service (low power wake-up service), and report it again when the network switches.
3. Enter "Response first" sleep mode by default, customers can choose "equilibrium mode"
4. After receiving the Modules data sent by the server, it is printed in luatools (customers can send and wake up through the web page)
5. After receiving the data from the server, connect to the TCP server of IPV4 and forward it to the ipv4 server. After forwarding, close TCP (use https://netlab.luatos.com/ to simulate the customer's ipv4 server)
6. You can wake up the Modules through IO and print it in luatools
7. After receiving the IO wake-up call, connect to the TCP server of IPV4 and forward it to the ipv4 server. After forwarding, close TCP*/

#include "luat_network_adapter.h"
#include "common_api.h"
#include "luat_debug.h"
#include "luat_rtos.h"
#include "luat_mobile.h"
#include "networkmgr.h"
#include "cmidev.h"
#include "luat_pm.h"

// #define  LUAT_TRANS_IPV6ADDR_TO_BYTE(ipv6, ipv6_bytes)   \
//   do{                                                     \
//         UINT16  ipv6U16 = 0;                              \
//         ipv6U16 = IP6_ADDR_BLOCK1((ipv6));   \
//         ipv6_bytes[0] = (UINT8)((ipv6U16 >> 8)&0xFF);   \
//         ipv6_bytes[1] = (UINT8)(ipv6U16 & 0xFF);        \
//         ipv6U16 = IP6_ADDR_BLOCK2((ipv6));      \
//         ipv6_bytes[2] = (UINT8)((ipv6U16 >> 8)&0xFF);   \
//         ipv6_bytes[3] = (UINT8)(ipv6U16 & 0xFF);        \
//         ipv6U16 = IP6_ADDR_BLOCK3((ipv6));      \
//         ipv6_bytes[4] = (UINT8)((ipv6U16 >> 8)&0xFF);   \
//         ipv6_bytes[5] = (UINT8)(ipv6U16 & 0xFF);         \
//         ipv6U16 = IP6_ADDR_BLOCK4((ipv6));      \
//         ipv6_bytes[6] = (UINT8)((ipv6U16 >> 8)&0xFF);  \
//         ipv6_bytes[7] = (UINT8)(ipv6U16 & 0xFF);   \
//         ipv6U16 = IP6_ADDR_BLOCK5((ipv6));   \
//         ipv6_bytes[8] = (UINT8)((ipv6U16 >> 8)&0xFF);  \
//         ipv6_bytes[9] = (UINT8)(ipv6U16 & 0xFF);   \
//         ipv6U16 = IP6_ADDR_BLOCK6((ipv6));  \
//         ipv6_bytes[10] = (UINT8)((ipv6U16 >> 8)&0xFF);   \
//         ipv6_bytes[11] = (UINT8)(ipv6U16 & 0xFF);    \
//         ipv6U16 = IP6_ADDR_BLOCK7((ipv6));   \
//         ipv6_bytes[12] = (UINT8)((ipv6U16 >> 8)&0xFF);  \
//         ipv6_bytes[13] = (UINT8)(ipv6U16 & 0xFF);    \
//         ipv6U16 = IP6_ADDR_BLOCK8((ipv6));   \
//         ipv6_bytes[14] = (UINT8)((ipv6U16 >> 8)&0xFF);  \
//         ipv6_bytes[15] = (UINT8)(ipv6U16 & 0xFF);  \
//     }while(0)



typedef struct app_client_send_data
{
    char *data;
    uint32_t len;
}app_client_send_data_t;


static int test = 0;

static luat_rtos_task_handle g_s_client_task_handle;
static char* g_s_remote_server_addr = "114.55.242.59";
static int g_s_remote_server_port = 45000;
// static char* g_s_remote_server_addr = "112.125.89.8";
// static int g_s_remote_server_port = 36794;
static int g_s_is_ipv6_ready = 0;
static luat_rtos_semaphore_t g_s_ipv6_ready_semaphore = NULL;
static luat_rtos_semaphore_t g_s_client_wakeup_semaphore = NULL;
static network_ctrl_t *g_s_network_ctrl;


static luat_rtos_task_handle g_s_server_task_handle;
static network_ctrl_t *g_s_server_network_ctrl;
static ip_addr_t g_s_server_ip;
static uint16_t server_port = 45000;
#define SERVER_RX_BUF_SIZE (1024)
static uint8_t *g_s_server_rx_data = NULL;

// Customized business client and server parameters
static luat_rtos_task_handle g_s_app_client_task_handle;
static char* g_s_app_remote_server_addr = "112.125.89.8";
static int g_s_app_remote_server_port = 36078;
static network_ctrl_t *g_s_app_network_ctrl;

//Button wakeup mark
static uint8_t g_s_pad_wakeup_flag = 0;
static luat_rtos_task_handle g_s_key_wakeup_task_handle;
static luat_rtos_semaphore_t g_s_key_wakeup_semaphore = NULL;

static int32_t luat_test_socket_callback(void *pdata, void *param)
{
	OS_EVENT *event = (OS_EVENT *)pdata;
	LUAT_DEBUG_PRINT("%x", event->ID);
	return 0;
}

static int32_t luat_server_test_socket_callback(void *pdata, void *param)
{
	OS_EVENT *event = (OS_EVENT *)pdata;
	LUAT_DEBUG_PRINT("%x", event->ID);
	return 0;
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
			LUAT_DEBUG_PRINT("type %x", type);

			g_s_is_ipv6_ready = 0;

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

						// If the ipv6 address changes, notify the client task to report the new ip address to the business server
						if (memcmp(&(g_s_server_ip.u_addr.ip6), &(pNetifInfo->ipv6Info.ipv6Addr), sizeof(pNetifInfo->ipv6Info.ipv6Addr)) != 0)
						{
							luat_rtos_semaphore_release(g_s_client_wakeup_semaphore);
						}
						
						g_s_server_ip.u_addr.ip6 = pNetifInfo->ipv6Info.ipv6Addr;
						g_s_server_ip.type = IPADDR_TYPE_V6;
						LUAT_DEBUG_PRINT("%s", ipaddr_ntoa(&g_s_server_ip));

						g_s_is_ipv6_ready = 1;
						luat_rtos_semaphore_release(g_s_ipv6_ready_semaphore);
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
		else if(LUAT_MOBILE_NETIF_LINK_OFF == status)
		{
			g_s_is_ipv6_ready = 0;
		}
	}
}

int app_client_send_data(const char *data, uint32_t len)
{
	if(data==NULL || len==0)
	{
		return -1;
	}
	app_client_send_data_t *data_item = (app_client_send_data_t *)malloc(sizeof(app_client_send_data_t));
	LUAT_DEBUG_ASSERT(data_item != NULL,"malloc data_item fail");;

	data_item->data = malloc(len);
	LUAT_DEBUG_ASSERT(data_item->data != NULL, "malloc data_item.data fail");
	memcpy(data_item->data, data, len);
	data_item->len = len;

	int ret = luat_rtos_message_send(g_s_app_client_task_handle, 0, data_item);

	if(ret != 0)
	{
		free(data_item->data);
		data_item->data = NULL;
		free(data_item);
		data_item = NULL;
	}

	return ret;
}


static void luat_tcp_server_task(void *param)
{
	g_s_server_network_ctrl = network_alloc_ctrl(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_init_ctrl(g_s_server_network_ctrl, g_s_server_task_handle, luat_server_test_socket_callback, NULL);
	network_set_base_mode(g_s_server_network_ctrl, 1, 15000, 1, 600, 5, 9);
	network_set_local_port(g_s_server_network_ctrl, server_port);
	g_s_server_network_ctrl->is_debug = 1;

	if (!g_s_server_rx_data)
	{
		g_s_server_rx_data = malloc(SERVER_RX_BUF_SIZE);
	}
	uint32_t tx_len, rx_len;
	int result;
	uint8_t is_break,is_timeout;

	while(1)
	{
		result = network_listen(g_s_server_network_ctrl, 0xffffffff);
		if (!result)
		{
			network_socket_accept(g_s_server_network_ctrl, NULL);
			LUAT_DEBUG_PRINT("client %s, %u", ipaddr_ntoa(&g_s_server_network_ctrl->remote_ip), g_s_server_network_ctrl->remote_port);
			while(!result)
			{
				result = network_wait_rx(g_s_server_network_ctrl, 30000, &is_break, &is_timeout);
				if (!result)
				{
					if (!is_timeout && !is_break)
					{
						do
						{
							result = network_rx(g_s_server_network_ctrl, g_s_server_rx_data, SERVER_RX_BUF_SIZE, 0, NULL, NULL, &rx_len);
							if (rx_len > 0)
							{
								uint8_t *rx_hex_data = malloc(rx_len*2+1);
								memset(rx_hex_data, 0, rx_len*2+1);
								for (size_t i = 0; i < rx_len; i++)
								{
									sprintf(rx_hex_data+i*2, "%02X", g_s_server_rx_data[i]);
								}
								LUAT_DEBUG_PRINT("rx %d: %s", rx_len, rx_hex_data);
								free(rx_hex_data);


								// prefix + end package
								if ((rx_len >= 12) && (memcmp(g_s_server_rx_data, "\x4C\x50\x00\x01", 4)==0) && (memcmp(g_s_server_rx_data+rx_len-4, "\x00\x00\x00\x00", 4)==0))
								{
									//The parameter len field value being parsed
									uint16_t param_len = 0;
									//The index position of the data being parsed in the entire message, adjusted past the 4-byte prefix, starting from the 5th byte
									uint16_t rx_index = 4;

									while (1)
									{
										// Preliminarily determine whether the remaining data can completely parse a parameter based on the length (a parameter requires a minimum of 4 bytes, id+len field)
										if (rx_index+8 > rx_len)
										{
											break;
										}

										param_len = *(g_s_server_rx_data+rx_index+2)*256 + *(g_s_server_rx_data+rx_index+3);
										if (rx_index+8+param_len > rx_len)
										{
											break;
										}


										if (memcmp(g_s_server_rx_data+rx_index, "\x00\x83", 2)==0)
										{
											LUIAT_DEBUG_PRINT("server wakeup");
											luat_rtos_semaphore_release(g_s_client_wakeup_semaphore);
										}
										else if (memcmp(g_s_server_rx_data+rx_index, "\x02\x01", 2)==0)
										{
											
										}
										else
										{
											LUAT_DEBUG_PRINT("unsupport cmd");										
										}

										rx_index += 4+param_len;
									}
									

								}
								else
								{
									LUAT_DEBUG_PRINT("invalid head or tail");
								}

								
								// Receive the correct wake-up message
								// if ((rx_len >= sizeof(wakeup_cmd)) && (memcmp(g_s_server_rx_data, "\x4C\x50\x00\x01", 4)==0) && (memcmp(g_s_server_rx_data+rx_len-4, "\x00\x00\x00\x00", 4)==0))
								// {
								// 	luat_rtos_semaphore_release(g_s_client_wakeup_semaphore);
								// }
							}
						}while(!result && rx_len > 0);

					}
				}
			}
		}
		LUAT_DEBUG_PRINT("Network disconnected, try again");
		network_close(g_s_server_network_ctrl, 5000);
	}
}

static void luat_udp_server_task(void *param)
{
	g_s_server_network_ctrl = network_alloc_ctrl(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_init_ctrl(g_s_server_network_ctrl, g_s_server_task_handle, luat_server_test_socket_callback, NULL);
	network_set_base_mode(g_s_server_network_ctrl, 0, 15000, 1, 600, 5, 9);
	network_set_local_port(g_s_server_network_ctrl, server_port);
	g_s_server_network_ctrl->is_debug = 1;

	if (!g_s_server_rx_data)
	{
		g_s_server_rx_data = malloc(SERVER_RX_BUF_SIZE);
	}
	
	uint32_t tx_len, rx_len;
	ip_addr_t remote_ip;
	uint16_t remote_port;
	int result;
	uint8_t is_break,is_timeout;

	while(1)
	{
		remote_ip = ip6_addr_any;
		result = network_connect(g_s_server_network_ctrl, NULL, 0, &remote_ip, 0, 30000);
		if (!result)
		{
			while(!result)
			{
				result = network_wait_rx(g_s_server_network_ctrl, 30000, &is_break, &is_timeout);
				if (!result)
				{
					if (!is_timeout && !is_break)
					{
						do
						{
							result = network_rx(g_s_server_network_ctrl, g_s_server_rx_data, SERVER_RX_BUF_SIZE, 0, &remote_ip, &remote_port, &rx_len);
							if (rx_len > 0)
							{
								
								uint8_t *rx_hex_data = malloc(rx_len*2+1);
								memset(rx_hex_data, 0, rx_len*2+1);
								for (size_t i = 0; i < rx_len; i++)
								{
									sprintf(rx_hex_data+i*2, "%02X", g_s_server_rx_data[i]);
								}
								LUAT_DEBUG_PRINT("rx %d: %s", rx_len, rx_hex_data);
								free(rx_hex_data);


								// prefix + end package
								if ((rx_len >= 12) && (memcmp(g_s_server_rx_data, "\x4C\x50\x00\x01", 4)==0) && (memcmp(g_s_server_rx_data+rx_len-4, "\x00\x00\x00\x00", 4)==0))
								{
									//The parameter len field value being parsed
									uint16_t param_len = 0;
									//The index position of the data being parsed in the entire message, adjusted past the 4-byte prefix, starting from the 5th byte
									uint16_t rx_index = 4;

									while (1)
									{
										// Preliminarily determine whether the remaining data can completely parse a parameter based on the length (a parameter requires a minimum of 4 bytes, id+len field)
										if (rx_index+8 > rx_len)
										{
											break;
										}

										param_len = *(g_s_server_rx_data+rx_index+2)*256 + *(g_s_server_rx_data+rx_index+3);
										if (rx_index+8+param_len > rx_len)
										{
											break;
										}


										// Wake-up packet
										if (memcmp(g_s_server_rx_data+rx_index, "\x00\x83", 2)==0)
										{
											luat_rtos_semaphore_release(g_s_client_wakeup_semaphore);
											// app_client_send_data(g_s_server_rx_data+rx_index+4, 4);
										}
										// Custom data package
										else if (memcmp(g_s_server_rx_data+rx_index, "\x02\x01", 2)==0)
										{
											app_client_send_data(g_s_server_rx_data+rx_index+4, param_len);
										}
										else
										{
											LUAT_DEBUG_PRINT("unsupport cmd");
										}

										rx_index += 4+param_len;
									}
									

								}
								else
								{
									LUAT_DEBUG_PRINT("invalid head or tail");
								}

							}
						}while(!result && rx_len > 0);

					}
				}
			}
		}
		LUAT_DEBUG_PRINT("Network disconnected, try again");
		network_close(g_s_server_network_ctrl, 5000);
	}
}


static void luat_app_client_task(void *param)
{
	// Apply for and configure a network adapter
	g_s_app_network_ctrl = network_alloc_ctrl(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_init_ctrl(g_s_app_network_ctrl, g_s_app_client_task_handle, NULL, NULL);
	network_set_base_mode(g_s_app_network_ctrl, 1, 15000, 1, 600, 5, 9);
	g_s_app_network_ctrl->is_debug = 1;


	uint8_t rx_data[50] = {0};
	uint32_t  tx_len, rx_len, cnt;
	int result;
	uint8_t is_break,is_timeout;
	cnt = 0;

	while (1)
	{
		uint32_t message_id;
		app_client_send_data_t *data_item;

		if(luat_rtos_message_recv(g_s_app_client_task_handle, &message_id, (void **)&data_item, LUAT_WAIT_FOREVER) == 0)
		{
			// Block + timeout 60 seconds to wait for the network environment to be ready, 0 indicates success
			while (network_wait_link_up(g_s_app_network_ctrl, 60000) != 0)
			{
				LUAT_DEBUG_PRINT("wait network link up");
			}
			
			while (1)
			{
				if (0 != network_connect(g_s_app_network_ctrl, g_s_app_remote_server_addr, strlen(g_s_app_remote_server_addr), NULL, g_s_app_remote_server_port, 30000))
				{
					LUAT_DEBUG_PRINT("connect fail, retry after 5 seconds");
					network_close(g_s_app_network_ctrl, 5000);
					luat_rtos_task_sleep(5000);
					continue;
				}

				if (0 != network_tx(g_s_app_network_ctrl, data_item->data, data_item->len, 0, NULL, 0, &tx_len, 15000))
				{
					LUAT_DEBUG_PRINT("send fail, retry after 5 seconds");
					network_close(g_s_network_ctrl, 5000);
					luat_rtos_task_sleep(5000);
					continue;
				}				

				network_close(g_s_app_network_ctrl, 5000);
				break;
			}
			

			free(data_item->data);
			data_item->data = NULL;
			free(data_item);
			data_item = NULL;
		}
	}
	

	luat_rtos_task_delete(g_s_app_client_task_handle);
}

static void luat_key_wakeup_task(void *param)
{
	while (1)
	{
		if (g_s_pad_wakeup_flag)
		{
			g_s_pad_wakeup_flag = 0;
			app_client_send_data("wakeup from wakeup0", strlen("wakeup from wakeup0"));
		}
		else
		{
			luat_rtos_semaphore_take(g_s_key_wakeup_semaphore, LUAT_WAIT_FOREVER);
		}
	}
	
}

//wakepad callback function
static luat_pm_wakeup_pad_isr_callback_t pad_wakeup_callback(LUAT_PM_WAKEUP_PAD_E num)
{
    LUAT_DEBUG_PRINT("pm demo wakeup pad num %d", num);
	if (LUAT_PM_WAKEUP_PAD_0 == num)
	{
		g_s_pad_wakeup_flag = 1;
		luat_rtos_semaphore_release(g_s_key_wakeup_semaphore);
	}	
}



static void luat_client_task(void *param)
{
	/*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! !*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET); 


	if (NULL == g_s_ipv6_ready_semaphore)
	{
		luat_rtos_semaphore_create(&g_s_ipv6_ready_semaphore, 1);
	}
	if (NULL == g_s_client_wakeup_semaphore)
	{
		luat_rtos_semaphore_create(&g_s_client_wakeup_semaphore, 1);
	}
	

	// Apply for and configure a network adapter
	g_s_network_ctrl = network_alloc_ctrl(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_init_ctrl(g_s_network_ctrl, g_s_client_task_handle, luat_test_socket_callback, NULL);
	network_set_base_mode(g_s_network_ctrl, 0, 15000, 0, 0, 0, 0);
	g_s_network_ctrl->is_debug = 1;


	uint8_t rx_data[50] = {0};
	uint32_t  tx_len, rx_len, cnt;
	int result;
	uint8_t is_break,is_timeout;
	cnt = 0;

	while(1)
	{
		while(1)
		{
			//Print ram information
			uint32_t all,now_free_block,min_free_block;
			luat_meminfo_sys(&all, &now_free_block, &min_free_block);
			LUAT_DEBUG_PRINT("meminfo %d,%d,%d",all,now_free_block,min_free_block);

			// Block + timeout 60 seconds to wait for the network environment to be ready, 0 indicates success
			result = network_wait_link_up(g_s_network_ctrl, 60000);
			if (result)
			{
				continue;
			}

			//Whether the sleep mode has been set during this boot operation?
			// After the network is ready, the interface can be called to set the sleep mode;
			// Some sleep parameters need to be set in airplane mode;
			// Use this flag variable to ensure it is only set once;
			static uint8_t has_set_power_mode = 0;
			if (!has_set_power_mode)
			{
				has_set_power_mode = 1;
				
				// Set to response priority sleep mode
				luat_pm_set_power_mode(LUAT_PM_POWER_MODE_HIGH_PERFORMANCE, 0);
			}
			

			// Wait for ipv6 to be ready
			if (!g_s_is_ipv6_ready)
			{
				LUAT_DEBUG_PRINT("wait ipv6 ready semaphore");
				luat_rtos_semaphore_take(g_s_ipv6_ready_semaphore, LUAT_WAIT_FOREVER);
				continue;
			}

			//Create an ipv6 server task and start an ipv6 server to accept connections from ipv6 clients and send wake-up messages or other customized messages.

			// It should be noted that you must use a SIM card with IPv6 service enabled to use IPv6 service;
			// If you use Luatools version 2.2.2 or greater, check Clear kv partition and Clear file system partition, directly burn and run this program, you can print out log information similar to the following:
			// [2023-06-06 18:32:22.745][000000005.052] luatos_mobile_event_callback 63:240E:46C:8920:1225:1766:B4E:9D8D:DDBD
			//The last 8 groups of hexadecimal strings separated by : are ipv6 addresses, which means that the SIM card used in the test supports ipv6 service;

			// If there is no following log information, follow the following steps to troubleshoot:
			// 1. Mobile phone cards generally support ipv6. You can visit http://www.test-ipv6.com/ in the mobile browser. If the ipv6 address can be displayed, it means it is supported. Otherwise, call the operator's customer service number to activate;
			// After confirming that the mobile phone card is supported, you can use the mobile phone card to test;
			// 2. Older IoT cards may not support ipv6, because IoT cards generally have an organic card binding function, so you cannot insert the IoT card into a mobile phone to verify whether it is supported according to the first situation;
			// You can consult the card dealer directly
			//    or
			// The device is plugged into the computer via USB, and the computer accesses the Internet through the device's rndis. The computer browser accesses http://www.test-ipv6.com/. If the ipv6 address can be displayed, it means it is supported. Otherwise, consult the card dealer to activate it;
			// After confirming that the IoT card is supported, you can use the IoT card to test;
			if (!g_s_server_task_handle)
			{
				// luat_rtos_task_create(&g_s_server_task_handle, 4 * 1024, 10, "server", luat_tcp_server_task, NULL, 16); // ipv6 tcp server
				luat_rtos_task_create(&g_s_server_task_handle, 4 * 1024, 10, "server", luat_udp_server_task, NULL, 16); // ipv6 udp server
			}

			//Create a custom ipv4 client that can connect to the custom ipv4 server and send login messages, wake-up messages, custom data sent by the ipv6 client and other messages to the ipv4 server.
			// In specific projects, this client needs to be modified and adjusted according to the customer's business logic.
			if (!g_s_app_client_task_handle)
			{
				luat_rtos_task_create(&g_s_app_client_task_handle, 2 * 1024, 90, "app_client", luat_app_client_task, NULL, 16);	
			}

			

			if (NULL == g_s_key_wakeup_semaphore)
			{
				luat_rtos_semaphore_create(&g_s_key_wakeup_semaphore, 1);
			}
			//Create key wake-up processing task
			if (!g_s_key_wakeup_task_handle)
			{
				luat_rtos_task_create(&g_s_key_wakeup_task_handle, 2 * 1024, 90, "key_wakeup", luat_key_wakeup_task, NULL, NULL);	
			}
			//Set wakeup0 interrupt callback function
			// Configure to trigger interrupt on falling edge
			// When testing with a low-power development board, shorting the wakeup0 pin gnd can trigger an interrupt.
			luat_pm_wakeup_pad_set_callback(pad_wakeup_callback);
			luat_pm_wakeup_pad_cfg_t cfg = {0};
			cfg.neg_edge_enable = 1;
			cfg.pos_edge_enable = 0;
			cfg.pull_down_enable = 0;
			cfg.pull_up_enable = 1;
			luat_pm_wakeup_pad_set(1, LUAT_PM_WAKEUP_PAD_0, &cfg);
				

			

			//Connect to server
			result = network_connect(g_s_network_ctrl, g_s_remote_server_addr, strlen(g_s_remote_server_addr), NULL, g_s_remote_server_port, 30000);
			if (!result)
			{
				uint8_t register_packet_data[120] = {0};
				uint32_t register_packet_len = 0;

				
				// Fixed prefix + registration package type
				sprintf(register_packet_data, "LP%c%c%c%c%c%c", 0x00, 0x01, 0x00, 0x01, 0x00, 0x00);
				register_packet_len += 8;

				//IMEI
				char imei[20] = {0};
				int imei_len = luat_mobile_get_imei(0, imei, sizeof(imei));
				if (imei_len > 0)
				{
					imei_len = strlen(imei);
				}
				
				snprintf(register_packet_data+register_packet_len, sizeof(register_packet_data)-register_packet_len, "%c%c%c%c%s", 0x01, 0x01, 0x00, (char)imei_len, imei);
				register_packet_len = register_packet_len+4+imei_len;

				//ICCID
				int sim_id = 0;
				luat_mobile_get_sim_id(&sim_id);
				char iccid[24] = {0};
				int iccid_len = luat_mobile_get_iccid(sim_id, iccid, sizeof(iccid));	
				if (iccid_len > 0)
				{
					iccid_len = strlen(iccid);
				}	
				snprintf(register_packet_data+register_packet_len, sizeof(register_packet_data)-register_packet_len, "%c%c%c%c%s", 0x01, 0x02, 0x00, (char)iccid_len, iccid);
				register_packet_len = register_packet_len+4+iccid_len;

				//IPV6
				// uint8_t ipv6_bytes[16] = {0};
				// int ipv6_len = 0;

				// NmAtiNetifInfo *pNetifInfo = malloc(sizeof(NmAtiNetifInfo));
				// NetMgrGetNetInfo(0xff, pNetifInfo);
				// if (pNetifInfo->ipv6Cid != 0xff)
				// {
				// 	net_lwip_set_local_ip6(&pNetifInfo->ipv6Info.ipv6Addr);
				// 	LUAT_TRANS_IPV6ADDR_TO_BYTE(&(pNetifInfo->ipv6Info.ipv6Addr), ipv6_bytes);
				// 	ipv6_len = 16;
				// }
				// free(pNetifInfo);

				ip_addr_t server_ip;
				char server_ip_str[45] = {0};
				NmAtiNetifInfo *pNetifInfo = malloc(sizeof(NmAtiNetifInfo));
				NetMgrGetNetInfo(0xff, pNetifInfo);
				if (pNetifInfo->ipv6Cid != 0xff)
				{
					server_ip.u_addr.ip6 = pNetifInfo->ipv6Info.ipv6Addr;
					server_ip.type = IPADDR_TYPE_V6;
					snprintf(server_ip_str, sizeof(server_ip_str)-1, "%s", ipaddr_ntoa(&server_ip));
				}
				free(pNetifInfo);



		
				snprintf(register_packet_data+register_packet_len, sizeof(register_packet_data)-register_packet_len, "%c%c%c%c%s", 
					0x01, 0x03, 0x00, (char)strlen(server_ip_str), server_ip_str);
				register_packet_len = register_packet_len+4+strlen(server_ip_str);
				// if (ipv6_len > 0)
				// {
				// 	snprintf(register_packet_data+register_packet_len, sizeof(register_packet_data)-register_packet_len, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 
				// 		ipv6_bytes[0],ipv6_bytes[1],ipv6_bytes[2],ipv6_bytes[3],
				// 		ipv6_bytes[4],ipv6_bytes[5],ipv6_bytes[6],ipv6_bytes[7],
				// 		ipv6_bytes[8],ipv6_bytes[9],ipv6_bytes[10],ipv6_bytes[11],
				// 		ipv6_bytes[12],ipv6_bytes[13],ipv6_bytes[14],ipv6_bytes[15]);
				// 	register_packet_len += ipv6_len;
				// }

				//Send registration message to server
				result = network_tx(g_s_network_ctrl, register_packet_data, register_packet_len, 0, NULL, 0, &tx_len, 15000);
				if (!result)
				{
					while(!result)
					{
						// Wait for the server to respond to the registration message
						result = network_wait_rx(g_s_network_ctrl, 20000, &is_break, &is_timeout);
						if (!result)
						{
							if (!is_timeout && !is_break)
							{
								cnt = 0;

								do
								{
									uint8_t register_ack[] = {'L', 'P', 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
									//Read the server's response data
									result = network_rx(g_s_network_ctrl, rx_data, sizeof(rx_data), 0, NULL, NULL, &rx_len);
									if (rx_len > 0)
									{
										LUAT_DEBUG_PRINT("rx %d", rx_len);
										// After receiving the correct response message, exit this task
										if ((rx_len == sizeof(register_ack)) && (memcmp(register_ack, rx_data, rx_len)==0))
										{
											network_close(g_s_network_ctrl, 5000);
											luat_rtos_semaphore_take(g_s_client_wakeup_semaphore, 100);
											goto CLIENT_TASK_WAIT_NEXT_WAKEUP;
										}	
									}
								}while(!result && rx_len > 0);
							}
							else if (is_timeout)
							{
								result = network_tx(g_s_network_ctrl, register_packet_data, register_packet_len, 0, NULL, 0, &tx_len, 15000);
								cnt++;
								if (cnt >= 10)
								{
									// After retrying 10 times, the reception still fails. What should I do? Restart directly?
									// luat_pm_reboot();
								}
							}
						}

					}
				}
			}
			LUAT_DEBUG_PRINT("Network disconnected, try again after 15 seconds");
			network_close(g_s_network_ctrl, 5000);
			luat_rtos_task_sleep(15000);
		}

CLIENT_TASK_WAIT_NEXT_WAKEUP:
		LUAT_DEBUG_PRINT("wait next wakeup");
		luat_rtos_semaphore_take(g_s_client_wakeup_semaphore, LUAT_WAIT_FOREVER);
	}

	luat_rtos_task_delete(g_s_client_task_handle);
}


static void flymode_task(void *arg)
{
	luat_rtos_task_sleep(5000);
	LUAT_DEBUG_PRINT("entry");
	while (1)
	{
		luat_rtos_task_sleep(60000);
		LUAT_DEBUG_PRINT("enter flymode");
		luat_mobile_set_flymode(0,1);
		luat_rtos_task_sleep(5000);
		luat_mobile_set_flymode(0,0);
		LUAT_DEBUG_PRINT("exit flymode");
	}
}


static void ipv6_sleep_wakeup_init(void)
{
	//Register network status event handler function
	luat_mobile_event_register_handler(luatos_mobile_event_callback);

	net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);

    // Enable the ipv6 function on the software; this interface needs to be called before the network environment is ready
	luat_mobile_set_default_pdn_ipv6(1);

    //Create an ipv4 client task to report imei, iccid, and ipv6 server addresses to the server;
	// At the same time, the ipv6 server task and business client task will also be created in this task main function;
	luat_rtos_task_create(&g_s_client_task_handle, 2 * 1024, 90, "client", luat_client_task, NULL, 16);
	

	// This task simulates a disconnection scenario by continuously entering and exiting airplane mode. Each time the network is disconnected and reconnected, the assigned ipv6 address will change;
	// Only used to simulate and test the function of "When the ipv6 address changes, the client task takes the initiative to report the imei, iccid, and ipv6 server addresses to the server again.";
	// You can open it yourself if necessary
	// luat_rtos_task_handle flymode_task_handle;
	// luat_rtos_task_create(&flymode_task_handle, 2048, 20, "flymode", flymode_task, NULL, NULL);
	
}

INIT_TASK_EXPORT(ipv6_sleep_wakeup_init, "1");
