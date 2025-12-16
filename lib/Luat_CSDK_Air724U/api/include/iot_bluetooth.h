#ifndef __IOT_BLUETOOTH_H__
#define __IOT_BLUETOOTH_H__

#include "iot_os.h"
#include "am_openat_bluetooth.h"

void bluetooth_callback(T_OPENAT_BLE_EVENT_PARAM *result);
/**
 * @defgroup iot_sdk_bluetooth À¶ÑÀ½Ó¿Ú
 * @{*/
/**@example bluetooth/demo_bluetooth.c
* bluetooth½Ó¿ÚÊ¾Àý*/

/**´ò ° àriañà
*@param mode: àriañà´ò £ ex express
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_bt_open(                                        
                        E_OPENAT_BT_MODE mode
                );

/**¹ø ± õààà¶ñà
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_bt_close(                                        
                        VOID
                );

/**Ð´àriañà
*@param handle: à¬~ ± u  
*@param uuid: ð´èëìøõ ÷ uuid
*@param Date: ð´èëêý¾ýäúèý
*@param len: ð´èëêý¾ý³hood

*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_ble_write(    
                        UINT16 handle,   
                        T_OPENAT_BLE_UUID uuid,                                 
                        char *data , 
                        UINT8 len         
                );

/**Àriañàæäëû² × ÷
*@param handle: à¬~ ± u  
*@param cmd: ¹¦äücmd
*@param parm: ²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_ble_iotctl(      
                        UINT16 handle,                                  
                        E_OPENAT_BT_CMD cmd,
                        U_OPENAT_BT_IOTCTL_PARAM  param
         
                );

/**A
* *@param handle: à¬~ ± u  
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_ble_disconnect(                                        
                        UINT16 handle
                );

/**À paper
* *@param addr: à¬ °à¶ñàµøö ·
* *@param addr_thype: à¬óàààà's · · · · · · · ·
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_ble_connect(    
                        char *addr ,                                    
                        UINT8 addr_type
                        
                );

#endif
