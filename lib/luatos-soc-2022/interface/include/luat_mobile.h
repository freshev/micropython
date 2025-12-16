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

#ifndef __LUAT_MOBILE_H__
#define __LUAT_MOBILE_H__

#include "luat_base.h"
/**
 * @defgroup luatos_mobile mobile network related interfaces
 * @{*/

/**
 * @brief Get IMEI
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param buff[OUT] IMEI data
 * @param buf_len The size of the cache passed by the user. If the amount of underlying data is greater than buf_len, only data of the size of buf_len will be sent out.
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_get_imei(int sim_id, char* buff, size_t buf_len);

/**
 * @brief Get the SN. If the user has not written the SN by calling the luat_mobile_set_sn interface, the default value is empty.
 *
 * @param buff[OUT] SN data
 * @param buf_len The size of the cache passed by the user. The maximum length supported by the bottom layer of the EC618 platform is 32 bytes. If the amount of bottom data is greater than buf_len, only data of the size of buf_len will be sent out.
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_get_sn(char* buff, size_t buf_len);

/**
 * @brief Set SN
 *
 * @param buff SN data, must be a visible ascii character with an ascii value greater than or equal to 0x21 and less than or equal to 0x7e
 * @param buf_len SN data length; the maximum length supported by the bottom layer of the EC618 platform is 32 bytes. If buf_len is greater than 32, only the first 32 bytes of data will be saved.
 * @return int = 0 success, = -1 failure*/
int luat_mobile_set_sn(char* buff, uint8_t buf_len);

/**
 * @brief Get the MUID, which does not necessarily exist
 *
 * @param buff[OUT] MUID data
 * @param buf_len The size of the cache passed by the user. If the amount of underlying data is greater than buf_len, only data of the size of buf_len will be sent out.
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_get_muid(char* buff, size_t buf_len);

/**
 * @brief Get the ICCID of the SIM card
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param buff[OUT] ICCID data
 * @param buf_len The size of the cache passed by the user. If the amount of underlying data is greater than buf_len, only data of the size of buf_len will be sent out.
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_get_iccid(int sim_id, char* buff, size_t buf_len);

/**
 * @brief Get the IMSI of the SIM card
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param buff[OUT] IMSI data
 * @param buf_len The size of the cache passed by the user. If the amount of underlying data is greater than buf_len, only data of the size of buf_len will be sent out.
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_get_imsi(int sim_id, char* buff, size_t buf_len);

/**
 * @brief The mobile phone number of the currently used SIM card. Note that it can only be read after the mobile phone number is written, so it may be empty when read.
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param buff[OUT] sim_number data
 * @param buf_len The size of the cache passed by the user. If the amount of underlying data is greater than buf_len, only data of the size of buf_len will be sent out.
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_get_sim_number(int sim_id, char* buff, size_t buf_len);

/**
 * @brief The location of the current SIM card is not necessarily supported.
 *
 * @param id[OUT] sim position, for dual-SIM dual-standby devices, output 0 or 1, other devices output 0
 * @return int =0 success, others failure*/
int luat_mobile_get_sim_id(int *id);

/**
 * @brief Changing the location of the SIM card used is not necessarily supported
 *
 * @param id sim position. For dual-SIM devices, select 0 or 1. Others are in automatic selection mode, but the priorities of 0 and 1 are the same. Non-dual SIM devices are not supported
 * @return int =0 success, others failure*/
int luat_mobile_set_sim_id(int id);


typedef enum LUAT_MOBILE_SIM_PIN_OP
{
    LUAT_SIM_PIN_VERIFY = 0,       /* verify pin code */
	LUAT_SIM_PIN_CHANGE,       /* change pin code */
	LUAT_SIM_PIN_ENABLE,       /* enable the pin1 state for verification */
	LUAT_SIM_PIN_DISABLE,      /* disable the pin1 state */
	LUAT_SIM_PIN_UNBLOCK,      /* unblock pin code */
//	LUAT_SIM_PIN_QUERY,        /* query pin1 state, enable or disable */
}LUAT_MOBILE_SIM_PIN_OP_E;

/**
 * @brief Operate the pin code of the SIM card
 *
 * @param id sim position. For dual-SIM devices, select 0 or 1. Others are in automatic selection mode, but the priorities of 0 and 1 are the same. Non-dual SIM devices are not supported
 * @param operation operation code, see LUAT_MOBILE_SIM_PIN_OP_E
 * @param pin1 old pin code, or verified pin code, PUK when unlocking the pin code, refer to the mobile phone operation, when not in use, write 0 in the first byte
 * @param pin2 The new pin code when changing the pin code operation, the new PIN when unlocking the pin code, refer to the mobile phone operation, when not in use, write 0 in the first byte
 * @return int =0 success, others failure*/
int luat_mobile_set_sim_pin(int id, uint8_t operation, char pin1[9], char pin2[9]);

/**
 * @brief Choose to enable softSim or physical card. The kv function is unavailable after enabling softSim.
 *
 * @param enable Select 1 to enable softSim, 0 to enable the physical card
 * @return int =0 success, others failure*/
int luat_mobile_soft_sim_switch(uint8_t enable);

/**
 * @brief Get the current sim configuration whether it is softSim or physical card
 *
 * @param is_soft_sim uint8_t pointer, when the return value is 0, this value is meaningful, 1 is a virtual card, 0 is a physical card
 * @return int =0 success, others failure*/
int luat_mobile_get_soft_sim_cfg(uint8_t *is_soft_sim);

/**
 * @brief Check if the SIM card is ready
 *
 * @param id sim position. For dual-SIM devices, select 0 or 1. Others are in automatic selection mode, but the priorities of 0 and 1 are the same. Non-dual SIM devices are not supported
 * @return =1 is ready, others are not ready, or the SIM card is not in place*/
uint8_t luat_mobile_get_sim_ready(int id);
/**
 * @brief When automatically selecting mode, sim0 will be used first after booting.
 **/
void luat_mobile_set_sim_detect_sim0_first(void);


/**
 * @brief Set whether the IPV6 network is required when the default PDN is activated. It is not enabled by default now.
 * @param onoff 1 on 0 off
 * @return uint8_t 1 on 0 off*/
void luat_mobile_set_default_pdn_ipv6(uint8_t onoff);

/**
 * @brief Returns whether an IPV6 network is required when the default PDN is activated
 * @return uint8_t 1 on 0 off*/
uint8_t luat_mobile_get_default_pdn_ipv6(void);

/**
 * @brief Get the configured apn name, which may not be supported
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param cid cid position 1~6
 * @param buff[OUT] apn name
 * @param buf_len The size of the cache passed by the user. If the amount of underlying data is greater than buf_len, only data of the size of buf_len will be sent out.
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_get_apn(int sim_id, int cid, char* buff, size_t buf_len);

/**
 * @brief User controls the APN activation process. Only after using this function can the user's APN be manually activated and a network card installed.*/
void luat_mobile_user_ctrl_apn(void);

/**
 * @brief Manually set the minimum information required for APN activation. If you need more detailed settings, you can modify this function yourself.
 * @attention needs to be used in the mobile_event callback. For specific usage, please refer to example_apn demo
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param cid cid position 2~6
 * @param type activation type 1 IPV4 2 IPV6 3 IPV4V6
 * @param apn_name apn name
 * @param name_len apn name length
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_set_apn_base_info(int sim_id, int cid, uint8_t type, uint8_t* apn_name, uint8_t name_len);


/**
 * @brief Manually set the encryption information required for APN activation. If you need more detailed settings, you can modify this function yourself. Encrypted information is not required in most cases, orientation cards may require
 * @attention needs to be used in the mobile_event callback. For specific usage, please refer to example_apn demo
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param cid cid position 2~6
 * @param protocol encryption protocol 0~2, 0xff means not required
 * @param user_name username
 * @param user_name_len username length
 * @param password password
 * @param password_len password length
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_set_apn_auth_info(int sim_id, int cid, uint8_t protocol, uint8_t *user_name, uint8_t user_name_len, uint8_t *password, uint8_t password_len);


/**
 * @brief Manually activate/deactivate APN
 * @attention needs to be used in the mobile_event callback. Deactivation does not need to be used in mobile_event. For specific usage, please refer to example_apn demo.
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param cid cid position 2~6
 * @param state 1 activation 0 deactivation, deactivation can only be done in the user's own tasks
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_active_apn(int sim_id, int cid, uint8_t state);

/**
 * @brief Manually activate the network card
 * @attention needs to be used in the mobile_event callback. For specific usage, please refer to example_apn demo
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param cid cid position 2~6
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_active_netif(int sim_id, int cid);

/**
 * @brief The user sets the basic information of the APN and activates it automatically. Note that it cannot be shared with the above-mentioned manual APN API. If the private network card cannot be activated with the public network apn and the default bearer must be used, this must be used.
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param cid cid position 1~6
 * @param ip_type activation type 1 IPV4 2 IPV6 3 IPV4V6
 * @param protocol_type encryption protocol 0=not required 1=PPP_LCP_PAP 2=PPP_LCP_CHAP 3=try both 1 and 2
 * @param apn_name apn name, if left blank, the default APN will be used
 * @param apn_name_len apn name length
 * @param user_name username
 * @param user_name_len username length
 * @param password password
 * @param password_len password length
 * @return none*/
void luat_mobile_user_apn_auto_active(int sim_id, uint8_t cid,
		uint8_t ip_type,
		uint8_t protocol_type,
		uint8_t *apn_name, uint8_t apn_name_len,
		uint8_t *user, uint8_t user_len,
		uint8_t *password, uint8_t password_len);

/**
 * @brief Get the apn name of the default CID, which is not necessarily supported
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param buff[OUT] apn name
 * @param buf_len The size of the cache passed by the user. If the amount of underlying data is greater than buf_len, only data of the size of buf_len will be sent out.
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mobile_get_default_apn(int sim_id, char* buff, size_t buf_len);

/**
 * @brief Delete the defined apn
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param cid cid position 1~6
 * @param is_dedicated Whether it is dedicated, if not clear, write 0
 * @return int =0 success, others failure*/
int luat_mobile_del_apn(int sim_id, uint8_t cid, uint8_t is_dedicated);

/**
 * @brief Switch in and out of airplane mode
 *
 * @param index sim position, for dual-SIM dual-standby devices, select 0 or 1, other devices are free
 * @param mode Airplane mode, 1 to enter, 0 to exit
 * @return int =0 success, others failure*/
int luat_mobile_set_flymode(int index, int mode);

/**
 * @brief current status of airplane mode
 *
 * @param index sim position, for dual-SIM dual-standby devices, select 0 or 1, other devices are free
 * @return int <0 Abnormal =0 Airplane mode =1 Normal working =4 RF off*/
int luat_mobile_get_flymode(int index);

#if (!defined __LUATOS__) || ((defined __LUATOS__) && (defined LUAT_USE_LWIP))
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/inet.h"

/**
 * @brief Get the local IP address of the activated bearer allocation
 *
 * @param sim_id sim location, for dual-SIM dual-standby devices, select 0 or 1, other devices are optional
 * @param cid cid position 1~6, if you do not use private network APN, use 1
 * @param ip_v4, IP address of ipv4
 * @param ip_v6, IP address of ipv6
 * @return int =0 success, others failure*/
int luat_mobile_get_local_ip(int sim_id, int cid, ip_addr_t *ip_v4, ip_addr_t *ip_v6);
#endif
/* -------------------------------------------------- cell info begin -------------------------------------------------- */
#define LUAT_MOBILE_CELL_MAX_NUM 9

typedef struct luat_mobile_gsm_service_cell_info
{
    int cid;        /**< Cell ID, (0 indicates information is not represent).*/
    int mcc;        /**< This field should be ignored when cid is not present*/
    int mnc;        /**< This field should be ignored when cid is not present*/
    int lac;        /**< Location area code.(This field should be ignord when cid is not present). */
    int arfcn;      /**< Absolute RF channel number. */
    int bsic;       /**< Base station identity code. (0 indicates information is not present). */
	int rssi;		/**< Receive signal strength, Value range: rxlev-111 for dbm format */
}luat_mobile_gsm_service_cell_info_t;

typedef struct luat_mobile_gsm_cell_info
{
    int cid;        /**Cell ID, (0 indicates information is not represent).*/
    int mcc;        /**This field should be ignored when cid is not present*/
    int mnc;        /**This field should be ignored when cid is not present*/
    int lac;        /**Location area code.(This field should be ignord when cid is not present). */
    int arfcn;      /**Absolute RF channel number. */
    int bsic;       /**Base station identity code. (0 indicates information is not present). */
	int rssi;		/**< Receive signal strength, Value range: rxlev-111 for dbm format */
}luat_mobile_gsm_cell_info_t;

typedef struct luat_mobile_lte_service_cell_info
{
    uint32_t cid;           /**<Cell ID, (0 indicates information is not represent).*/
    uint16_t mcc;           /**This field should be ignored when cid is not present*/
    uint16_t mnc;           /**This field should be ignored when cid is not present*/
    uint16_t tac;           /**Tracing area code (This field should be ignored when cid is not present). */
    uint16_t pci;           /**Physical cell ID. Range: 0 to 503. */
    uint32_t earfcn;        /**E-UTRA absolute radio frequency channel number of the cell. RANGE: 0 TO 65535. */
    int16_t rssi;		   /**< Receive signal strength, Value range: rsrp-140 for dbm format */
	int16_t rsrp;
	int16_t rsrq;
	int16_t snr;
	uint8_t is_tdd;
	uint8_t band;
	uint8_t ulbandwidth;
	uint8_t dlbandwidth;
}luat_mobile_lte_service_cell_info_t;

typedef struct luat_mobile_lte_cell_info
{
	uint32_t cid;           /**<Cell ID, (0 indicates information is not represent).*/
	uint16_t mcc;           /**This field should be ignored when cid is not present*/
	uint16_t mnc;           /**This field should be ignored when cid is not present*/
	uint16_t tac;           /**Tracing area code (This field should be ignored when cid is not present). */
	uint16_t pci;           /**Physical cell ID. Range: 0 to 503. */
    uint32_t earfcn;        /**E-UTRA absolute radio frequency channel number of the cell. RANGE: 0 TO 65535. */
    int16_t rsrp;
	int16_t rsrq;
	int16_t snr;
}luat_mobile_lte_cell_info_t;

typedef struct luat_mobile_cell_info
{
    luat_mobile_gsm_service_cell_info_t gsm_service_info;					/**<   GSM cell information (Serving). */
    luat_mobile_gsm_cell_info_t    gsm_info[LUAT_MOBILE_CELL_MAX_NUM];    /**<   GSM cell information (neighbor). */
    luat_mobile_lte_service_cell_info_t lte_service_info;					/**<   LTE cell information (Serving). */
    luat_mobile_lte_cell_info_t    lte_info[LUAT_MOBILE_CELL_MAX_NUM];    /**<   LTE cell information (neighbor). */
    uint32_t 						version;
    uint8_t                         gsm_info_valid;                         /**< Must be set to TRUE if gsm_info is being passed. */
    uint8_t                         gsm_neighbor_info_num;                           /**< Must be set to the number of elements in entry*/
    uint8_t                         lte_info_valid;                         /**< Must be set to TRUE if lte_info is being passed. */
    uint8_t                     	lte_neighbor_info_num;                           /**< Must be set to the number of elements in entry*/
}luat_mobile_cell_info_t;

/**
 * @brief Immediately search for surrounding cell base station information and return the results synchronously
 *
 * @param info Current mobile network signal status details
 * @return int =0 success, others failure*/
int luat_mobile_get_cell_info(luat_mobile_cell_info_t  *info);

/**
 * @brief Immediately search for surrounding cell base station information, return the search completion message through LUAT_MOBILE_CELL_INFO_UPDATE, and obtain detailed information through luat_mobile_get_last_notify_cell_info
 *
 * @param max_time The maximum time for searching, in seconds
 * @return int =0 success, others failure*/
int luat_mobile_get_cell_info_async(uint8_t max_time);

/**
 * @brief Get the last asynchronous search of surrounding cell base station information, including periodic search and asynchronous search. Use this function to obtain the information after the arrival of LUAT_MOBILE_CELL_INFO_UPDATE
 *
 * @param info Current mobile network signal status details
 * @param max_time The maximum time for searching
 * @return int =0 success, others failure*/
int luat_mobile_get_last_notify_cell_info(luat_mobile_cell_info_t  *info);


typedef struct luat_mobile_gw_signal_strength_info
{
    int rssi;
    int bitErrorRate;
    int rscp;
    int ecno;
}luat_mobile_gw_signal_strength_info_t;

typedef struct luat_mobile_lte_signal_strength_info
{
    int16_t rssi;		   /**< Receive signal strength, Value range: rsrp-140 for dbm format */
	int16_t rsrp;
	int16_t rsrq;
	int16_t snr;
}luat_mobile_lte_signal_strength_info_t;

typedef struct luat_mobile_signal_strength_info
{
    luat_mobile_gw_signal_strength_info_t   gw_signal_strength;
    luat_mobile_lte_signal_strength_info_t  lte_signal_strength;
    uint8_t luat_mobile_gw_signal_strength_vaild;
    uint8_t luat_mobile_lte_signal_strength_vaild;
}luat_mobile_signal_strength_info_t;

/**
 * @brief When converting from RSSI to CSQ, RSSI can only be used as a reference for the antenna port status, but not as a reference for the LTE network signal status.
 *
 * @param rssi RSSI value
 * @return uint8_t CSQ value*/
uint8_t luat_mobile_rssi_to_csq(int8_t rssi);

/**
 * @brief Get current mobile network signal status details
 *
 * @param info Current mobile network signal status details
 * @return int =0 success, others failure*/
int luat_mobile_get_signal_strength_info(luat_mobile_signal_strength_info_t *info);

/**
 * @brief Get the CSQ value CSQ is converted from RSSI and can only be used as a reference for the antenna port status, but not as a reference for the LTE network signal status.
 *
 * @param csq CSQ value
 * @return int =0 success, others failure*/
int luat_mobile_get_signal_strength(uint8_t *csq);

/**
 * @brief Get the network signal status details after the latest network signal status update notification
 *
 * @param info Network signal status details
 * @return int =0 success, others failure*/
int luat_mobile_get_last_notify_signal_strength_info(luat_mobile_signal_strength_info_t *info);

/**
 * @brief Get the CSQ value after the latest network signal status update notification
 *
 * @param info CSQ value
 * @return int =0 success, others failure*/
int luat_mobile_get_last_notify_signal_strength(uint8_t *csq);

/**
 * @brief Get the ECI of the current serving cell
 *
 * @param eci
 * @return int =0 success, others failure*/
int luat_mobile_get_service_cell_identifier(uint32_t *eci);/**

 * @brief Get the TAC or LAC of the current serving cell
 *
 * @param tac
 * @return int =0 success, others failure*/
int luat_mobile_get_service_tac_or_lac(uint16_t *tac);
/* --------------------------------------------------- cell info end --------------------------------------------------- */


/* ------------------------------------------------ mobile status begin ----------------------------------------------- */
/**
 * @brief Message about changes in network status and related functional status
 **/
typedef enum LUAT_MOBILE_EVENT
{
	LUAT_MOBILE_EVENT_CFUN = 0, /**< CFUN message*/
	LUAT_MOBILE_EVENT_SIM, /**< SIM card message*/
	LUAT_MOBILE_EVENT_REGISTER_STATUS,     /**< Mobile network registration message*/
	LUAT_MOBILE_EVENT_CELL_INFO, 	/**< Cell base station information and network signal change message*/
	LUAT_MOBILE_EVENT_PDP, 	/**< PDP status message*/
	LUAT_MOBILE_EVENT_NETIF, 	/**< internet status*/
	LUAT_MOBILE_EVENT_TIME_SYNC, 	/**< Completed through base station time synchronization*/
	LUAT_MOBILE_EVENT_CSCON, /**< RRC status, 0 idle 1 active*/
	LUAT_MOBILE_EVENT_BEARER,/**< PDP bearer status*/
	LUAT_MOBILE_EVENT_SMS,/**< sms message, not needed for air780e*/
	LUAT_MOBILE_EVENT_NAS_ERROR,/**< NAS exception message, valid for air780e, air780ep*/
	LUAT_MOBILE_EVENT_FATAL_ERROR = 0xff,/**< The network encountered a serious failure*/
}LUAT_MOBILE_EVENT_E;

typedef enum LUAT_MOBILE_CFUN_STATUS
{
	LUAT_MOBILE_CFUN_OFF = 0,
	LUAT_MOBILE_CFUN_ON,
	LUAT_MOBILE_CFUN_NO_RF = 4,
}LUAT_MOBILE_CFUN_STATUS_E;

typedef enum LUAT_MOBILE_SIM_STATUS
{
	LUAT_MOBILE_SIM_READY = 0,
	LUAT_MOBILE_NO_SIM,
	LUAT_MOBILE_SIM_NEED_PIN,
	LUAT_MOBILE_SIM_ENTER_PIN_RESULT,
	LUAT_MOBILE_SIM_NUMBER,
	LUAT_MOBILE_SIM_WC
}LUAT_MOBILE_SIM_STATUS_E;


typedef enum LUAT_MOBILE_REGISTER_STATUS
{
	LUAT_MOBILE_STATUS_UNREGISTER,  /**< The network is not registered*/
	LUAT_MOBILE_STATUS_REGISTERED,  /**< The network is registered*/
	LUAT_MOBILE_STATUS_INSEARCH, 	/**< Searching the Internet*/
	LUAT_MOBILE_STATUS_DENIED,  	/**< Online registration rejected*/
	LUAT_MOBILE_STATUS_UNKNOW,		/**<Network status unknown*/
	LUAT_MOBILE_STATUS_REGISTERED_ROAMING, 	/**<Network registered, roaming*/
	LUAT_MOBILE_STATUS_SMS_ONLY_REGISTERED,
	LUAT_MOBILE_STATUS_SMS_ONLY_REGISTERED_ROAMING,
	LUAT_MOBILE_STATUS_EMERGENCY_REGISTERED,
	LUAT_MOBILE_STATUS_CSFB_NOT_PREFERRED_REGISTERED,
	LUAT_MOBILE_STATUS_CSFB_NOT_PREFERRED_REGISTERED_ROAMING,
}LUAT_MOBILE_REGISTER_STATUS_E;

typedef enum LUAT_MOBILE_CELL_INFO_STATUS
{
	LUAT_MOBILE_CELL_INFO_UPDATE = 0,	/**< Cell base station information changes will only occur when periodic search for cell base stations is set.*/
	LUAT_MOBILE_SIGNAL_UPDATE,			/**< The network signal status changes, but it does not necessarily mean there is a change*/
}LUAT_MOBILE_CELL_INFO_STATUS_E;

typedef enum LUAT_MOBILE_PDP_STATUS
{
	LUAT_MOBILE_PDP_ACTIVED = 0,
	LUAT_MOBILE_PDP_DEACTIVING,
	LUAT_MOBILE_PDP_DEACTIVED,
}LUAT_MOBILE_PDP_STATUS_E;

typedef enum LUAT_MOBILE_NETIF_STATUS
{
	LUAT_MOBILE_NETIF_LINK_ON = 0, /**< Connected to the Internet*/
	LUAT_MOBILE_NETIF_LINK_OFF,	/**< Disconnected from the Internet*/
	LUAT_MOBILE_NETIF_LINK_OOS,	/**<Lost network connection, trying to recover, equivalent to LUAT_MOBILE_NETIF_LINK_OFF*/
}LUAT_MOBILE_NETIF_STATUS_E;

typedef enum LUAT_MOBILE_BEARER_STATUS
{
	LUAT_MOBILE_BEARER_GET_DEFAULT_APN = 0,/**< Get the default APN*/
	LUAT_MOBILE_BEARER_APN_SET_DONE,/**< Set APN information completed*/
	LUAT_MOBILE_BEARER_AUTH_SET_DONE,/**< Set APN encryption status completed*/
	LUAT_MOBILE_BEARER_DEL_DONE,/**< Deletion of APN information completed*/
	LUAT_MOBILE_BEARER_SET_ACT_STATE_DONE,/**< APN activation/deactivation completed*/
}LUAT_MOBILE_BEARER_STATUS_E;


/**
 * @brief Get SIM card status
 *
 * @return see @enum LUAT_MOBILE_SIM_STATUS_E*/
LUAT_MOBILE_SIM_STATUS_E luat_mobile_get_sim_status(void);

/**
 * @brief Get the current mobile network registration status
 *
 * @return see @enum LUAT_MOBILE_REGISTER_STATUS_E*/
LUAT_MOBILE_REGISTER_STATUS_E luat_mobile_get_register_status(void);

/**
 * @brief The callback function when the network status and related functional status are changed. Event is the message, index is the serial number such as CID and SIM card number, and status is the changed status or a more specific ENUM.
 **/
typedef void (*luat_mobile_event_callback_t)(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status);


/**
 * @brief The underlying SMS message callback function, event is the message, and param is the specific data pointer. Different platforms need to be processed separately for the time being.
 **/
typedef void (*luat_mobile_sms_event_callback_t)(uint32_t event, void *param);


/**
 * @brief Register the callback function when the network status and related functional status change
 *
 * @param callback_fun callback function when network status and related functional status change
 * @return int =0 success, others failure*/
int luat_mobile_event_register_handler(luat_mobile_event_callback_t callback_fun);

/**
 * @brief Callback function when logout network status and related functional status changes
 *
 * @return int =0 success, others failure*/
int luat_mobile_event_deregister_handler(void);


/**
 * @brief Register the underlying SMS message callback function, which will be later changed to unified message processing
 *
 * @param callback_fun SMS message callback function, if it is NULL, it means logout
 * @return int =0 success, others failure*/
int luat_mobile_sms_sdk_event_register_handler(luat_mobile_sms_event_callback_t callback_fun);
/* ------------------------------------------------- mobile status end ------------------------------------------------ */

/**
 * @brief Set the RRC automatic release time, and release RRC at the appropriate time after a period of time after RRC active (see LUAT_MOBILE_EVENT_CSCON)
 *
 * @param s timeout time, unit seconds, if it is 0, the function is turned off
 * @note If you have not used AT*RTIME on Air724, or do not understand the meaning of RRC, please do not use RRC related APIs*/
void luat_mobile_set_rrc_auto_release_time(uint8_t s);

/**
 * @brief RRC automatically releases pause/resume
 *
 * @param onoff 1 pause 0 resume
 * @note If you have not used AT*RTIME on Air724, or do not understand the meaning of RRC, please do not use RRC related APIs*/
void luat_mobile_rrc_auto_release_pause(uint8_t onoff);


/**
 * @brief RRC is released immediately and cannot be used in luat_mobile_event_callback
 * @note If you have not used AT*RTIME on Air724, or do not understand the meaning of RRC, please do not use RRC related APIs*/
void luat_mobile_rrc_release_once(void);

/**
 * @brief Re-evaluate the underlying network protocol stack. The essence is to quickly enter and exit flight mode. Note that it conflicts with setting flight mode. Only one can be used within a certain period of time.
 *
 * @return int =0 success, others failure*/
int luat_mobile_reset_stack(void);

/**
 * @brief Allow automatic restart of the protocol stack when encountering serious network errors
 * @param onoff 0 is off, others are on
 * @return void*/
void luat_mobile_fatal_error_auto_reset_stack(uint8_t onoff);
/**
 * @brief Set up periodic auxiliary work, including periodic search for cell base stations, and periodic attempts to restore the SIM card after it leaves the card slot for a short time. This function may conflict with luat_mobile_reset_stack. All functions are turned off by default
 *
 * @param get_cell_period The time interval for periodically searching for cell base stations, in ms. This will increase low power consumption. Make it as long as possible, or write 0 to turn off this function and use the above method to search.
 * @param check_sim_period The time interval for trying to recover after the SIM card is removed from the card slot for a short time, in ms. It is recommended to be 5000~10000, or write 0. When the SIM card removal message comes up, manually restart the protocol stack
 * @param search_cell_time After starting the periodic search for cell base stations, the maximum time for each search, unit s, 1~8
 * @return int*/
int luat_mobile_set_period_work(uint32_t get_cell_period, uint32_t check_sim_period, uint8_t search_cell_time);

/**
 * @brief Set up a regular check to see if the network is normal and restore it by restarting the protocol stack when no network is detected for a long time. However, success is not guaranteed. This function may conflict with luat_mobile_reset_stack. All functions are turned off by default
 *
 * @param period The length of time without network, unit ms, cannot be too short. It is recommended to be above 60000 to reserve enough time for network search.
 * @return void*/
void luat_mobile_set_check_network_period(uint32_t period);


/**
 * @brief Get accumulated IP traffic data
 * @param uplink uplink traffic
 * @param downlink downlink traffic
 * @return none*/
void luat_mobile_get_ip_data_traffic(uint64_t *uplink, uint64_t *downlink);
/**
 * @brief Clear IP traffic data
 * @param clear_uplink Clear uplink traffic
 * @param clear_downlink Clear downlink traffic
 * @return none*/
void luat_mobile_clear_ip_data_traffic(uint8_t clear_uplink, uint8_t clear_downlink);
/**
 * @brief Get the frequency bands supported by the Modules
 * @param band The cache that stores the output band value requires at least CMI_DEV_SUPPORT_MAX_BAND_NUM bytes of space.
 * @param total_num number of frequency bands
 * @return Returns 0 if successful, otherwise failed*/
int luat_mobile_get_support_band(uint8_t *band,  uint8_t *total_num);
/**
 * @brief Get the frequency band currently used by the Modules
 * @param band The cache that stores the output band value requires at least CMI_DEV_SUPPORT_MAX_BAND_NUM bytes of space.
 * @param total_num number of frequency bands
 * @return Returns 0 if successful, otherwise failed*/
int luat_mobile_get_band(uint8_t *band,  uint8_t *total_num);
/**
 * @brief Set the frequency band used by the Modules
 * @param band The frequency band set requires at least total_num amount of space.
 * @param total_num number of frequency bands
 * @return Returns 0 if successful, otherwise failed*/
int luat_mobile_set_band(uint8_t *band,  uint8_t total_num);

#ifndef __LUATOS__
enum
{
	MOBILE_CONF_RESELTOWEAKNCELL = 1,
	MOBILE_CONF_STATICCONFIG,
	MOBILE_CONF_QUALITYFIRST,
	MOBILE_CONF_USERDRXCYCLE,
	MOBILE_CONF_T3324MAXVALUE,
	MOBILE_CONF_PSM_MODE,
	MOBILE_CONF_CE_MODE,
	MOBILE_CONF_SIM_WC_MODE,
	MOBILE_CONF_FAKE_CELL_BARTIME,
	MOBILE_CONF_RESET_TO_FACTORY,
};
#endif
/**
 * @brief LTE protocol stack function special configuration
 * @param item see MOBILE_CONF_XXX
 * @param value configuration value
 * @return Returns 0 if successful, otherwise failed*/
int luat_mobile_config(uint8_t item, uint32_t value);

/**
 * @brief RF test mode
 * @param uart_id serial port ID of test result output
 * @param on_off enters and exits test mode, 0 exits, others enter
 * @return none*/
void luat_mobile_rf_test_mode(uint8_t uart_id, uint8_t on_off);
/**
 * @brief RF test instructions or data input need to be obtained by the user from the serial port.
 * @param data data, can be empty, only when it is empty will the instruction actually start processing
 * @param data_len data length, can be 0, only when it is 0 will the instruction actually start processing
 * @return none*/
void luat_mobile_rf_test_input(char *data, uint32_t data_len);


/**
 * @brief Whether to allow base station time to be synchronized to local time
 * @param on_off 0 is not allowed, others are allowed*/
void luat_mobile_set_sync_time(uint8_t on_off);

/**
 * @brief Check whether the base station time is currently allowed to be synchronized to local time
 * @return =0 is not allowed, others are allowed*/
uint8_t luat_mobile_get_sync_time(void);

int luat_mobile_softsim_onoff(uint8_t on_off);
int luat_mobile_sim_detect_onoff(uint8_t on_off);
//Experimental API, please do not use it
void luat_mobile_set_auto_rrc(uint8_t s1, uint32_t s2);
void luat_mobile_set_auto_rrc_default(void);
/** @}*/

#endif
