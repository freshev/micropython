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
#include "cmsis_os2.h"
#include "os_common.h"
#include "showmac.h"

extern BOOL    bSoftSIMTaskCreate;

void SoftSimReset(UINT16 *atrLen, UINT8 *atrData)
{
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, PNULL);
    osStatus_t osState = osOK;

    /*
    * softsim vender shall implement 3 steps as below
    */

    /*
    * 1> create signal/msg
    */

    /*
    * 2> send signal/msg to softsim task with sem/atrLen/atrData
    */

    /*
    * 3> process signal/msg on softsim task, then retrun paramters and release sem on softsim task
    */
    send_vsim_reset_signal(atrLen, atrData, sem);

    /*
     * wait for sem 2sec
    */
    if ((osState = osSemaphoreAcquire(sem, 2000)) != osOK)
    {
        OsaDebugBegin(FALSE, osState, 0, 0);
        OsaDebugEnd();
    }

    /*
     * Semaphore delete
    */
    osSemaphoreDelete(sem);

}

void SoftSimApduReq(UINT16 txDataLen, UINT8 *txData, UINT16 *rxDataLen, UINT8 *rxData)
{
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, PNULL);
    osStatus_t osState = osOK;

    /*
    * softsim vender shall implement 3 steps as below
    */

    /*
    * 1> create signal/msg
    */

    /*
    * 2> send signal/msg to softsim task with sem/txDataLen/txData/rxDataLen/rxData
    */

    /*
    * 3> process signal/msg on softsim task, then retrun paramters and release sem on softsim task
    */
    send_vsim_apdu_signal(txDataLen, txData, rxDataLen, rxData, sem);

    /*
     * wait for sem 2sec
    */
    if ((osState = osSemaphoreAcquire(sem, 2000)) != osOK)
    {
        OsaDebugBegin(FALSE, osState, 0, 0);
        OsaDebugEnd();
    }

    /*
     * Semaphore delete
    */
    osSemaphoreDelete(sem);

}

void SoftSimInit(void)
{
    if (bSoftSIMTaskCreate == TRUE)
    {
        ECPLAT_PRINTF(UNILOG_PLA_DRIVER, SoftSimInit_0, P_INFO, "Softsim task has already been created");
        return;
    }
    /*
    * start softsim task
    */
    ECPLAT_PRINTF(UNILOG_PLA_DRIVER, SoftSimInit_1, P_INFO, "Start softsim task");
    /*
    * softsim vender implement softsim task created
    */

    showmac_app_start();

    //delay 100ms for softsim task init
    osDelay(100);

    //set flag after task created
    bSoftSIMTaskCreate = TRUE;

}


static void sms_event_cb(uint32_t event, void *param)
{
	LUAT_DEBUG_PRINT("SMS event%d,%x",event, param);
}

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
	case LUAT_MOBILE_EVENT_FATAL_ERROR:
		LUAT_DEBUG_PRINT("The network needs serious failure. It is recommended to restart the protocol stack.");
		//If you plan to enter airplane mode, you can ignore this error.
		//If automatic recovery is not set through luat_mobile_fatal_error_auto_reset_stack, manual recovery is required.
		luat_mobile_reset_stack();
		break;
	default:
		break;
	}
}

static void task_run(void *param)
{
	/*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! ! 1*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET); 
    luat_rtos_task_sleep(5000);
    uint8_t is_soft_sim = 2;
    int get_cfg_result = luat_mobile_get_soft_sim_cfg(&is_soft_sim);
    LUAT_DEBUG_PRINT("get sim cfg result %d %d", get_cfg_result, is_soft_sim);
    if(0 == get_cfg_result)
    {
        if(0 == is_soft_sim)
        {
            luat_mobile_set_flymode(0, 1);
            luat_rtos_task_sleep(5000);
            LUAT_DEBUG_PRINT("switch soft sim result %d", luat_mobile_soft_sim_switch(1));
            luat_mobile_set_flymode(0, 0);
            luat_rtos_task_sleep(5000);
        }
    }
   
    
	int i;
	luat_mobile_cell_info_t  cell_info;
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
	luat_rtos_task_sleep(1000);

	uint8_t band[32];
	uint8_t total_num;
	uint8_t band1[32];
	uint8_t total_num1;
	luat_mobile_get_band(band1, &total_num1);
	for(i = 0; i < total_num1; i++)
	{
		LUAT_DEBUG_PRINT("Use band %d", band1[i]);
	}
	LUAT_DEBUG_PRINT("Modify frequency band");
	uint8_t band2[3] = {38,39,40};
	luat_mobile_set_band(band2, 3);
	luat_mobile_get_band(band, &total_num);
	for(i = 0; i < total_num; i++)
	{
		LUAT_DEBUG_PRINT("Use band %d", band[i]);
	}
	LUAT_DEBUG_PRINT("recovery band");
	luat_mobile_set_band(band1, total_num1);
	luat_mobile_get_band(band, &total_num);
	for(i = 0; i < total_num; i++)
	{
		LUAT_DEBUG_PRINT("Use band %d", band[i]);
	}


	while(1)
	{
		luat_rtos_task_sleep(120000);
		luat_mobile_set_sim_id(0);
		luat_rtos_task_sleep(10000);
		luat_mobile_set_sim_id(1);
		luat_rtos_task_sleep(10000);
		luat_mobile_set_sim_id(2);	//During automatic recognition, you need to manually restart the protocol stack or start the SIM card automatic recovery function.
		luat_mobile_reset_stack();
		luat_rtos_task_sleep(15000);
		LUAT_DEBUG_PRINT("Manually obtain surrounding cell information once, asynchronously");
		luat_mobile_get_cell_info_async(6);
		luat_rtos_task_sleep(15000);
		LUAT_DEBUG_PRINT("Manually obtain surrounding community information once, synchronization method");
		luat_mobile_get_cell_info(&cell_info);
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

	}
}

void task_init(void)
{
	luat_mobile_event_register_handler(mobile_event_cb);
	luat_mobile_sms_event_register_handler(sms_event_cb);
	luat_mobile_set_period_work(90000, 0, 4);
	luat_mobile_set_sim_id(2);
	luat_mobile_set_sim_detect_sim0_first();
	luat_rtos_task_handle task_handle;
	luat_rtos_task_create(&task_handle, 8*1024, 50, "test", task_run, NULL, 32);
}

INIT_TASK_EXPORT(task_init, "0");
