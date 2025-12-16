/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_bluetooth.h
 * Author:  liangjian
 * Version: V0.1
 * Date:    2020/09/08
 *

 **************************************************************************/


#ifndef __PLATFORM_BLUETOOTH_H__
#define __PLATFORM_BLUETOOTH_H__

#ifdef LUA_BLUETOOTH_LIB


#include "type.h"

typedef struct 
{
   UINT16 AdvMin;            ///< ×îÐ¡¹ã²¥¼ä¸ô
   UINT16 AdvMax;            ///< ×î´ó¹ã²¥¼ä¸ô
   UINT8 AdvType;           /// <¹Ã² ¥ ààðn
   UINT8 OwnAddrType;       /// <¹Ã² ¥ ± ¾µøµøÖ · Ààðn
   UINT8 DirectAddrType;    /// <<¶¶ · website
   char *DirectAddr;        ///< ¶¨ÏòµØÖ·
   UINT8 AdvChannMap;       ///< ¹ã²¥channel map,3¸öbit£¬·Ö±ð¶ÔÓ¦37£¬ 38£¬ 39ÐÅµÀ
   UINT8 AdvFilter;         /// <¥² ¥ ¥ ¥ý²ßo
} plat_advparam_t;

typedef struct 
{
    UINT8 scanType;            
    UINT16 scanInterval;      
    UINT16 scanWindow;     
    UINT8 filterPolicy; 
    UINT8 own_addr_type;
} plat_scanparam_t;

typedef struct _plat_bt_addr
{
    UINT8 addr[6];
} plat_bt_addr;

/*+\ENEW, administrators ±ÁÁãx´´´±‐offion½é´é´é basis °O´´´© £±£n µn 4µ*/

typedef struct _plat_ble_recv_buff
{
	UINT8 uuid_flag;
	UINT16 uuid; ///characteristic
	UINT8 long_uuid[16];
	UINT8 len;
	unsigned char *dataPtr; /// <· M »
} plat_ble_recv_buff;
/*-\NEW cgg\ czg\2020.11.25\BUG 3702: ±ÁÁãx´´´±‐offion½é´é´é basis °O´´´© £±£n µn 4µ*/

BOOL platform_ble_open(u8 mode);
BOOL platform_ble_close(void);
BOOL platform_ble_send(u8* data, int len, u16 uuid_c,u16 handle);
BOOL platform_ble_send_string(u8* data, int len, u8*  uuid_c,u16 handle);

/*+\ New \ czm \ 2020.11.25 \ bug 3702: 1.3 à¶ñelua ourselves £ ´øóðêý¾ý £ ¬çäî ° £ ™ £ £ £ £ £ £ £ £ £*/
int platform_ble_recv(plat_ble_recv_buff *data);
/*-\ new \ czm \ 2020.11.25 \ bug 3702: 1.3 à¶ñelua ourselves £ ´øóðêý¾ý £ ¬thão ° £ £ ¬¶Áè¡*/

BOOL platform_ble_set_name(u8* data);
BOOL platform_ble_set_adv_param(plat_advparam_t *param);
BOOL platform_ble_set_adv_data(u8* data,int len);
BOOL platform_ble_set_scanrsp_data(u8* data,int len);
BOOL platform_ble_set_adv_enable(u8 data);
BOOL platform_ble_set_scan_param(plat_scanparam_t *param);
BOOL platform_ble_set_scan_enable(u8 data);
BOOL platform_ble_disconnect(UINT16 handle);
BOOL platform_ble_connect(UINT8 addr_type, u8 *addr);
BOOL platform_ble_add_service(u16 uuid_s);
BOOL platform_ble_add_service_string(u8 *uuid_s);
BOOL platform_ble_add_characteristic(u16 uuid_c,u8 type,u16 permission);
BOOL platform_ble_add_characteristic_string(u8 *uuid_c,u8 type,u16 permission);
BOOL platform_ble_add_descriptor(u16 uuid_d,u8 *value,u16 configurationBits);
BOOL platform_ble_add_descriptor_string(u8 *uuid_d,u8 *value,u16 configurationBits);
BOOL platform_ble_find_characteristic(u16 uuid_s,u16 handle);
BOOL platform_ble_find_characteristic_string(u8* uuid_s,u16 handle);
BOOL platform_ble_find_service(u16 handle);

BOOL platform_ble_open_notification(u16 uuid_c,u16 handle);
BOOL platform_ble_open_notification_string(u8* uuid_c,u16 handle);
BOOL platform_ble_close_notification(u16 uuid_c,u16 handle);
BOOL platform_ble_close_notification_string(u8* uuid_c,u16 handle);
BOOL platform_ble_get_addr(u8* addr);
BOOL platform_ble_set_beacon_data(u8* uuid,u16 major,u16 minor);

BOOL platform_ble_read_state();
#endif

#endif //__PLATFORM_BLUETOOTH_H__


