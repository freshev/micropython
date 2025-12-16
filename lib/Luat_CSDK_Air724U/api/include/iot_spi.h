#ifndef __IOT_SPI_H__
#define __IOT_SPI_H__

#include "iot_os.h"

/**
 * @ingroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
/**
 * @defgroup iot_sdk_spi spi½Ó¿Ú
 * @{*/

/**Åäöãspi
*@param port: SPI ± atºå
*@param cfg: ³õê¼ »¯²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_spi_open(
                        E_AMOPENAT_SPI_PORT  port,         
                        T_AMOPENAT_SPI_PARAM *cfg        
                  );

/**¶Áè¡spieý¾ý
*@param port: SPI ± atºå
*@param BUF: ´æ´ ¢ Êý¾ýµøö ·
*@param buflen: ´æ´ ¢ — õ vokes
*@Return uint32: ê µsywork
**/
UINT32 iot_spi_read(                                       
                        E_AMOPENAT_SPI_PORT port,         
                        UINT8* buf,                      
                        UINT32 bufLen                      
                  );

/**Ð´èëspiêý¾ý
*@param port: SPI ± atºå
*@param buff: ð´èëêý¾ýµøö ·
*@param buflen: ð´èëêý¾ý³hood
*@Return uint32: ê µ Jeje´èë³¤¶è
**/
UINT32 iot_spi_write(                                       
                        E_AMOPENAT_SPI_PORT port,        
                        CONST UINT8* buf,                  
                        UINT32 bufLen                      
                   );

/** I'm workingout «ë« ¹¤'
* @ Mees and "ë" ë¹ · call · ½¹ · call · · · · "· · · · · · · · · CASTATION · · ·FFER · BE CAURA TOOR £ ¬ Weather"
* @ Param Port: spills ± atºl
* @ Param Txbuf: ð'»º³å
* @ Param Rasturba: ¶e »º³å
* @ Param Len: ¶eð'³¤¶ès
* @ Returns Uint32: ile μsorðs³¤¶è
**/
UINT32 iot_spi_rw(                                       
                        E_AMOPENAT_SPI_PORT port,         
                        CONST UINT8* txBuf,               
                        UINT8* rxBuf,                       
                        UINT32 len                          
                );

/**¹ø ± õspi
*@param port: SPI ± atºå
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_spi_close(
                        E_AMOPENAT_SPI_PORT  port
                );    

/** @}*/
/** @}*/

#endif



