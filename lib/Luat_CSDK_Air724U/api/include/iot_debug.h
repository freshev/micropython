#ifndef __IOT_DEBUG_H__
#define __IOT_DEBUG_H__

#include "iot_os.h"
#include "stdarg.h"

/**
 * @defgroup iot_sdk_debug µ÷ÊÔ½Ó¿Ú
 * @{*/

/**µ÷ÊÔÐÅÏ¢´òÓ¡
**/
VOID iot_debug_print(     CHAR *fmt, ...);

/**Assertinaïñô
*@param Condition: ¶ïñôìõrseþ
*@param FUNC: ¶ïñóºº thousand
*@param line: ¶ïñôî »Öã
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
VOID iot_debug_assert(                                            
                        BOOL condition,                  
                        CHAR *func,                      
                        UINT32 line                     
              );

/**ÉèÖÃÈn¼þÒì³£Ê±£¬Éè±¸Ä£Ê½
*@param		mode:	OPENAT_FAULT_RESET ÖØÆôÄ£Ê½
                    OPENAT_FAULT_HANG  µ÷ÊÔÄ£Ê½
**/

VOID iot_debug_set_fault_mode(E_OPENAT_FAULT_MODE mode);

/** @}*/

#endif
