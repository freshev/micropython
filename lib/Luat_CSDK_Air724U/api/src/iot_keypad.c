#include "iot_keypad.h"



/**¼üåì³õê¼ »¯ 
*@param pconfig: ¼üåìåäöã²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_keypad_init(                         
                        T_AMOPENAT_KEYPAD_CONFIG *pConfig
                  )
{
   return OPENAT_init_keypad(  pConfig );
}

