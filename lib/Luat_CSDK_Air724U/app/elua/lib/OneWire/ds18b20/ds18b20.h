#ifndef __ONEWIRE_DS18B20_H
#define __ONEWIRE_DS18B20_H

#include "OneWire.h"


// pinè¡Öµ0¡ ¢ 1¡ ¢ 2¡ ¢ 3¡ ¢ 7¶ôó¦GPio0¡ ¢ GPIO1¡ ¢ GPIO2¡ ¢ Gpio3¡ ¢ GpiO7 £ ¬õÂ¼¸ö¶¼¿Éòôõý³ £ ¶ÁÈ¡µ ¥ × üÏß £


/**
 * @dwriting: ugds18b20µµµ´â ¼¾¾¾µ²0.0625¹´´«´´óR³
 * @param : pin{unt8}:O´µµý´´µug¶ç0§0±±±±¢7
 * TempNum{int *: ISâ uggâ₳
 * @return 0:Õ³£”.
 * 2:´ÀµµµnÔÔÔnnnnnS§
 * 3:â€™s²µ²µµµ´s*/
uint8 DS18B20_GetTemp_Num(uint8 pin, int *TempNum);


/*you
 I
 I
 I
 *@re
 * 1:TSTS
 *2:2µµ »
 * 3: ¼ón¬¬¬¬18b20*/
uint8 DS18B20_GetTemp_String(uint8 pin, char *TempStr);
#endif
