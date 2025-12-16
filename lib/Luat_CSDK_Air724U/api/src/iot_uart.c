#include "iot_uart.h"

/**´ò ‘ªUART
*@param port: uart ± atºå
*@param cfg: åööãðåï ¢
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_uart_open(
                        E_AMOPENAT_UART_PORT port,          
                        T_AMOPENAT_UART_PARAM *cfg         
                   )
{
    return OPENAT_config_uart(port, cfg);
}

/**¹Ø ± õuart
*@param port: uart ± atºå
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_uart_close(
                        E_AMOPENAT_UART_PORT port          
                   )
{
    return OPENAT_close_uart(port);
}
 
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
                   )
{
    return OPENAT_read_uart(port, buf, bufLen, timeoutMs);
}

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
                    )
{
    return OPENAT_write_uart(port, buf, bufLen);
}

/**Uart½óêõööïê¹äü
*@param port: uart ± atºå
*@param enable: Éç · ñê¹äü
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_uart_enable_rx_int(
                        E_AMOPENAT_UART_PORT port,          
                        BOOL enable                       
                            )
{
    //return IVTBL(uart_enable_rx_int)(port, enable);
    return FALSE;
}
