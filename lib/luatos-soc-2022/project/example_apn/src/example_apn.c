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
#include "luat_rtos.h"
#include "luat_mobile.h"
#include "luat_debug.h"
#define AUTO_APN_TEST	//Use the mode of automatically setting APN. If you want to set it manually, comment it out. You need to use the default special APN activation, which must be turned on.
#ifndef AUTO_APN_TEST
static g_s_test_cid = 2;
#endif
static void sms_event_cb(uint32_t event, void *param)
{
	LUAT_DEBUG_PRINT("SMS event%d,%x",event, param);
}

static void mobile_event_cb(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	luat_mobile_cell_info_t cell_info;
	luat_mobile_signal_strength_info_t signal_info;
	int apn_len;
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
		LUAT_DEBUG_PRINT("SIM card message, card slot %d", index);
		switch(status)
		{
		case LUAT_MOBILE_SIM_READY:
			LUAT_DEBUG_PRINT("SIM card works normally");
			luat_mobile_get_iccid(0, iccid, sizeof(iccid));
			LUAT_DEBUG_PRINT("ICCID %s", iccid);
			luat_mobile_get_imsi(0, imsi, sizeof(imsi));
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
#ifndef AUTO_APN_TEST
		if ((g_s_test_cid == index) && !status)
		{
			luat_mobile_active_netif(0, index);
		}
#endif
		break;
	case LUAT_MOBILE_EVENT_NETIF:
		LUAT_DEBUG_PRINT("The internet working status changes to %d", status);
		switch (status)
		{
		case LUAT_MOBILE_NETIF_LINK_ON:
			LUAT_DEBUG_PRINT("Can access the Internet");
#ifndef AUTO_APN_TEST
			luat_mobile_get_local_ip(0, g_s_test_cid, &ipv4, &ipv6);
#else
			luat_mobile_get_local_ip(0, 1, &ipv4, &ipv6);
#endif
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
	case LUAT_MOBILE_EVENT_BEARER:
		switch(status)
		{
		case LUAT_MOBILE_BEARER_GET_DEFAULT_APN:
			//If you do not use the default APN for activation, you can set the APN directly without waiting to get the default APN.
			apn_len = luat_mobile_get_apn(0, 0, apn, sizeof(apn));
			if (apn_len > 0)
			{
				LUAT_DEBUG_PRINT("Default apn %s", apn);
			}
#ifndef AUTO_APN_TEST
			luat_mobile_set_apn_base_info(0, g_s_test_cid, 3, apn, apn_len);
#endif
			break;
		case LUAT_MOBILE_BEARER_APN_SET_DONE:
#ifndef AUTO_APN_TEST
			//You can activate it directly without setting it.
			luat_mobile_set_apn_auth_info(0, g_s_test_cid, 0xff, NULL, 0, NULL, 0);
#endif
			break;
		case LUAT_MOBILE_BEARER_AUTH_SET_DONE:
#ifndef AUTO_APN_TEST
			luat_mobile_active_apn(0, g_s_test_cid, 1);
#endif
			break;
		case LUAT_MOBILE_BEARER_DEL_DONE:
			break;
		case LUAT_MOBILE_BEARER_SET_ACT_STATE_DONE:
			//Here it just says that the operation was executed, but whether it can be activated successfully is not checked here.
			break;
		}
		break;
	default:
		break;
	}
}

static void task_run(void *param)
{
	int i;
	luat_mobile_cell_info_t  cell_info;
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);
#ifndef AUTO_APN_TEST
	luat_mobile_user_ctrl_apn();
#endif
	/*When setting the APN information of the private network card, if you need to set it as the first bearer (CID 1), you must use the automatic mode to activate it.

If it is carried by other roads, automatic mode or manual mode can be used

If the Modules with APN information set needs to be replaced with a normal card, the apn needs to be cleared.

If APN requires setting a password, set it according to APN information and encryption protocol

The apn information needs to be configured before turning on the computer and registering for the network.*/
#ifdef AUTO_APN_TEST
	//If you have used a special APN card before and now want to switch back to a normal card, it is recommended to delete the originally set APN information and call the del interface below.
//	luat_mobile_del_apn(0,1,0);
	luat_mobile_user_apn_auto_active(0, 1, 3,3, "CMIOTTQJ",8,"ceshi",5,"tqj123456",9);
#endif

	char imei[20] = {0};
	luat_mobile_get_imei(0, imei, sizeof(imei));
	LUAT_DEBUG_PRINT("IMEI %s", imei);
	luat_mobile_set_sn("1234567890abcdefghijklmnopqrstuv",32);
	char sn[33] = {0};
	luat_mobile_get_sn(sn, sizeof(sn));
	LUAT_DEBUG_PRINT("SN %s", sn);
	char muid[64] = {0};
	luat_mobile_get_muid(muid, sizeof(muid));
	LUAT_DEBUG_PRINT("MUID %s", muid);
	char apn[64] = {0};
	luat_rtos_task_sleep(10000);
	//Here is a demonstration of deleting an already set APN.
	luat_mobile_user_apn_auto_active(0, 0, 0,0, NULL, 0, NULL, 0, NULL, 0);	//Delete cached data in RAM
	luat_mobile_user_ctrl_apn_stop();//Change back to automatic configuration
	luat_mobile_set_flymode(0, 1);//Delete the advanced flight mode saved by the system
	luat_rtos_task_sleep(100);
	luat_mobile_del_apn(0,1,0);//Delete the ones saved by the system
	luat_mobile_set_flymode(0, 0);//Exit airplane mode

	//luat_mobile_reset_stack();
	while(1)
	{
		luat_rtos_task_sleep(10000);
		luat_mobile_get_apn(0, 0, apn, 10);	//Here we deliberately do not get the complete
		DBG("%s", apn);
	}
}

void task_init(void)
{
	luat_mobile_event_register_handler(mobile_event_cb);
	luat_mobile_sms_event_register_handler(sms_event_cb);
	luat_mobile_set_period_work(90000, 0, 4);
	luat_mobile_set_sim_id(2);
	luat_rtos_task_handle task_handle;
	luat_rtos_task_create(&task_handle, 4 * 1204, 50, "test", task_run, NULL, 32);

}

INIT_TASK_EXPORT(task_init, "0");
