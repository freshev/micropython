#include "iot_sys.h"

/**Ôrio³ìéý¼àê¼ »¯
*@Return 0: ± Nê¾³é¹¦
* <0 £ º ± Nê¾óð´nîó
**/
E_OPENAT_OTA_RESULT iot_fota_init(void)
{
    return openat_otaInit();
}

/**Ôrio³ìéý¼¶ïâôø
*@param Date: ïâôØ¹ì¼þ ° üêý¾ý
*@param len: ïâôØ¹ì¼þ ° ü³hood
*@total param: ¹ìrse ° ü × ü´óðª
*@Return 0: ± Nê¾³é¹¦
* <0 £ º ± Nê¾óð´nîó
**/
E_OPENAT_OTA_RESULT iot_fota_download(const char* data, UINT32 len, UINT32 total)
{
    return openat_otaProcess((char*)data, len,total);
}

/**Ôrio³ìéý¼¶
*@Return 0: ± Nê¾³é¹¦
* <0 £ º ± Nê¾óð´nîó
**/
E_OPENAT_OTA_RESULT iot_fota_done(void)
{
    return openat_otaDone();
}

/**otoÉÀYÃ core sedµçµµ´´´´óçócultculüçóóóó¼µµµµâ
*@param NewCorFile: Â³â‼oâ‼ 
*@return TRUE: ³BS″ FALSE: AN§°Ü
*×¢ºµµµölireFilleFileEâ ¼ïx´culnÑ´Ðughçoçü¼µçoçoçoreFilleFileFileFille.
**/
BOOL iot_ota_newcore(              
                    CONST char* newCoreFile
               )
{
    return IVTBL(FlashSetNewCoreRegion)((char*)newCoreFile);
}


/**otaÉalew app¼ö¼µµ´´´´óçócultçóóççÀ‼µµµµ³culcultâ
*@param NewPPFile: Â³âMESâ 
*@return TRUE: ³BS″ FALSE: AN§°Ü
*×¢ºµµ µÉÑµ±±´culè´culèÐ¼¼¼µcultlateÑ¼,AYYàcnewAPPFile Åª "s
**/
BOOL iot_ota_newapp(              
                    CONST char* newAPPFile
               )
{
    return IVTBL(FlashSetNewAppRegion)(newAPPFile);
}


/**ugaraAteâ‟âÄçarü´”«WWAR´´ghE´×ðððYðón_-opsµfly_foundµfileµ‵µµâ
*@param dst: ×³””³³¹z
*@param src: µýý×µ¼µ××è××Ö®
*@return ·ØsstY×Ö 
**/ 
WCHAR* iot_strtows(WCHAR* dst, const char* src)
{
   WCHAR* rlt = dst;
   while(*src)
   {
       *dst++ = *src++;
   }
   *dst = 0;
   
   return (rlt);
}


