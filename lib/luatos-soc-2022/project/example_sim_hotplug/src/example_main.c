#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_gpio.h"
#include "luat_mobile.h"

#define SIM_HOTPLUG_PIN         HAL_WAKEUP_2
#define SIM_REMOVE              LUAT_GPIO_HIGH
#define SIM_INSERT              LUAT_GPIO_LOW

static luat_rtos_task_handle g_s_task_handle;

static void mobile_event_cb(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	luat_mobile_cell_info_t cell_info;
	luat_mobile_signal_strength_info_t signal_info;
	uint8_t csq, i;
	char imsi[20];
	char iccid[24] = {0};
	char apn[32] = {0};
	ip_addr_t ipv4;
	ip_addr_t ipv6;
	switch(event)
	{
	case LUAT_MOBILE_EVENT_CFUN:
		LUAT_DEBUG_PRINT("CFUN message, status %d", status);
		break;
	case LUAT_MOBILE_EVENT_SIM:
		if (status != LUAT_MOBILE_SIM_NUMBER)
		{
			LUAT_DEBUG_PRINT("SIM card message, card slot %d", index);
		}
		switch(status)
		{
		case LUAT_MOBILE_SIM_READY:
			LUAT_DEBUG_PRINT("SIM card works normally");
			luat_mobile_get_iccid(index, iccid, sizeof(iccid));
			LUAT_DEBUG_PRINT("ICCID %s", iccid);
			luat_mobile_get_imsi(index, imsi, sizeof(imsi));
			LUAT_DEBUG_PRINT("IMSI %s", imsi);
			break;
		case LUAT_MOBILE_NO_SIM:
			LUAT_DEBUG_PRINT("SIM card does not exist");
			break;
		case LUAT_MOBILE_SIM_NEED_PIN:
			LUAT_DEBUG_PRINT("SIM card requires PIN code");
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_REGISTER_STATUS:
		LUAT_DEBUG_PRINT("Mobile network service status changes, currently %d", status);
		break;
	case LUAT_MOBILE_EVENT_CELL_INFO:
		switch(status)
		{
		case LUAT_MOBILE_CELL_INFO_UPDATE:
			LUAT_DEBUG_PRINT("Periodic search for cell information is completed once");
			luat_mobile_get_last_notify_cell_info(&cell_info);
			if (cell_info.lte_service_info.cid)
			{
				LUAT_DEBUG_PRINT("Service cell information mcc %x mnc %x cellid %u band %d tac %u pci %u earfcn %u is_tdd %d rsrp %d rsrq %d snr %d rssi %d",
						cell_info.lte_service_info.mcc, cell_info.lte_service_info.mnc, cell_info.lte_service_info.cid,
						cell_info.lte_service_info.band, cell_info.lte_service_info.tac, cell_info.lte_service_info.pci, cell_info.lte_service_info.earfcn,
						cell_info.lte_service_info.is_tdd, cell_info.lte_service_info.rsrp, cell_info.lte_service_info.rsrq,
						cell_info.lte_service_info.snr, cell_info.lte_service_info.rssi);
			}
			for (i = 0; i < cell_info.lte_neighbor_info_num; i++)
			{
				if (cell_info.lte_info[i].cid)
				{
					LUAT_DEBUG_PRINT("Neighboring cell %d mcc %x mnc %x cellid %u tac %u pci %u", i + 1, cell_info.lte_info[i].mcc,
							cell_info.lte_info[i].mnc, cell_info.lte_info[i].cid, cell_info.lte_info[i].tac, cell_info.lte_info[i].pci);
					LUAT_DEBUG_PRINT("Neighboring cell %d earfcn %u rsrp %d rsrq %d snr %d", i + 1, cell_info.lte_info[i].earfcn, cell_info.lte_info[i].rsrp,
							cell_info.lte_info[i].rsrq, cell_info.lte_info[i].snr);
				}
			}
			break;
		case LUAT_MOBILE_SIGNAL_UPDATE:
			LUAT_DEBUG_PRINT("Service cell signal status changes");
			luat_mobile_get_last_notify_signal_strength_info(&signal_info);
			luat_mobile_get_last_notify_signal_strength(&csq);
			if (signal_info.luat_mobile_lte_signal_strength_vaild)
			{
				LUAT_DEBUG_PRINT("rsrp %d, rsrq %d, snr %d, rssi %d, csq %d %d", signal_info.lte_signal_strength.rsrp,
						signal_info.lte_signal_strength.rsrq, signal_info.lte_signal_strength.snr,
						signal_info.lte_signal_strength.rssi, csq, luat_mobile_rssi_to_csq(signal_info.lte_signal_strength.rssi));
			}

			break;
		}
		break;
	case LUAT_MOBILE_EVENT_PDP:
		LUAT_DEBUG_PRINT("CID %d PDP activation status changed to %d", index, status);
		break;
	case LUAT_MOBILE_EVENT_NETIF:
		LUAT_DEBUG_PRINT("The internet working status changes to %d, cause %d", status,index);
		switch (status)
		{
		case LUAT_MOBILE_NETIF_LINK_ON:
			LUAT_DEBUG_PRINT("Can access the Internet");
			if (luat_mobile_get_apn(0, 0, apn, sizeof(apn)))
			{
				LUAT_DEBUG_PRINT("Default apn %s", apn);
			}
			luat_mobile_get_local_ip(0, 1, &ipv4, &ipv6);
			if (ipv4.type != 0xff)
			{
				LUAT_DEBUG_PRINT("IPV4 %s", ip4addr_ntoa(&ipv4.u_addr.ip4));
			}
			if (ipv6.type != 0xff)
			{
				LUAT_DEBUG_PRINT("IPV6 %s", ip6addr_ntoa(&ipv4.u_addr.ip6));
			}
			break;
		default:
			LUAT_DEBUG_PRINT("Can't access the Internet");
			break;
		}
		break;
	case LUAT_MOBILE_EVENT_TIME_SYNC:
		LUAT_DEBUG_PRINT("UTC time synchronized over mobile network");
		break;
	case LUAT_MOBILE_EVENT_CSCON:
		LUAT_DEBUG_PRINT("RRC status %d", status);
		break;
	case LUAT_MOBILE_EVENT_NAS_ERROR:
		LUAT_DEBUG_PRINT("NAS exception type %d, rejection reason %d", index, status);
		break;
	case LUAT_MOBILE_EVENT_FATAL_ERROR:
		LUAT_DEBUG_PRINT("The network needs serious failure. It is recommended to restart the protocol stack after 5 seconds.");
		break;
	default:
		break;
	}
}

static int gpio_cb(int pin, void *param)
{
    if (SIM_HOTPLUG_PIN == pin)
    {
        if(luat_gpio_get(pin) == SIM_REMOVE){
            luat_rtos_event_send(g_s_task_handle, 0, SIM_REMOVE, 0, 0, 0);
        }else{
            luat_rtos_event_send(g_s_task_handle, 0, SIM_INSERT, 0, 0, 0);
        }
    }
}

static void test_task(void *param){
    luat_gpio_cfg_t cfg = {0};
    cfg.pin = SIM_HOTPLUG_PIN;
    cfg.mode = LUAT_GPIO_IRQ;
    cfg.pull = LUAT_GPIO_PULLUP;
    cfg.irq_type = LUAT_GPIO_BOTH_IRQ;
    cfg.irq_cb = gpio_cb;
    luat_gpio_open(&cfg);
    luat_event_t event = {0};
    while (1)
    {
        if(0 == luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER))
        {
            if (SIM_REMOVE == event.param1)
            {
                LUAT_DEBUG_PRINT("Remove the sim card");
                luat_mobile_set_flymode(0, 1);
            }
            else
            {
                LUAT_DEBUG_PRINT("sim card inserted");
                luat_mobile_set_flymode(0, 0);
            }
        }
    }
}
	

static void task_demo(void)
{
    luat_mobile_event_register_handler(mobile_event_cb);
    luat_rtos_task_create(&g_s_task_handle, 2048, 20, "test", test_task, NULL, 10);
}

INIT_TASK_EXPORT(task_demo,"1");



