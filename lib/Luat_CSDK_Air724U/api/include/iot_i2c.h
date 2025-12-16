#ifndef __IOT_I2C_H__
#define __IOT_I2C_H__

#include "iot_os.h"

/**
 * @ingroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
/**
 * @defgroup iot_sdk_i2c i2c½Ó¿Ú
 * @{*/

/**‘ª´òi2c
*@param port: i2c ± atºå
*@param param: ³õê¼ »¯²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_i2c_open(
                        E_AMOPENAT_I2C_PORT  port,         
                        T_AMOPENAT_I2C_PARAM *param        
                  );

/**¹Ø ± õi2c
*@param port: i2c ± atºå
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_i2c_close(
                        E_AMOPENAT_I2C_PORT  port          
                  );

/**Ð´èëi2cêý¾ý
*@param port: i2c ± atºå
*@param salveaddr: ´Oéè ± tiono ·
*@param pregaddr: ¼ä´ææ ÷ µøö ·
*@param buff: ð´èëêý¾ýµøö ·
*@param buflen: ð´èëêý¾ý³hood
*@Return uint32: ê µ Jeje´èë³¤¶è
**/
UINT32 iot_i2c_write(                                    
                        E_AMOPENAT_I2C_PORT port,        
                        UINT8 salveAddr,
                        CONST UINT8 *pRegAddr,          
                        CONST UINT8* buf,               
                        UINT32 bufLen                    
                   );

/**¶Áè¡ê2cêý¾ý
*@param port: i2c ± atºå
*@param slaveaddr: ´Oéè ± tiono ·
*@param pregaddr: ¼ä´ææ ÷ µøö ·
*@param BUF: ´æ´ ¢ Êý¾ýµøö ·
*@param buflen: ´æ´ ¢ — õ vokes
*@Return uint32: ê µsywork
**/
UINT32 iot_i2c_read(                                        
                        E_AMOPENAT_I2C_PORT port,        
                        UINT8 slaveAddr, 
                        CONST UINT8 *pRegAddr,             
                        UINT8* buf,                      
                        UINT32 bufLen                      
                  );

/** @}*/
/** @}*/

#endif

