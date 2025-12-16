#include "iot_pwm.h"

/**´ò ° Pwm¹¦äü 
*@param port: ¶ëú
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_pwm_open(E_AMOPENAT_PWM_PORT port)
{
    return OPENAT_pwm_open(port);
}

/**Éèöãpwm¹¦äü
*@param pwm_cfg: pwm_cfg
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_pwm_set(T_AMOPENAT_PWM_CFG * pwm_cfg)
{
    return OPENAT_pwm_set(pwm_cfg);
}

/**¹ø ± õpwm¹¦äü
*@param port: ¶ëú
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_pwm_close(E_AMOPENAT_PWM_PORT port)
{
    return OPENAT_pwm_close(port);
}

