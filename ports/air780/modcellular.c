/*                                                                                          
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 pulkin
 * Copyright (c) 2024 freshev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "cmimm.h"
#include "modcellular.h"
#include "modcellsms.c"

// Tracks the status on the network
uint8_t network_status = 0;
uint16_t network_exception = NTW_NO_EXC;
uint8_t network_signal_quality = 0;
int16_t network_signal_rx_level = 0;
int8_t network_signal_snr = 0;
mp_obj_t network_status_callback = mp_const_none;
mp_obj_list_t *plmn_list_buffer = NULL;
uint8_t plmn_list_flag = 0;

void modcellular_network_status_update(uint8_t new_status, uint16_t new_exception) {
    if (new_exception) network_exception = new_exception;
    network_status = new_status;
    if (network_status_callback && network_status_callback != mp_const_none) {
    	mp_sched_schedule(network_status_callback, mp_obj_new_int(network_status));
    }
}

mp_obj_t modcellular_plmn_from_list_record(CmiMmManualPlmnSearchCnf *record);

static void mobile_event_cb(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status) {
	luat_mobile_cell_info_t cell_info;
	luat_mobile_signal_strength_info_t signal_info;
	uint8_t csq, i;
	char imsi[20];
	char iccid[24] = {0};
	char apn[32] = {0};
	ip_addr_t ipv4;
	ip_addr_t ipv6;

	switch(event) {
		case LUAT_MOBILE_EVENT_CFUN:
			LUAT_DEBUG_PRINT("CFUN message, status %d", status);
			break;
        
		case LUAT_MOBILE_EVENT_SIM:
			//if (status != LUAT_MOBILE_SIM_NUMBER) {  LUAT_DEBUG_PRINT("SIM card message, card slot %d", index); }
			switch(status) {
				case LUAT_MOBILE_SIM_READY:
					//LUAT_DEBUG_PRINT("SIM card works normally");
					//luat_mobile_get_iccid(index, iccid, sizeof(iccid));
					//LUAT_DEBUG_PRINT("ICCID %s", iccid);
					//luat_mobile_get_imsi(index, imsi, sizeof(imsi));
					//LUAT_DEBUG_PRINT("IMSI %s", imsi);
					break;
				case LUAT_MOBILE_NO_SIM:
					//LUAT_DEBUG_PRINT("SIM card does not exist");
        			modcellular_network_status_update(0, NTW_EXC_NOSIM);
					break;
				case LUAT_MOBILE_SIM_NEED_PIN:
					LUAT_DEBUG_PRINT("SIM card requires PIN code");
					break;
				}
			break;
		case LUAT_MOBILE_EVENT_REGISTER_STATUS:
			
			switch(status) {
				case LUAT_MOBILE_STATUS_UNREGISTER:
					LUAT_DEBUG_PRINT("Mobile status unregistered");
    				modcellular_network_status_update(0, 0);
    				break;
				case LUAT_MOBILE_STATUS_REGISTERED:
    				LUAT_DEBUG_PRINT("Mobile status registered home", status);
    				modcellular_network_status_update(NTW_REG_BIT, 0);
    				break;	
				case LUAT_MOBILE_STATUS_INSEARCH:
					LUAT_DEBUG_PRINT("Mobile status searching");
    				modcellular_network_status_update(NTW_REG_PROGRESS_BIT, 0);
    				break;
    			case LUAT_MOBILE_STATUS_DENIED:
					LUAT_DEBUG_PRINT("Mobile status registration denied");
    				modcellular_network_status_update(0, NTW_EXC_REG_DENIED);
    				break;
    			case LUAT_MOBILE_STATUS_UNKNOW:
					LUAT_DEBUG_PRINT("Mobile status unknown");    				
    				break;
    			case LUAT_MOBILE_STATUS_REGISTERED_ROAMING:
					LUAT_DEBUG_PRINT("Mobile status registered roaming");
    				modcellular_network_status_update(NTW_REG_BIT | NTW_ROAM_BIT, 0);
    				break;
    			case LUAT_MOBILE_STATUS_EMERGENCY_REGISTERED:
					LUAT_DEBUG_PRINT("Mobile status registered emergency only");
    				break;    			
    			default:
    				LUAT_DEBUG_PRINT("Mobile status %d", status);
    				break;
    		}
			break;

		case LUAT_MOBILE_EVENT_CELL_INFO:
			switch(status) {
				case LUAT_MOBILE_CELL_INFO_UPDATE:
					//LUAT_DEBUG_PRINT("Periodic search for cell information is completed once");
					luat_mobile_get_last_notify_cell_info(&cell_info);
					if (cell_info.lte_service_info.cid)
					{
						//LUAT_DEBUG_PRINT("Service cell information mcc %x mnc %x cellid %u band %d tac %u pci %u earfcn %u is_tdd %d rsrp %d rsrq %d snr %d rssi %d",
						//		cell_info.lte_service_info.mcc, cell_info.lte_service_info.mnc, cell_info.lte_service_info.cid,
						//		cell_info.lte_service_info.band, cell_info.lte_service_info.tac, cell_info.lte_service_info.pci, cell_info.lte_service_info.earfcn,
						//		cell_info.lte_service_info.is_tdd, cell_info.lte_service_info.rsrp, cell_info.lte_service_info.rsrq,
						//		cell_info.lte_service_info.snr, cell_info.lte_service_info.rssi);
					}
					for (i = 0; i < cell_info.lte_neighbor_info_num; i++)
					{
						if (cell_info.lte_info[i].cid)
						{
							//LUAT_DEBUG_PRINT("Neighboring cell %d mcc %x mnc %x cellid %u tac %u pci %u", i + 1, cell_info.lte_info[i].mcc,
							//		cell_info.lte_info[i].mnc, cell_info.lte_info[i].cid, cell_info.lte_info[i].tac, cell_info.lte_info[i].pci);
							//LUAT_DEBUG_PRINT("Neighboring cell %d earfcn %u rsrp %d rsrq %d snr %d", i + 1, cell_info.lte_info[i].earfcn, cell_info.lte_info[i].rsrp,
							//		cell_info.lte_info[i].rsrq, cell_info.lte_info[i].snr);
						}
					}
					break;
				case LUAT_MOBILE_SIGNAL_UPDATE:
					//LUAT_DEBUG_PRINT("Service cell signal status changes");
					luat_mobile_get_last_notify_signal_strength_info(&signal_info);
					luat_mobile_get_last_notify_signal_strength(&csq);
					if (signal_info.luat_mobile_lte_signal_strength_vaild)
					{
						//LUAT_DEBUG_PRINT("rsrp %d, rsrq %d, snr %d, rssi %d, csq %d %d", signal_info.lte_signal_strength.rsrp,
						//		signal_info.lte_signal_strength.rsrq, signal_info.lte_signal_strength.snr,
						//		signal_info.lte_signal_strength.rssi, csq, luat_mobile_rssi_to_csq(signal_info.lte_signal_strength.rssi));

						network_signal_quality = luat_mobile_rssi_to_csq(signal_info.lte_signal_strength.rssi);
    					network_signal_rx_level = signal_info.lte_signal_strength.rssi;
    					network_signal_snr = signal_info.lte_signal_strength.snr;
					}
                
					break;
			}
			break;

		case LUAT_MOBILE_EVENT_PDP:
			//LUAT_DEBUG_PRINT("CID %d PDP activation status changed to %d", index, status);
			// modcellular_network_status_update(network_status & ~NTW_ACT_BIT, 0); ????
			break;

		case LUAT_MOBILE_EVENT_NETIF:
			//LUAT_DEBUG_PRINT("The internet working status changes to %d, cause %d", status,index);
			switch (status) {
				case LUAT_MOBILE_NETIF_LINK_ON:
					//LUAT_DEBUG_PRINT("Can access the Internet");
					//if (luat_mobile_get_apn(0, 0, apn, sizeof(apn))) {
					//	LUAT_DEBUG_PRINT("Default apn %s", apn);
					//}
					luat_mobile_get_local_ip(0, 1, &ipv4, &ipv6);
					if (ipv4.type != 0xff) { LUAT_DEBUG_PRINT("IPV4 %s", ip4addr_ntoa(&ipv4.u_addr.ip4)); }
					if (ipv6.type != 0xff) { LUAT_DEBUG_PRINT("IPV6 %s", ip6addr_ntoa(&ipv4.u_addr.ip6)); }
    				modcellular_network_status_update(network_status | NTW_ACT_BIT, 0);
					break;
				default:
					//LUAT_DEBUG_PRINT("Can't access the Internet");
					modcellular_network_status_update(network_status & ~NTW_ACT_BIT, NTW_EXC_ACT_FAILED);
					break;
			}
			break;
		case LUAT_MOBILE_EVENT_TIME_SYNC:
			//if (status == 0) {
			//	LUAT_DEBUG_PRINT("Time synchronized");
			//} else {
			//	LUAT_DEBUG_PRINT("Time NOT synchronized");
			//}
			break;

		case LUAT_MOBILE_EVENT_CSCON:
			if(status == 0) {
				//LUAT_DEBUG_PRINT("RRC status detached");
				modcellular_network_status_update(network_status & ~NTW_ATT_BIT, 0);
			}
			if(status == 1) {
				//LUAT_DEBUG_PRINT("RRC status attached");
    			modcellular_network_status_update(network_status | NTW_ATT_BIT, 0);
			}
			// modcellular_network_status_update(network_status & ~NTW_ATT_BIT, NTW_EXC_ATT_FAILED);
			break;

		case LUAT_MOBILE_EVENT_BEARER:
		    //LUAT_DEBUG_PRINT("BEARER status %d", status);
		    break;

		case LUAT_MOBILE_EVENT_NAS_ERROR:
			LUAT_DEBUG_PRINT("NAS exception type %d, rejection reason %d", index, status);
			break;

		case LUAT_MOBILE_EVENT_FATAL_ERROR:
			LUAT_DEBUG_PRINT("The network needs serious failure. It is recommended to restart the protocol stack after 5 seconds.");
			break;
		
		/*
 		//<stat>: integer type
 		// 0 unknown
		// 1 available
		// 2 current
		// 3 forbidden
		typedef enum CmiMmPlmnStateEnum_Tag {
    		CMI_MM_PLMN_UNKNOWN = 0,                    
    		CMI_MM_PLMN_AVAILABLE,
    		CMI_MM_PLMN_CURRENT,
    		CMI_MM_PLMN_FORBIDDEN
		} CmiMmPlmnStateEnum;
		typedef struct CmiMmManualPlmnInfo_Tag {
    		UINT8  plmnState;   //CmiMmPlmnStateEnum
    		UINT8  act;         //CmiCregActEnum
    		UINT16 reserved;
    		UINT16 mcc;
		    UINT16 mncWithAddInfo;
		    UINT8  longPlmn[CMI_MM_STR_PLMN_MAX_LENGTH]; // end with '\0'
		    UINT8  shortPlmn[CMI_MM_SHORT_STR_PLMN_MAX_LENGTH]; // end with '\0'
		} CmiMmManualPlmnInfo;   //size = (4+4+32+8) = 48
		typedef struct CmiMmManualPlmnSearchCnf_Tag {
    		UINT8     plmnNum;
    		UINT8     reserved1;
    		UINT16    reserved2;
    		CmiMmManualPlmnInfo plmnList[CMI_MM_PLMN_SEARCH_NUM]; // size = (4+4+32+8)*10 = 480, max 10 PLMNs, if > 10, cut;
		} CmiMmManualPlmnSearchCnf;*/
		case CMI_MM_MANUAL_PLMN_SEARCH_CNF: {
		    LUAT_DEBUG_PRINT("PLMN list index = %d, status = ", index, status);
		    /*CmiMmManualPlmnSearchCnf * plmn_rec = (CmiMmManualPlmnSearchCnf *)param;
            LUAT_DEBUG_PRINT("PLMN list %d", plmn_rec->plmnNum);
        	if (plmn_list_buffer) {        		
        		mp_obj_list_append(plmn_list_buffer, modcellular_plmn_from_list_record(plmn_rec));
    		} else network_exception = NTW_EXC_PLMN_LIST;
		    if(sms_rec->endStatus == 1) plmn_list_flag = 1;
		    */
        	}
			break;

		default:
		    LUAT_DEBUG_PRINT("No handler: event=%d, status=%d", event, status);
			break;
	}
}


void modcellular_init0(void) {
    // Reset callbacks
    network_status_callback = mp_const_none;
    sms_callback = mp_const_none;
    ussd_callback = mp_const_none;

    // Reset statuses
    network_exception = NTW_NO_EXC;

    luat_sms_init();
    luat_sms_recv_msg_register_handler(modcellular_sms_recv_cb); 
    luat_sms_send_msg_register_handler(modcellular_sms_send_cb); 
    luat_mobile_sms_event_register_handler(sms_event_cb); // redefine SMS callback, inited in "luat_sms_init"
   	luat_mobile_event_register_handler(mobile_event_cb); 
	modcellular_init_sms();

    luat_mobile_set_period_work(90000, 5000, 4);
	luat_mobile_set_check_network_period(120000);
	luat_mobile_set_sim_id(2);
	luat_mobile_set_sim_detect_sim0_first();	
}


// Register
// not supported, TODO receive HAL_WAKEUP_2 message 

STATIC mp_obj_t modcellular_get_signal_quality(void) {
    // ========================================
    // Retrieves the network signal quality.
    // Returns:
    //     Two integers: quality
    // ========================================
    mp_obj_t tuple[3] = {
        network_signal_quality == 31 ? mp_const_none : mp_obj_new_int(network_signal_quality),
        network_signal_rx_level == -999 ? mp_const_none : mp_obj_new_int(network_signal_rx_level),
        network_signal_snr == 0 ? mp_const_none : mp_obj_new_int(network_signal_snr)
    };
    return mp_obj_new_tuple(3, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_get_signal_quality_obj, modcellular_get_signal_quality);

STATIC mp_obj_t modcellular_get_imei(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Retrieves IMEI number.
    // Args:
    //     index: SIM index (0 or 1)
    // Returns:
    //     IMEI number as a string.
    // ========================================
    int index = 0;
    if(n_args == 1) index = mp_obj_get_int(args[0]);
    char imei[16];
    memset(imei,0,sizeof(imei));
    int res = luat_mobile_get_imei(index, imei, sizeof(imei));
    if (res > 0) return mp_obj_new_str(imei, strlen(imei));
    else return mp_obj_new_str("", 0);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_get_imei_obj, 0, 1, modcellular_get_imei);

STATIC mp_obj_t modcellular_is_sim_present(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Checks whether the SIM card is inserted and ICCID can be retrieved.
    // Args:
    //     index: SIM index (0 or 1)
    // Returns:
    //     True if SIM present.
    // ========================================
    int index = 0;
    if(n_args == 1) index = mp_obj_get_int(args[0]);
    int res = luat_mobile_get_sim_ready(index);
    if(res == 1) return mp_obj_new_bool(true);
    else return mp_obj_new_bool(false);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_is_sim_present_obj, 0, 1, modcellular_is_sim_present);


STATIC mp_obj_t modcellular_poll_network_exception(void) {
    // ========================================
    // Raises a last network exception.
    // ========================================
    uint8_t e = network_exception;
    network_exception = NTW_NO_EXC;

    switch (e) {
        case NTW_NO_EXC:
            break;

        default:
            mp_raise_OSError(e);
            break;

    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_poll_network_exception_obj, modcellular_poll_network_exception);

STATIC mp_obj_t modcellular_get_network_status(void) {
    // ========================================
    // Retrieves the network status.
    // Returns:
    //     Network status as an integer.
    // ========================================
    return mp_obj_new_int(network_status);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_get_network_status_obj, modcellular_get_network_status);

STATIC mp_obj_t modcellular_is_network_registered(void) {
    // ========================================
    // Checks whether registered on the cellular network.
    // Returns:
    //     True if registered.
    // ========================================
    return mp_obj_new_bool(network_status & NTW_REG_BIT);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_is_network_registered_obj, modcellular_is_network_registered);

STATIC mp_obj_t modcellular_is_roaming(void) {
    // ========================================
    // Checks whether registered on the roaming network.
    // Returns:
    //     True if roaming.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    return mp_obj_new_bool(network_status & NTW_ROAM_BIT);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_is_roaming_obj, modcellular_is_roaming);

STATIC mp_obj_t modcellular_get_iccid(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Retrieves ICCID number.
    // Args:
    //     index: SIM index (0 or 1)
    // Returns:
    //     ICCID number as a string.
    // ========================================
    int index = 0;
    if(n_args == 1) index = mp_obj_get_int(args[0]);
    char iccid[21];
    memset(iccid, 0, sizeof(iccid));
    int res = luat_mobile_get_iccid(index, iccid, sizeof(iccid));
    if(res > 0) return mp_obj_new_str(iccid, strlen(iccid));
    else {
    	mp_raise_RuntimeError("No ICCID data available");
    	return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_get_iccid_obj, 0, 1, modcellular_get_iccid);


STATIC mp_obj_t modcellular_get_imsi(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Retrieves IMSI number.
    // Args:
    //     index: SIM index (0 or 1)
    // Returns:
    //     IMSI number as a string.
    // ========================================
    int index = 0;
    if(n_args == 1) index = mp_obj_get_int(args[0]);
    char imsi[21];
    memset(imsi, 0, sizeof(imsi));
    int res = luat_mobile_get_imsi(index, imsi, sizeof(imsi));
    if(res > 0) return mp_obj_new_str(imsi, strlen(imsi));
    else {
    	mp_raise_RuntimeError("No IMSI data available");
    	return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_get_imsi_obj, 0, 1, modcellular_get_imsi);

bool get_flight_mode(int index) {
    int res = luat_mobile_get_flymode(index);
    if (res < 0) mp_raise_RuntimeError("Failed to retrieve flight mode status");
    return res == 0;
}

STATIC mp_obj_t modcellular_flight_mode(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Retrieves and switches the flight mode
    // status.
    // Args:
    //      mode: flight mode (1 - on, 0 - off)
    // 		index (optional): SIM index, 
    // Returns:
    //     The new flight mode status.
    // ========================================
    int index = 0;
    if (n_args > 0) {
        if (n_args == 2) index = mp_obj_get_int(args[1]);
        mp_int_t set_flag = mp_obj_get_int(args[0]);
        int res = luat_mobile_set_flymode(index, set_flag);
        if(res != 0){
        	mp_raise_RuntimeError("Failed to set flight mode status");
            return mp_const_none;
        }
        WAIT_UNTIL(set_flag == get_flight_mode(index), TIMEOUT_FLIGHT_MODE, 100, mp_raise_OSError(MP_ETIMEDOUT));
    }
    return mp_obj_new_bool(get_flight_mode(index));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_flight_mode_obj, 0, 2, modcellular_flight_mode);

STATIC mp_obj_t modcellular_set_bands(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Sets 4G bands the module operates at.
    // Args:
    //     bands (int): a mask specifying
    //     bands;
    // ========================================
    if (n_args == 0 || (n_args == 1 && mp_obj_get_int(args[0]) == BANDS_ALL)) {
    	// uint8_t default_bands[32];
		// uint8_t default_count;
		// get all supported bands (for info only)
		// luat_mobile_get_band(default_bands, &default_count);
		// for(int i = 0; i < default_count; i++) LUAT_DEBUG_PRINT("Use band %d", default_bands[i]);
		uint8_t default_bands[] = {
			#ifdef NETWORK_FREQ_BAND_1
    		1,
    		#endif
    		#ifdef NETWORK_FREQ_BAND_3
    		3,
    		#endif
    		#ifdef NETWORK_FREQ_BAND_5
    		5, 
    		#endif
    		#ifdef NETWORK_FREQ_BAND_8
			8,
    		#endif
    		#ifdef NETWORK_FREQ_BAND_34
    		34,
    		#endif
    		#ifdef NETWORK_FREQ_BAND_38
    		38,
	    	#endif
    		#ifdef NETWORK_FREQ_BAND_39
    		39,
    		#endif
    		#ifdef NETWORK_FREQ_BAND_40
    		40, 
    		#endif
    		#ifdef NETWORK_FREQ_BAND_41
			41,
    		#endif
    	};
		// set all bands
    	int res = luat_mobile_set_band(default_bands, sizeof(default_bands));
    	if(res != 0) {
            mp_raise_RuntimeError("Failed to reset bands");
            return mp_const_none;
        }
    } else if (n_args == 1) {
		uint8_t bands[1];
		bands[0] = mp_obj_get_int(args[0]);
    	int res = luat_mobile_set_band(bands, 1);
        if(res != 0) {
            mp_raise_RuntimeError("Failed to set bands");
            return mp_const_none;
        }
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_set_bands_obj, 0, 1, modcellular_set_bands);

/*
STATIC mp_obj_t modcellular_gprs(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Polls and switches GPRS status.
    // Args:
    //     apn (str, bool): access point name
    //     or False if GPRS shutdown requested.
    //     user (str): username;
    //     pass (str): password;
    //     timeout (int): time to wait until
    //     connected;
    // Returns:
    //     True if GPRS is active, False
    //     otherwise.
    // ========================================
    REQUIRES_NETWORK_REGISTRATION; // checks network_status

    if (n_args == 1 || n_args == 2) {
        mp_int_t flag = mp_obj_get_int(args[0]);
        if (flag != 0) {
            mp_raise_ValueError("Unkown integer argument supplied, zero (or False) expected");
            return mp_const_none;
        }
        mp_int_t timeout = TIMEOUT_GPRS_ACTIVATION;
        if (n_args == 2) timeout = mp_obj_get_int(args[1]);

        if (network_status & NTW_ACT_BIT) {
            if (!Network_StartDeactive(1)) {
                mp_raise_RuntimeError("Cannot initiate context deactivation");
                return mp_const_none;
            }
            WAIT_UNTIL(!(network_status & NTW_ACT_BIT), timeout, 100, mp_raise_OSError(MP_ETIMEDOUT));
        }

        // !!!!!!!!!!!!!!!!!!!!!!!
        if (network_status & NTW_ATT_BIT)
            if (Network_StartDetach())
                WAIT_UNTIL(!(network_status & NTW_ATT_BIT), TIMEOUT_GPRS_ATTACHMENT, 100, break);
        // !!!!!!!!!!!!!!!!!!!!!!!

    } else if (n_args == 3 || n_args == 4) {
        const char* c_apn = mp_obj_str_get_str(args[0]);
        const char* c_user = mp_obj_str_get_str(args[1]);
        const char* c_pass = mp_obj_str_get_str(args[2]);
        mp_int_t timeout = TIMEOUT_GPRS_ACTIVATION;
        if (n_args == 4) timeout = mp_obj_get_int(args[3]);

        if (network_status & NTW_ACT_BIT) {
            mp_raise_ValueError("GPRS is already on");
            return mp_const_none;
        }
        WAIT_UNTIL(__is_attached(), TIMEOUT_GPRS_ATTACHMENT, 100, mp_raise_RuntimeError("Network is not attached: try resetting"));

        if (!(network_status & NTW_ACT_BIT)) {
            Network_PDP_Context_t context;
            memcpy(context.apn, c_apn, MIN(strlen(c_apn) + 1, sizeof(context.apn)));
            memcpy(context.userName, c_user, MIN(strlen(c_user) + 1, sizeof(context.userName)));
            memcpy(context.userPasswd, c_pass, MIN(strlen(c_pass) + 1, sizeof(context.userPasswd)));

            if (!Network_StartActive(context))
                mp_raise_RuntimeError("Cannot initiate context activation");
            WAIT_UNTIL(network_status & NTW_ACT_BIT, timeout, 100, mp_raise_OSError(MP_ETIMEDOUT));
        }

    } else if (n_args != 0) {
        mp_raise_ValueError("Unexpected number of argument: 0, 1 or 3 required");
    }

    return mp_obj_new_bool(network_status & NTW_ACT_BIT);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modcellular_gprs_obj, 0, 4, modcellular_gprs);
*/

mp_obj_t modcellular_plmn_from_list_record(CmiMmManualPlmnSearchCnf *record) {
    return mp_const_none;
}


void modcellular_scan_cb(uint16_t param_size, void* p_param) { 
	//CmiMmManualPlmnSearchReq req = {0};
	//req.gardTimer = TIMEOUT_LIST_OPERATORS / 1000;
	//psCamCmiReq(PS_DIAL_REQ_HANDLER, CAM_MM, CMI_MM_MANUAL_PLMN_SEARCH_REQ, sizeof(CmiMmManualPlmnSearchReq), (CmiMmManualPlmnSearchReq*)&req);
	mmManualPlmnSearch(PS_DIAL_REQ_HANDLER, TIMEOUT_LIST_OPERATORS / 1000);
}
void modcellular_clear_scan_list() {
    mp_obj_list_t *self = MP_OBJ_TO_PTR(plmn_list_buffer);
    self->len = 0;
    self->items = m_renew(mp_obj_t, self->items, self->alloc, LIST_MIN_ALLOC);
    self->alloc = LIST_MIN_ALLOC;
    mp_seq_clear(self->items, 0, self->alloc, sizeof(*self->items));
}


STATIC mp_obj_t modcellular_scan(void) {
    // ========================================
    // Lists network operators.
    // ========================================
    plmn_list_flag = 0;
    if(plmn_list_buffer == NULL) plmn_list_buffer = mp_obj_new_list(0, NULL);
    if(plmn_list_buffer != NULL) {
    	cmsNonBlockApiCall(modcellular_scan_cb, 0, NULL);
    	WAIT_UNTIL(plmn_list_flag, TIMEOUT_LIST_OPERATORS, 100, mp_raise_RuntimeError("Can not get PLMN list");); // wait for CMI_MM_MANUAL_PLMN_SEARCH_CNF
    }    

    //mp_obj_t items[network_list_buffer_len];
    mp_obj_t plmn_list = mp_obj_new_list(0, NULL);
    for (int i = 0; i < plmn_list_buffer->len; i++) {
        // Name
        /*uint8_t *op_name;
        if (!Network_GetOperatorNameById(network_list_buffer[i].operatorId, &op_name)) {
            mp_raise_RuntimeError("Failed to poll operator name");
            return mp_const_none;
        }*/

        /*mp_obj_t tuple[3] = {
            mp_obj_new_bytearray(sizeof(plmn_list[i].operatorId), network_list_buffer[i].operatorId),
            mp_obj_new_str((char*) op_name, strlen((char*) op_name)),
            mp_obj_new_int(network_list_buffer[i].status),
        };
        items[i] = mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
        mp_obj_list_append(plmn_list, mp_obj_new_tuple(3, tuple));
        */
    }
    modcellular_clear_scan_list();

    return plmn_list;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_scan_obj, modcellular_scan);


STATIC int modcellular_convert_MCCMNC(int code) {
    // convert to hex and use as int
    char str [10];
	itoa(code, str, 16);
	return atoi(str);
}

STATIC mp_obj_t modcellular_stations(void) {
    // ========================================
    // Returns base stations cells.
    // ========================================
    luat_mobile_cell_info_t info = {0};
    if (luat_mobile_get_cell_info(&info) != 0) {
        mp_raise_RuntimeError("Failed to poll base stations");
        return mp_const_none;
    }

 	mp_obj_t cell_list = mp_obj_new_list(0, NULL);

    //LUAT_DEBUG_PRINT("gsm_info_valid = %d", info.gsm_info_valid);
    //LUAT_DEBUG_PRINT("gsm_neighbor_info_num = %d", info.gsm_neighbor_info_num);
    //LUAT_DEBUG_PRINT("lte_info_valid = %d", info.lte_info_valid);
    //LUAT_DEBUG_PRINT("lte_neighbor_info_num = %d", info.lte_neighbor_info_num);
    if (info.lte_info_valid) {
	    luat_mobile_lte_service_cell_info_t cell = info.lte_service_info;
    	mp_obj_t tuple[8] = {
        	mp_obj_new_int(modcellular_convert_MCCMNC(cell.mcc)),
            mp_obj_new_int(modcellular_convert_MCCMNC(cell.mnc)),
            mp_obj_new_int(cell.tac),
	        mp_obj_new_int(cell.cid),
    	    mp_obj_new_int(cell.pci),
        	mp_obj_new_int(cell.rsrq),
            mp_obj_new_int(cell.snr),
	        mp_obj_new_int(cell.earfcn),
    	};
    	//LUAT_DEBUG_PRINT("serving cell: mcc=%d, mnc=%d, tac=%d, cid=%d, rsrq=%d, snr=%d, earfcn=%d", cell.mcc, cell.mnc, cell.tac, cell.cid, cell.rsrq, cell.snr, cell.earfcn);
    	mp_obj_list_append(cell_list, mp_obj_new_tuple(8, tuple));

    	for (int i = 0; i < info.lte_neighbor_info_num; i++) {
    	    luat_mobile_lte_cell_info_t ncell = info.lte_info[i];
        	mp_obj_t tuple[8] = {
            	mp_obj_new_int(modcellular_convert_MCCMNC(ncell.mcc)),
            	mp_obj_new_int(modcellular_convert_MCCMNC(ncell.mnc)),
	            mp_obj_new_int(ncell.tac),
		        mp_obj_new_int(ncell.cid),
    		    mp_obj_new_int(ncell.pci),
        		mp_obj_new_int(ncell.rsrq),
	            mp_obj_new_int(ncell.snr),
		        mp_obj_new_int(ncell.earfcn),
    	    };
    	    //LUAT_DEBUG_PRINT("neighbour cell[%d]: mcc=%d, mnc=%d, tac=%d, cid=%d, rsrq=%d, snr=%d, earfcn=%d", i, ncell.mcc, ncell.mnc, ncell.tac, ncell.cid, ncell.rsrq, ncell.snr, ncell.earfcn);
        	mp_obj_list_append(cell_list, mp_obj_new_tuple(8, tuple));
       	}
    }
    return cell_list;
    
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_stations_obj, modcellular_stations);

STATIC mp_obj_t modcellular_reset(void) {
    // ========================================
    // Resets network settings to defaults.
    // Note:
    //     Conflicts with setting flight mode. 
    //     Only one can be used within a certain period of time.
    // ========================================    
    luat_mobile_reset_stack();
    modcellular_init0();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modcellular_reset_obj, modcellular_reset);

STATIC mp_obj_t modcellular_on_status_event(mp_obj_t callable) {
    // ========================================
    // Sets a callback on status event.
    // Args:
    //     callback (Callable): a callback to
    //     execute on status event.
    // ========================================
    network_status_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modcellular_on_status_event_obj, modcellular_on_status_event);


STATIC const mp_map_elem_t mp_module_cellular_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_cellular) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_SMS), (mp_obj_t)MP_ROM_PTR(&modcellular_sms_type) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_get_imei), (mp_obj_t)&modcellular_get_imei_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_signal_quality), (mp_obj_t)&modcellular_get_signal_quality_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_poll_network_exception), (mp_obj_t)&modcellular_poll_network_exception_obj },
    
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_network_status), (mp_obj_t)&modcellular_get_network_status_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_sim_present), (mp_obj_t)&modcellular_is_sim_present_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_network_registered), (mp_obj_t)&modcellular_is_network_registered_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_roaming), (mp_obj_t)&modcellular_is_roaming_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_iccid), (mp_obj_t)&modcellular_get_iccid_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_imsi), (mp_obj_t)&modcellular_get_imsi_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_flight_mode), (mp_obj_t)&modcellular_flight_mode_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_bands), (mp_obj_t)&modcellular_set_bands_obj },
    //{ MP_OBJ_NEW_QSTR(MP_QSTR_gprs), (mp_obj_t)&modcellular_gprs_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_scan), (mp_obj_t)&modcellular_scan_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ussd), (mp_obj_t)&modcellular_ussd_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_stations), (mp_obj_t)&modcellular_stations_obj },
    //{ MP_OBJ_NEW_QSTR(MP_QSTR_agps_station_data), (mp_obj_t)&modcellular_agps_station_data_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset), (mp_obj_t)&modcellular_reset_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_status_event), (mp_obj_t)&modcellular_on_status_event_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_sms), (mp_obj_t)&modcellular_on_sms_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_ussd), (mp_obj_t)&modcellular_on_ussd_obj },

    #ifdef NETWORK_FREQ_BAND_1
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_1),  MP_ROM_INT(1)  },
    #endif
    #ifdef NETWORK_FREQ_BAND_3
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_3),  MP_ROM_INT(3)  },
    #endif
    #ifdef NETWORK_FREQ_BAND_5
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_5),  MP_ROM_INT(5)  },
    #endif
    #ifdef NETWORK_FREQ_BAND_8
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_8),  MP_ROM_INT(8)  },
    #endif
    #ifdef NETWORK_FREQ_BAND_34
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_34), MP_ROM_INT(34) },
    #endif
    #ifdef NETWORK_FREQ_BAND_38
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_38), MP_ROM_INT(38) },
    #endif
    #ifdef NETWORK_FREQ_BAND_39
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_39), MP_ROM_INT(39) },
    #endif
    #ifdef NETWORK_FREQ_BAND_40
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_40), MP_ROM_INT(40) },
    #endif
    #ifdef NETWORK_FREQ_BAND_41
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BAND_41), MP_ROM_INT(41) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_NETWORK_FREQ_BANDS_ALL), MP_ROM_INT(BANDS_ALL) },

    // { MP_ROM_QSTR(MP_QSTR_SMS_SENT), MP_ROM_INT(SMS_SENT) },

    { MP_ROM_QSTR(MP_QSTR_ENOSIM), MP_ROM_INT(NTW_EXC_NOSIM) },
    { MP_ROM_QSTR(MP_QSTR_EREGD), MP_ROM_INT(NTW_EXC_REG_DENIED) },
    { MP_ROM_QSTR(MP_QSTR_ESMSSEND), MP_ROM_INT(NTW_EXC_SMS_SEND) },
    { MP_ROM_QSTR(MP_QSTR_ESMSDROP), MP_ROM_INT(NTW_EXC_SMS_DROP) },
    { MP_ROM_QSTR(MP_QSTR_ESIMDROP), MP_ROM_INT(NTW_EXC_SIM_DROP) },
    { MP_ROM_QSTR(MP_QSTR_EATTACHMENT), MP_ROM_INT(NTW_EXC_ATT_FAILED) },
    { MP_ROM_QSTR(MP_QSTR_EACTIVATION), MP_ROM_INT(NTW_EXC_ACT_FAILED) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_delete_by_index), (mp_obj_t)&modcellular_sms_delete_by_index_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_delete_all_read), (mp_obj_t)&modcellular_sms_delete_all_read_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_read_all), (mp_obj_t)&modcellular_sms_read_all_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_list_read), (mp_obj_t)&modcellular_sms_list_read_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_list), (mp_obj_t)&modcellular_sms_list_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sms_get_storage_size), (mp_obj_t)&modcellular_sms_get_storage_size_obj },
    
};

STATIC MP_DEFINE_CONST_DICT(mp_module_cellular_globals, mp_module_cellular_globals_table);

const mp_obj_module_t cellular_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_cellular_globals,
};

MP_REGISTER_MODULE(MP_QSTR_cellular, cellular_module);