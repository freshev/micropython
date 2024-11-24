#include "luat_debug.h"
#include "luat_rtos.h"
#include "luat_mobile.h"

/*Function Modules name: IP network monitoring;

This functional Modules monitors the IP layer network link status;
The external function Modules can obtain the status by calling the network_service_is_ready interface. 1 indicates that the IP network is ready, 0 indicates that the IP network is not ready;
After the IP network is ready, the external function Modules can normally use socket, http, mqtt, etc. to connect to the server and send and receive data;*/


static uint8_t g_s_is_link_up;
static uint8_t g_s_cid;

uint8_t network_service_is_ready(void)
{
	return g_s_is_link_up;
}

uint8_t network_service_get_cid(void)
{
	return g_s_cid;
}


static void mobile_event_cb(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	switch(event)
	{
	case LUAT_MOBILE_EVENT_CFUN:
		LUAT_DEBUG_PRINT("cfun event,status %d", status);
		break;
	case LUAT_MOBILE_EVENT_SIM:
		switch(status)
		{
		case LUAT_MOBILE_SIM_READY:
			LUAT_DEBUG_PRINT("sim event ready");
			break;
		case LUAT_MOBILE_NO_SIM:
			LUAT_DEBUG_PRINT("sim event not insert");
			break;
		case LUAT_MOBILE_SIM_NEED_PIN:
			LUAT_DEBUG_PRINT("sim event need pin");
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_REGISTER_STATUS:
		LUAT_DEBUG_PRINT("register event, status %d", status);
		break;
	case LUAT_MOBILE_EVENT_CELL_INFO:
		switch(status)
		{
		case LUAT_MOBILE_CELL_INFO_UPDATE:
			break;
		case LUAT_MOBILE_SIGNAL_UPDATE:
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_PDP:
        LUAT_DEBUG_PRINT("pdp event, cid %d, status %d", index, status);
		g_s_cid = index;
		break;
	case LUAT_MOBILE_EVENT_NETIF:
		switch (status)
		{
		case LUAT_MOBILE_NETIF_LINK_ON:
			LUAT_DEBUG_PRINT("netif link on event");
			g_s_is_link_up = 1;
			break;
		default:
			LUAT_DEBUG_PRINT("netif link off event");
			g_s_is_link_up = 0;
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_TIME_SYNC:
		LUAT_DEBUG_PRINT("time sync event");
		break;
	case LUAT_MOBILE_EVENT_CSCON:
		LUAT_DEBUG_PRINT("rrc event, status %d", status);
		break;
	default:
		break;
	}
}

void network_service_init(void)
{
	luat_mobile_event_register_handler(mobile_event_cb);
}
