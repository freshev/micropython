#include "iot_bluetooth.h"
/****************************** BLUETOOTH ******************************/


/**´ò ° àriañà
*@param mode: àriañà´ò £ ex express
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_bt_open(                                        
                        E_OPENAT_BT_MODE mode
                )
{
    IVTBL(SetBLECallback)((F_BT_CB)bluetooth_callback);
    return IVTBL(OpenBT)(mode);
}

/**¹ø ± õààà¶ñà
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_bt_close(                                        
                        VOID
                )
{
    return IVTBL(CloseBT)();
}

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
                )
{
    return IVTBL(WriteBLE)(handle,uuid,data,len);
}

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
         
                )
{
    return IVTBL(IotctlBLE)(handle,cmd,param);
}

/**A
*@param handle: à¬~ ± u  
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_ble_disconnect(                                        
                        UINT16 handle
                )
{
    return IVTBL(DisconnectBLE)(handle);
}

/**À paper
* *@param addr: à¬ °à¶ñàµøö ·
* *@param addr_thype: à¬óàààà's · · · · · · · ·
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_ble_connect(    
                        char *addr ,                                    
                        UINT8 addr_type
                        
                )
{
    return IVTBL(ConnectBLE)(addr_type,addr);
}

