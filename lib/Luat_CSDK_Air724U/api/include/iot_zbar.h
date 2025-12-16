#ifndef __IOT_ZBAR_H__
#define __IOT_ZBAR_H__
#include "iot_os.h"

/**
 * @defgroup Iot_sdk_zbar ¶þîîâë½âîö½ó¿ú
 * @{{{*/
 
/**@EXAMPLE ZBAR/DEMO_ZBAR.C
* ¶Þîîâë½ó¿ones*/ 

/**½ €þþî¬âë £ ¬ ¬ µ »Ø¾ä ± uhandle
*@param width: ½ ½önmost
*@param height: ½,îömost
*@param Date: ½,îömost
*@param size: ½ ½önàn¬êýtherý´óðª
*@return »ñè¡ ± uhandle £ ¬ ¬ ²» µèóú0 ± nê¾½îö³é¹¦
*           
**/
int iot_zbar_scannerOpen(int width, int height, int size, unsigned char *data);

/**»Ñèěâîö¶þþ¬¬¬ýty
*@param Handle: Žä ± Ú, óéiot_zbar_scanneropenopenéúúúúúúú
*@param only: »ñènky µä µäč¤¶è
*@Return »ñè údenýchýchýchýchýchý m
**/
char * iot_zbar_getData(int handle, int *len);

/**²é'´êç · ñ »¹óðïâò»
*@param handle: ¾ä ± ú, oéiot_zbar_scanneropeneú³é
*@Return True: »¹óóïâò»
* False: ã »Óðïâò»
**/
BOOL iot_zbar_findNextData(int handle);

/**»Ñè¡ ± ° ° ° ±
*@param handle: ¾ä ± ú, oéiot_zbar_scanneropeneú³é
*@Return · µ »Øµäçãoà àðn
**/
char * iot_zbar_getType(int handle);

/**Én · å¾ä ± u
*@param handle: ¾ä ± ú, oéiot_zbar_scanneropeneú³é
*@Return Void	
**/
void iot_zbar_scannerClose(int handle);

/** @}*/
#endif
