#ifndef __IOT_UART_H__
#define __IOT_UART_H__

#include "iot_os.h"
/**
 * @ingroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
/**
 * @defgroup iot_sdk_uart ´®¿Ú½Ó¿Ú
 * @{*/
/**@example uart/demo_uart.c
* uart½Ó¿ÚÊ¾Àý*/ 

/**´ò ‘ªUART
*@param port: uart ± atºå
*@param cfg: åööãðåï ¢
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_uart_open(
                        E_AMOPENAT_UART_PORT port,       
                        T_AMOPENAT_UART_PARAM *cfg         
                   );

/**¹Ø ± õuart
*@param port: uart ± atºå
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_uart_close(
                        E_AMOPENAT_UART_PORT port          
                   );


/**¶AUART
*@param port: uart ± atºå
*@param BUF: ´æ´ ¢ Êý¾ýµøö ·
*@param buflen: ´æ´ ¢ — õ vokes
*@param timeoutms: ¶eè¡³¬ê ± ms
*@Return uint32: ê µsywork
**/
UINT32 iot_uart_read(                                       
                        E_AMOPENAT_UART_PORT port,         
                        UINT8* buf,                       
                        UINT32 bufLen,                      
                        UINT32 timeoutMs                  
                   );

/**дuart
*@param port: uart ± atºå
*@param buff: ð´èëêý¾ýµøö ·
*@param buflen: ð´èëêý¾ý³hood
*@Return uint32: ê µsywork
**/
UINT32 iot_uart_write(                                      
                        E_AMOPENAT_UART_PORT port,        
                        UINT8* buf,                         
                        UINT32 bufLen                       
                    );

/** @}*/
/** @}*/
#endif

