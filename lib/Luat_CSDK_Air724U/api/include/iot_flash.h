#ifndef __IOT_FLASH_H__
#define __IOT_FLASH_H__

#include "iot_os.h"


/**
 * @defgroup iot_sdk_flash flash½Ó¿Ú
 * @{*/
/**@example flash/demo_flash.c
* flash½Ó¿ÚÊ¾Àý*/ 

/**»ñÈ¡flash¿ÉÓÃµÄµØÖ·¿Õ¼ä£¬·µ»ØµÄµØÖ·ÓÃÀ´´«Èëiot_flash_erase¡¢iot_flash_write¡¢iot_flash_readµÈ½Ó¿Ú¡£
* @ param addrout: · μ »ø¿óóãflashμøÖ ·
* @ param lenout: · μ »ø¿Öãflash³³¶è £ ¬μ ¥ î» î î ×½ú
* @ Return e_amopenat_memd_err: ³³¹ |: openat_memd_err_no, AEOàE§ ° ü
* @ Note ¸ã½ · μ »øμäμøÖ · êç64kb¶soæë · μ» øμäμμÖ · ¿μä¸¾¾¾μ ± ç ° ³³Ðò'óÐ¡à'è · ¶¨¡ £ £

**/
VOID iot_flash_getaddr(    
                    UINT32* addrout,
                    UINT32* lenout
               );

/**Flash²e 
*@param Startaddr: ²eð´µøö · 64k¶ôæë
*@param endaddr: ²eð´½eøµøö ·
*@Return e_amopenat_memd_err: ³é¹¦: openat_memd_err_no
**/
E_AMOPENAT_MEMD_ERR iot_flash_erase(              
                    UINT32 startAddr,
                    UINT32 endAddr
               );

/**Flashд 
*@param Startaddr: ð´µøö · 
*@param size: ð´êý¾ý´óðª
*@param Writensize: ð´êý¾ýàààn´óðª
*@param buff: ð´êýöçoë
*@Return e_amopenat_memd_err: ³é¹¦: openat_memd_err_no
**/
E_AMOPENAT_MEMD_ERR iot_flash_write(             
                    UINT32 startAddr,
                    UINT32 size,
                    UINT32* writenSize,
                    CONST UINT8* buf
               );

/**Flashlia
*@param STARTADDR: ¶eµøö · 
*@param size: ¶eêý¾ý´óðª
*@param readsize: ¶eêý¾ýààðn´óðª
*@param buff: ¶eêýöçoë
*@Return e_amopenat_memd_err: ³é¹¦: openat_memd_err_no
**/
E_AMOPENAT_MEMD_ERR iot_flash_read(              
                    UINT32 startAddr,
                    UINT32 size,
                    UINT32* readSize,
                    UINT8* buf
               );



/** @}*/







#endif

