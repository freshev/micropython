#ifndef __IOT_PWM_H__
#define __IOT_PWM_H__

#include "iot_os.h"

/**
 * @ingroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
/**
 * @defgroup iot_sdk_pwm pwm½Ó¿Ú
 * @{*/

/**´ò ° Pwm¹¦äü 
*@param port: ¶ëú
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_pwm_open(E_AMOPENAT_PWM_PORT port);

/**Éèöãpwm¹¦äü
*@param pwm_cfg: pwm_cfg
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_pwm_set(T_AMOPENAT_PWM_CFG * pwm_cfg);

/**¹ø ± õpwm¹¦äü
*@param port: ¶ëú
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_pwm_close(E_AMOPENAT_PWM_PORT port);


/** @}*/
/** @}*/





#endif

