#ifndef __IOT_KEYPAD_H__
#define __IOT_KEYPAD_H__

#include "iot_os.h"

/**
 * @ingroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
/**
 * @defgroup iot_sdk_keypad °´¼ü½Ó¿Ú
 * @{*/


/**¼üåì³õê¼ »¯ 
*@param pconfig: ¼üåìåäöã²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_keypad_init(                         
                        T_AMOPENAT_KEYPAD_CONFIG *pConfig
                  );

/** @}*/
/** @}*/

#endif

