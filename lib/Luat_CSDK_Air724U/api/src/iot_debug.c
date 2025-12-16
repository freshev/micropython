#include "iot_debug.h"
#include "am_openat.h"
#include "string.h"
#include <stdio.h>


extern BOOL g_s_traceflag;
/*******************************************
**                 DEBUG                  **
*******************************************/

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
              )
{
    IVTBL(assert)(condition, func, line);
}


/**µ÷ÊÔÐÅÏ¢´òÓ¡
**/
VOID iot_debug_print(CHAR *fmt, ...)
{
	char buff[256] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(buff, 256, fmt, args);
	if(g_s_traceflag)
		IVTBL(lua_print)("%s", buff);
	else
		IVTBL(print)("%s", buff);
	va_end (args);
}

/**ÉèÖÃÈn¼þÒì³£Ê±£¬Éè±¸Ä£Ê½
*@param	  mode:   OPENAT_FAULT_RESET ÖØÆôÄ£Ê½
				  OPENAT_FAULT_HANG  µ÷ÊÔÄ£Ê½
**/

VOID iot_debug_set_fault_mode(E_OPENAT_FAULT_MODE mode)
{
	IVTBL(SetFaultMode)(mode);
}

