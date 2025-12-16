#include "iot_flash.h"
#include "hal_config.h"


/** »Ñè¡Flash¿éóÃμäμøÖ · ¿¼¼¼ £ ¬ · μ» μäμøÖ · óÃà'''''''' 'èëiot_flash_erase¡ ¢ iot_flash_write¡ ¢ iot_flash_write¡ ¢ iot_flash_readμè½ó¡ £
* @ param addrout: · μ »ø¿óóãflashμøÖ ·
* @ param lenout: · μ »ø¿Öãflash³³¶è £ ¬μ ¥ î» î î ×½ú
* @ Return e_amopenat_memd_err: ³³¹ |: openat_memd_err_no, AEOàE§ ° ü
* @ Note ¸ã½ · μ »øμäμøÖ · êç64kb¶soæë · μ» øμäμμÖ · ¿μä¸¾¾¾μ ± ç ° ³³Ðò'óÐ¡à'è · ¶¨¡ £ £
**/
VOID iot_flash_getaddr(    
                    UINT32* addrout,
                    UINT32* lenout
               )
{
    extern char __flash_start;
    extern char __flash_end;
    UINT32 aligned = ((UINT32)&__flash_end + 0x10000 - 1) & (~(0x10000 - 1));
    if(addrout)
    {
        *addrout = aligned - CONFIG_NOR_PHY_ADDRESS;
    }
    if(lenout)
    {
        *lenout = CONFIG_APPIMG_FLASH_SIZE - (aligned - (UINT32)&__flash_start);
    }
}

/**Flash²e 
*@param Startaddr: ²eð´µøö · 64k¶ôæë
*@param endaddr: ²eð´½eøµøö ·
*@Return e_amopenat_memd_err: ³é¹¦: openat_memd_err_no
**/
E_AMOPENAT_MEMD_ERR iot_flash_erase(             
                        UINT32 startAddr,
                        UINT32 endAddr
                   )
{
    return OPENAT_flash_erase(startAddr, endAddr);
}

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
                   )
{
    return OPENAT_flash_write(startAddr, size, writenSize, buf);
}

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
                   )
{
    return OPENAT_flash_read(startAddr, size, readSize, buf);
}

