/*********************************************************************
 * Author: Zhou Linlin
 * E-mail: 461146760@qq.com
 * Date  : 2022-09-09 13:09:18.
 * Copyright (c) 2022 all rights reserved.
*********************************************************************/

#ifndef __SHOWMAC_H__
#define __SHOWMAC_H__

#ifdef __cplusplus
extern "C"{
#endif

/**
* Showmac application work mode type definition.
*/
typedef enum {
	SHOWMAC_PHYSICAL_SIM_MODE,	// PHYSICAL SIM
	SHOWMAC_SOFT_SIM_MODE,		// SOFT	SIM
	SHOWMAC_INVALID_MODE
} showmac_work_mode_t;

/**
 * Set PDP context ID
 * @param [IN]	cid, range: 1 ~ 10, 1 as default.
 * @return 0: success, -1: PDP context id out of range, -2: Store to file fail.
 */
int set_pdp_context_id(int cid);

/**
 * Get Showmac VSIM ATR
 * @param pATR 		[OUT] 	ATR
 * @param pSize		[OUT]	ATR content length (at least 14)
 * @param nSimID	[IN] 	ignored, not support multi card
 * @return always 0.
 * NOTE: The user should be supply a buffer do not less than 20 bytes length to receive the ATR.
 */
unsigned int get_vsim_ATR(unsigned char *pATR, unsigned int *pSize, unsigned char nSimID);

/**
 * APDU command handler
 * @param txData 	[IN]	APDU command
 * @param tx_len 	[IN]	APDU command length
 * @param rxData 	[OUT]	APDU response
 * @param rxSize 	[OUT]	APDU response length
 * @param nSimID 	[IN]	ignored, not support multi card
 * @return status word.
 */
unsigned int processAPDUCommand(unsigned char *txData, unsigned int tx_len, unsigned char *rxData, unsigned int *rxSize, unsigned char nSimID);

/**
 * Showmac app initialize
 * @return 0: success, -1: error occur
 */
int showmac_init(void);

/**
 * Get showmac work mode
 * @return SHOWMAC_PHYSICAL_SIM_MODE or SHOWMAC_SOFT_SIM_MODE
 */
showmac_work_mode_t get_showmac_work_mode(void);

/**
 * Set showmac work mode
 * @param type, should be SHOWMAC_PHYSICAL_SIM_MODE or SHOWMAC_SOFT_SIM_MODE
 * @return 0 success, -1 fail
 */
int set_showmac_work_mode(showmac_work_mode_t mode);

/**
 * Start showmac application
 */
void showmac_start(void);

/**
 * Stop showmac application
 */
void showmac_stop(void);

/**
 * switch SIM by operator ID.
 * @param [IN] id: 1 - China mobile, 2 - China Unicom, 3 - China Telecom
 * @return 0 success, others fail
 * NOTE: If no SIM installed, switch will be always failed.
 */
int showmac_switch_sim_by_operator_id(unsigned char id);

/*
    init physical tcp connect to sever
*/
void init_physical_tcp(void);




/**
 * Start showmac app all in one.
 */
void showmac_app_start(void);

void showmac_app_stop(void);

#include "cmsis_os2.h"

void send_vsim_apdu_signal(UINT16 txDataLen, UINT8 *txData, UINT16 *rxDataLen, UINT8 *rxData, osSemaphoreId_t sem);

void send_vsim_reset_signal(UINT16 *atrLen, UINT8 *atrData, osSemaphoreId_t sem);

#ifdef __cplusplus
}
#endif

#endif //__SHOWMAC_H__
