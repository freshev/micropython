#include "luat_network_adapter.h"
#include "common_api.h"
#include "luat_debug.h"

#include "luat_rtos.h"
#include "luat_mobile.h"
#include "networkmgr.h"

#include "luat_mem.h"

#include "cJSON.h"
#include "andlink.h"

/*Modifying the following parameters can be quickly verified. For more parameter customization, please read the demo below in detail.*/
#define PRODUCT_ID           "XXX"
#define ANDLINK_TOKEN        "XXX"
#define PRODUCT_TOKEN        "XXX"
#define IMEI        		 "XXX"
#define SN 					 "XXX"

#define ANDLINK_QUEUE_SIZE 	 32
static luat_is_link_up = 0;
static luat_rtos_task_handle andlink_init_task_handle;
static luat_rtos_task_handle andlink_task_handle;
static luat_rtos_queue_t andlink_queue_handle;

//Notify device status
int andlink_set_led_callback(ADL_DEV_STATE_e state){
	return luat_rtos_queue_send(andlink_queue_handle, &state, NULL, 0);
}

//Downward control
int andlink_dn_send_cmd_callback(RESP_MODE_e mode, dn_dev_ctrl_frame_t *ctrlFrame, char *eventType, char *respData, int respBufSize){

	return 0;
}

// Get device IP
int andlink_get_device_ipaddr(char *ip, char *broadAddr){
	char *ipaddr = NULL;
	ip_addr_t ipv4 = {.type=0xFF};
	ip_addr_t ipv6 = {.type=0xFF};
	while (luat_is_link_up == 0){
		luat_rtos_task_sleep(100);
	}
	luat_mobile_get_local_ip(0, 1, &ipv4, &ipv6);
	if (ipv4.type != 0xff){
		ipaddr = ip4addr_ntoa(&ipv4.u_addr.ip4);
	}
	if (ipv6.type != 0xff){
		ipaddr = ip4addr_ntoa(&ipv4.u_addr.ip6);
	}
	if (ipaddr == NULL) {
		return -1;
	}
	memcpy(ip, ipaddr, strlen(ipaddr));
	return 0;
}


static void luat_andlink_init_task(void *param)
{
	luat_rtos_task_sleep(2000);
	cJSON* chips_json;
	cJSON* extInfo_json = cJSON_CreateObject();
	cJSON_AddStringToObject(extInfo_json, "cmei", IMEI);				// Unique identifier of the device, required
	cJSON_AddNumberToObject(extInfo_json, "authMode", 0);				// 0 indicates type authentication, 1 indicates device authentication. When authenticating device, authId and authKey need to be used.
	cJSON_AddStringToObject(extInfo_json, "sn", SN);					//Device SN, required
	cJSON_AddStringToObject(extInfo_json, "manuDate", "2023-07");		// Equipment production date, format is year-month
	cJSON_AddStringToObject(extInfo_json, "OS", "Freertos");			// operating system
	cJSON* chips_array = cJSON_CreateArray();
	cJSON_AddItemToArray(chips_array,chips_json = cJSON_CreateObject());
	cJSON_AddItemToObject(extInfo_json, "chips", chips_array);			// Chip information, can contain multiple groups
	cJSON_AddStringToObject(chips_json, "type", "4G");					// Chip type, such as Main/WiFi/Zigbee/BLE, etc.
	cJSON_AddStringToObject(chips_json, "factory", "airm2m");			// chip manufacturer
	cJSON_AddStringToObject(chips_json, "model", "air780");				// Chip model
	char* extInfo = cJSON_Print(extInfo_json);
	cJSON_Delete(extInfo_json);
	adl_dev_attr_t devAttr = {
		.cfgNetMode = NETWOKR_MODE_4G,
		.deviceType = PRODUCT_ID,
		.deviceMac =  IMEI,
		.andlinkToken = ANDLINK_TOKEN,
		.productToken = PRODUCT_TOKEN,
		.firmWareVersion = "0.0.1",
		.softWareVersion = "0.0.1",
		.extInfo = extInfo,
	};
	adl_dev_callback_t devCbs = {
		.set_led_callback = andlink_set_led_callback,
		.dn_send_cmd_callback = andlink_dn_send_cmd_callback,
		.get_device_ipaddr = andlink_get_device_ipaddr,
	};
	andlink_init(&devAttr, &devCbs);
	cJSON_free(extInfo);
	luat_rtos_task_delete(andlink_init_task_handle);
}

static void luat_andlink_task(void *param){
	ADL_DEV_STATE_e andlink_state;
	while (1){
        if (luat_rtos_queue_recv(andlink_queue_handle, &andlink_state, NULL, 5000) == 0){
			switch (andlink_state)
			{
			case ADL_BOOTSTRAP:
				LUAT_DEBUG_PRINT("Device starts registration status");
				break;
			case ADL_BOOTSTRAP_SUC:
				LUAT_DEBUG_PRINT("Device registration success status");
				break;
			case ADL_BOOTSTRAP_FAIL:
				LUAT_DEBUG_PRINT("Device registration failed status");
				break;
			case ADL_BOOT:
				LUAT_DEBUG_PRINT("The device starts to go online");
				break;
			case ADL_BOOT_SUC:
				LUAT_DEBUG_PRINT("Device online success status");
				break;
			case ADL_BOOT_FAIL:
				LUAT_DEBUG_PRINT("Device online failure status");
				break;
			case ADL_ONLINE:
				LUAT_DEBUG_PRINT("Device online status");
				break;
			case ADL_RESET:
				LUAT_DEBUG_PRINT("Device reset state");
				break;
			case ADL_OFFLINE:
				LUAT_DEBUG_PRINT("Device offline status");
				break;
			}
        }
	}
}


static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	if (LUAT_MOBILE_EVENT_NETIF == event)
	{
		if (LUAT_MOBILE_NETIF_LINK_ON == status)
		{
			luat_is_link_up = 1;
			luat_socket_check_ready(index, NULL);
		}
        else if(LUAT_MOBILE_NETIF_LINK_OFF == status || LUAT_MOBILE_NETIF_LINK_OOS == status)
        {
            luat_is_link_up = 0;
        }
	}
}

static void luat_libandlink_init(void)
{
	luat_mobile_event_register_handler(luatos_mobile_event_callback);
	net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
	luat_rtos_queue_create(&andlink_queue_handle, ANDLINK_QUEUE_SIZE, sizeof(ADL_DEV_STATE_e));
	luat_rtos_task_create(&andlink_init_task_handle, 4 * 1024, 10, "andlink_init", luat_andlink_init_task, NULL, 16);
	luat_rtos_task_create(&andlink_task_handle, 4 * 1024, 10, "andlink", luat_andlink_task, NULL, 16);
}

INIT_TASK_EXPORT(luat_libandlink_init, "1");
