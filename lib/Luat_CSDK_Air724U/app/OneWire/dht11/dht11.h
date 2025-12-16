#ifndef __ONEWIRE_DHT11_H
#define __ONEWIRE_DHT11_H

#include "OneWire.h"

// pinè¡Öµ0¡ ¢ 1¡ ¢ 2¡ ¢ 3¡ ¢ 7¶ôó¦GPio0¡ ¢ GPIO1¡ ¢ GPIO2¡ ¢ Gpio3¡ ¢ GpiO7 £ ¬õÂ¼¸ö¶¼¿Éòôõý³ £ ¶ÁÈ¡µ ¥ × üÏß £

/**
 * @described: ´Ódht11µµµ´â‼ug´è¶ÕÕÕÕÕ×Ö
 * @param : pin{unt8}:O´µµý´´µug¶ç0§0±±±±¢7
 * HumNum{uth8 *: ISâ ³âLYµ
 * TemNum{uth8 *: Bound³è¶
 * @return 0:Õ³£”.
 * 2:´ÀµµµnÔÔÔnnnnnS§
 * 3: â€™s²µµ¼½dht11
 * 4:ââÄº´öé²ó*/

uint8 DHT11_GetData_Num(uint8 pin, uint8 *HumNum, uint8 *TemNum);
/**
 * @described: ´Ódht11µµµµ´µµÖ ×èÖ´®
 * @param : pin{unt8}:O´µµý´´µug¶ç0§0±±±±¢7
 * HumStr{char *}: INµµÖµÖ ´®
 * TemStr{char *}:WholdµµµµÖ Ö.
 * @return 0:Õ³£”.
 * 1:HumStr == NULL || TemStr == NULL
 * 2:´ÀµµµnÔÔÔnnnnnS§
 * 3: â€™s²µµ¼½dht11
 * 4:ââÄº´öé²ó*/
uint8 DHT11_GetData_String(uint8 pin, char *HumStr, char *TemStr);
#endif
