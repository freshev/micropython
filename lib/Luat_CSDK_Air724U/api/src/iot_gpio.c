#include "iot_gpio.h"

/**Åäöãgpio 
*@param port: gpio ± àºå
*@param cfg: åööãðåï ¢
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_gpio_open(                          
                        E_AMOPENAT_GPIO_PORT port, 
                        T_AMOPENAT_GPIO_CFG *cfg    
                   )
{
    return OPENAT_config_gpio(port, cfg);
}

/**Éèöãgpio 
*@param port: gpio ± àºå
*@param value: 0 or 1
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_gpio_set(                               
                        E_AMOPENAT_GPIO_PORT port,  
                        UINT8 value                
                )
{
    return OPENAT_set_gpio(port, value);
}

/**¶Áè¡ 
*@param port: gpio ± àºå
*@param value: 0 or 1
*@Return True: ³é¹¦
* FALSE: § ° ü
**/				
BOOL iot_gpio_read(                            
                        E_AMOPENAT_GPIO_PORT port, 
                        UINT8* value                
                  )
{
    return OPENAT_read_gpio(port, value);
}

/**¹Ø ± õgpio 
*@param port: gpio ± àºå
*@Return True: ³é¹¦
* FALSE: § ° ü
**/	
BOOL iot_gpio_close(                            
                        E_AMOPENAT_GPIO_PORT port
                  )
{
    return OPENAT_close_gpio(port);
}
