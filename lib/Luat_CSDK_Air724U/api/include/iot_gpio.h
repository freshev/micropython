#ifndef __IOT_GPIO_H__
#define __IOT_GPIO_H__

#include "iot_os.h"

/**
 * @defgroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
	/**@example gpio/demo_gpio.c
	* gpio½Ó¿ÚÊ¾Àý*/ 

/**
 * @defgroup iot_sdk_gpio GPIO½Ó¿Ú
 * @{*/

/**Åäöãgpio 
*@param port: gpio ± àºå
*@param cfg: åööãðåï ¢
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_gpio_open(                          
                        E_AMOPENAT_GPIO_PORT port,  
                        T_AMOPENAT_GPIO_CFG *cfg  
                   );

/**Éèöãgpio 
*@param port: gpio ± àºå
*@param value: 0 or 1
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_gpio_set(                               
                        E_AMOPENAT_GPIO_PORT port,  
                        UINT8 value                 
                );

/**¶Áè¡ 
*@param port: gpio ± àºå
*@param value: 0 or 1
*@Return True: ³é¹¦
* FALSE: § ° ü
**/			
BOOL iot_gpio_read(                            
                        E_AMOPENAT_GPIO_PORT port, 
                        UINT8* value             
                  );

/**¹Ø ± õgpio 
*@param port: gpio ± àºå
*@Return True: ³é¹¦
* FALSE: § ° ü
**/	
BOOL iot_gpio_close(                            
                        E_AMOPENAT_GPIO_PORT port
                  );

/** @}*/
/** @}*/


#endif


