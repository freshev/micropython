#include "am_openat.h"
#include "iot_zbar.h"

/**½ €þþî¬âë £ ¬ ¬ µ »Ø¾ä ± uhandle
*@param width: ½ ½önmost
*@param height: ½,îömost
*@param Date: ½,îömost
*@param size: ½ ½önàn¬êýtherý´óðª
*@return »ñè¡ ± uhandle £ ¬ ¬ ²» µèóú0 ± nê¾½îö³é¹¦
*           
**/
int iot_zbar_scannerOpen(int width, int height, int size, unsigned char *data)
{
    return IVTBL(zbar_scanner_open)(width, height, size, data);
}

/**»Ñèěâîö¶þþ¬¬¬ýty
*@param Handle: Žä ± Ú, óéiot_zbar_scanneropenopenéúúúúúúú
*@param only: »ñènky µä µäč¤¶è
*@Return »ñè údenýchýchýchýchýchý m
**/
char * iot_zbar_getData(int handle, int *len)
{
    return IVTBL(zbar_get_data)(handle, len);
}

/**²é'´êç · ñ »¹óðïâò»
*@param handle: ¾ä ± ú, oéiot_zbar_scanneropeneú³é
*@Return True: »¹óóïâò»
* False: ã »Óðïâò»
**/
BOOL iot_zbar_findNextData(int handle)
{
    return IVTBL(zbar_find_nextData)(handle);
}

/**»Ñè¡ ± ° ° ° ±
*@param handle: ¾ä ± ú, oéiot_zbar_scanneropeneú³é
*@Return · µ »Øµäçãoà àðn
**/
char * iot_zbar_getType(int handle)
{
  return IVTBL(zbar_get_type)(handle);
}

/**Én · å¾ä ± u
*@param handle: ¾ä ± ú, oéiot_zbar_scanneropeneú³é
*@Return Void	
**/
void iot_zbar_scannerClose(int handle)
{
    IVTBL(zbar_scanner_close)(handle);
}

