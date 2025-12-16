#ifndef __IOT_PMD_H__
#define __IOT_PMD_H__

#include "iot_os.h"

/**
 * @defgroup iot_sdk_pmd µçÔ´¹ÜÀn½Ó¿Ú
 * @{*/

/**³äµç³õê ° »ogra
*@Param Charmode: ³äµç · ½êince
*@param cfg: åäöãðå after ¢
*@Param ppmmessage: ïûï ¢ »Øµ ÷ º owner
*@Return True: ³éiate
* False: ê§ ° ü
**/
BOOL iot_pmd_init(     
                    E_AMOPENAT_PM_CHR_MODE chrMode,     
                    T_AMOPENAT_PMD_CFG*    cfg,       
                    PPM_MESSAGE            pPmMessage  
            );


/** »Ñne¡μçowe ×ìtt
* @ Param Batstatus: μçskol ×ì¬
**/
VOID iot_pmd_get_batteryStatus(
                    T_AMOPENAT_BAT_STATUS* batStatus    
                     );

/**»ñÈ¡³äµçÆ÷×´Ì¬
*@param		chrStatus:		³äµçÆ÷×´Ì¬
**/
VOID iot_pmd_get_chargerStatus(
                    T_AMOPENAT_CHARGER_STATUS* chrStatus
                     );

/**²éÑ¯³äµçÆ÷HW×´Ì¬½Ó¿Ú
*@return	E_AMOPENAT_CHR_HW_STATUS: ³äµçÆ÷HW×´Ì¬½Ó¿Ú
**/
E_AMOPENAT_CHR_HW_STATUS iot_pmd_get_chargerHwStatus(
                    VOID
                    );

/**
*@param battStatus:
*@param battVolt:
*@param battLevel:
*@param chargerStatus:
*@param chargeState:
*@return int:
**/
int iot_pmd_get_chg_param(BOOL *battStatus, u16 *battVolt, u8 *battLevel, BOOL *chargerStatus, u8 *chargeState);


/**Õý³ £ ª »ú
*@param yesstartupmode: ‘ªæôsim« · ½ê½
*@param nwstartupmode: ‘ªæôðéõ» · ½ê½
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_pmd_poweron_system(                                     
                    E_AMOPENAT_STARTUP_MODE simStartUpMode,
                    E_AMOPENAT_STARTUP_MODE nwStartupMode
                  );

/** ÕÝ³ £ ¹Ø »Ù
*. Not n'tices I õÝ³ ¹Øout úе Er di ± üõ¨ oμ μç
**/
VOID iot_pmd_poweroff_system(VOID);

/**´ò —ªLDO
*@param ldo: ldon¨µà
*@param Level: 0-7 0: ¹Ø ± õ 1 ~ 7µçñ¹^è¼¶
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_pmd_poweron_ldo(                                       
                    E_AMOPENAT_PM_LDO    ldo,
                    UINT8                level          
               );

/**½øÈëË¯Ãß
**/
VOID iot_pmd_enter_deepsleep(VOID);

/**Íë³öë¯ãß
**/
VOID iot_pmd_exit_deepsleep(VOID);                               

/**»Ñèzzo» úôòòöµ
*@Return e_amopenat_poweron_reason: · µ »Øncy» úôòòòöµ
**/
E_AMOPENAT_POWERON_REASON iot_pmd_get_poweronCasue(VOID);

/** @}*/

#endif

