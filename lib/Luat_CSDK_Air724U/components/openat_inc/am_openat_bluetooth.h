/*********************************************************
  Copyright (C), AirM2M Tech. Co., Ltd.
  Author: liangjian
  Description: AMOPENAT ¿ª·ÅÆ½Ì¨
  Others:
  History: 
    Version£º Date:       Author:   Modification:
    V0.1      2020.09.05  liangjain     ´´½¨ÎÄ¼þ
*********************************************************/
#ifndef AM_OPENAT_BULETOOTH_H
#define AM_OPENAT_BULETOOTH_H

#include "am_openat_common.h"
//#include "drv_bluetooth.h"

#define BLE_MAX_DATA_COUNT  244
#define BLE_MAX_ADV_MUBER  31
#define BLE_LONG_UUID_FLAG 16
#define BLE_SHORT_UUID_FLAG 2

typedef struct 
{
    unsigned char id;         ///< event identifier
    char state;               /// <· µ »
    UINT8 len;                /// <· m »
    unsigned char * dataPtr;  /// <· M »
    UINT16 uuid;   
    UINT16 handle;
    UINT8 long_uuid[16];
    UINT8 uuid_flag;
}T_OPENAT_BLE_EVENT_PARAM;

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
}T_OPENAT_BLE_ADV_PARAM;

typedef struct 
{
    UINT8 scanType;        //  É¨ÃèÀàÐÍ
    UINT16 scanInterval;   //  É¨Ãè¼ä¸ô
    UINT16 scanWindow;     //  É¨Ãè´°¿Ú
    UINT8 filterPolicy;    // E¨ â¹ý eïßo
    UINT8 own_addr_type;   // ± ± irenha · website
} T_OPENAT_BLE_SCAN_PARAM;

typedef enum
{
    BLE_SET_NAME = 0x01,    /// <éèöãble ¹ã² ¥ ÃÛ³Æ
    BLE_SET_ADV_PARAM,		/// <éèöãble 14 ¥ ¹ den
    BLE_SET_ADV_DATA,		/// <éèöãble 14 ¥ ¹ den
    BLE_SET_SCANRSP_DATA,	/// <éèöãble ¹ ax ¥ ïìó¦ ° üêýther
	BLE_SET_ADV_ENABLE,		/// <e · · ¥ ¥
	BLE_SET_SCAN_ENABLE,		/// <Êç · ¹ ¹ ê. ¹ ê ¹ ¹
	BLE_READ_STATE,			///
    BLE_ADD_SERVICE,        /// <ìn¼ó · Þîñ
    BLE_ADD_CHARACTERISTIC, ///< Ìn¼ÓÌØÕ÷
    BLE_ADD_DESCRIPTOR,     ///< Ìn¼ÓÃèÊö
    BLE_FIND_SERVICE,       /// <· ¢ ïö · Þîñ
    BLE_FIND_CHARACTERISTIC,///< ·¢ÏÖÌØÕ÷
    BLE_OPEN_NOTIFICATION,  /// <'all of it
    BLE_CLOSE_NOTIFICATION,  ///< ¹Ø±ÕÍ¨Öª
    BLE_GET_ADDR,           /// <»Đàn · ·
    BLE_SET_BEACON_DATA,     ///< ÉèÖÃbeaconÊý¾Ý
    BLE_SET_SCAN_PARAM,		/// <.èöãbleé¨Ãè²Î
    BT_GET_ADDR,           /// <»Đàn · ·
    BT_READ_STATE,			/// <nbt · · ·
    BT_SET_NAME,           ///< xedÃ³
    BT_SET_VISIBILITY,      /// <éèöãbt¿é¼únge
    BT_SET_HFP_VOL,      /// <Emmasiwpp [,ат
    BT_HFP_CALL_REJECT,  ///< HFP¾Ü¾øÀ´µç
    BT_HFP_CALL_ANSWER,  ///< HFP½ÓÌýÀ´µç
    BT_HFP_CALL_HANGUP,  /// <hfp¹òàope
    BT_HFP_CALL_REDIAL,  /// <hfpöø²¦
    BT_HFP_CALL_DIAL,  /// <hfp²¦ºå
    BT_SET_AVRCP_VOL,  ///< ÉèÖÃAVRCPµÄÒôÁ¿
    BT_SET_AVRCP_SONGS,    /// <AVRCPµä¸èçú¿øöæ
} E_OPENAT_BT_CMD;

typedef enum
{
    BLE_SLAVE = 0,    ///< ÉèÖÃBLE´ÓÄ£Ê½
    BLE_MASTER,	      ///< ÉèÖÃBLEÖ÷Ä£Ê½
    BT_CLASSIC,	          ///< ÉèÖÃ¾­µäÀ¶ÑÀ
} E_OPENAT_BT_MODE;

typedef enum
{
    UUID_SHORT = 0,    // 16λuuid
    UUID_LONG,	       // 128λuuid
} E_OPENAT_BLE_UUID_FLAG;

typedef struct 
{
    E_OPENAT_BLE_UUID_FLAG uuid_type;
    union {
        UINT16 uuid_short;
        UINT8 uuid_long[16];
    };
}T_OPENAT_BLE_UUID;

typedef struct 
{
    T_OPENAT_BLE_UUID uuid;
    UINT8   attvalue;// êôðµ
    UINT16  permisssion;//È¨ÏÞ
}T_OPENAT_BLE_CHARACTERISTIC_PARAM;

typedef struct 
{
    T_OPENAT_BLE_UUID uuid;
    UINT8   value[255];/// °ð00
    UINT16  configurationBits;/// °ð00
}T_OPENAT_BLE_DESCRIPTOR_PARAM;

typedef struct 
{
    UINT8 		data[BLE_MAX_ADV_MUBER]; 
    UINT8       len;
}T_OPENAT_BLE_ADV_DATA;

typedef struct 
{
    UINT8 uuid[16];
    UINT16 major;
    UINT16 minor; 
}T_OPENAT_BLE_BEACON_DATA;

typedef enum
{
    BT_AVRCP_STATE_PAUSE = 0x00,
    BT_AVRCP_STATE_START,
    BT_AVRCP_STATE_PREVI,
    BT_AVRCP_STATE_NEXT,
} E_OPENAT_BT_AVRCP_STATE;

typedef union {
    T_OPENAT_BLE_ADV_PARAM  *advparam;   /// <éèöãble ¹à² ¥ ²îê
    T_OPENAT_BLE_SCAN_PARAM  *scanparam;   /// <éèöãble é¨ãè²îê
    T_OPENAT_BLE_ADV_DATA   *advdata;    ///< ¹ã²¥°üÊý¾Ý¡¢ÏìÓ¦°üÊý¾Ý
    UINT8 	*data;    ///< ISÃBLE ¹ ²³Æ³”niÑÀ¶â‼ugúÑ¢³³²²²²²²²«
    UINT8       advEnable;          /// ñ · ñ tuus¹²¹¹¹¹ü bbbbããããããããããããããããaók
    T_OPENAT_BLE_UUID  *uuid;   /// <ìNóp · ¢ ï¢ · ¢ ïoö ¡¢ ¢ ¢ ¢ ¢ ¢ ¢ ¹õ ¡า õา <¢ ±า
    T_OPENAT_BLE_CHARACTERISTIC_PARAM  *characteristicparam;   ///< Ìn¼ÓÌØÕ÷
    T_OPENAT_BLE_DESCRIPTOR_PARAM   *descriptorparam;   ///< Ìn¼ÓÃèÊö
    T_OPENAT_BLE_BEACON_DATA   *beacondata;   ///< ÉèÖÃbeaconÊý¾Ý
    E_OPENAT_BT_AVRCP_STATE state; ///< ÉèÖÃBTµÄ¸èÇú²¥·Å×´Ì¬
}U_OPENAT_BT_IOTCTL_PARAM;

typedef enum{
    OPENAT_BT_ME_ON_CNF = 1,
    OPENAT_BT_ME_OFF_CNF,
    OPENAT_BT_VISIBILE_CNF,
    OPENAT_BT_HIDDEN_CNF,
    OPENAT_BT_SET_LOCAL_NAME_RES,
    OPENAT_BT_SET_LOCAL_ADDR_RES,
    OPENAT_BT_INQ_DEV_NAME,
    OPENAT_BT_INQ_COMP_CNF,
    OPENAT_BT_INQUIRY_CANCEL,
    OPENAT_BT_DEV_PAIR_COMPLETE,
    OPENAT_BT_DELETE_DEVICE_RES,
    OPENAT_BT_DEV_PIN_REQ,
    OPENAT_BT_SSP_NUM_IND,
    OPENAT_BT_SPP_CONNECT_IND,
    OPENAT_BT_SPP_DISCONNECT_IND,
    OPENAT_BT_SPP_DATA_RECIEVE_IND,
    OPENAT_BT_SPP_DATA_SEND_IND,
	OPENAT_BT_HFP_CONNECT_RES,
	OPENAT_BT_HFP_DISCONNECT_RES,
	OPENAT_BT_REOPEN_IND,				//stop of BT reopen
	OPENAT_BT_REOPEN_ACTION_IND, //start of BT reopen
    OPENAT_BT_HFP_CALLSETUP_OUTGOING,
    OPENAT_BT_HFP_CALLSETUP_INCOMING,
    OPENAT_BT_HFP_CALLSETUP_NONE,
    OPENAT_BT_HFP_RING_INDICATION,
    OPENAT_BT_AVRCP_CONNECT_COMPLETE,
    OPENAT_BT_AVRCP_DISCONNECT_COMPLETE,

    OPENAT_BLE_SET_PUBLIC_ADDR = 51,
    OPENAT_BLE_SET_RANDOM_ADDR,
    OPENAT_BLE_ADD_WHITE_LIST,
    OPENAT_BLE_REMOVE_WHITE_LIST,
    OPENAT_BLE_CLEAR_WHITE_LIST,
    OPENAT_BLE_CONNECT,
    OPENAT_BLE_DISCONNECT,
    OPENAT_BLE_UPDATA_CONNECT,
    OPENAT_BLE_SET_ADV_PARA,
    OPENAT_BLE_SET_ADV_DATA,
    OPENAT_BLE_SET_ADV_ENABLE,
    OPENAT_BLE_SET_ADV_SCAN_RSP,
    OPENAT_BLE_SET_SCAN_PARA,
    OPENAT_BLE_SET_SCAN_ENABLE,
    OPENAT_BLE_SET_SCAN_DISENABLE,
    OPENAT_BLE_SET_SCAN_REPORT,
    OPENAT_BLE_SET_SCAN_FINISH,
    OPENAT_BLE_CONNECT_IND,
    OPENAT_BLE_DISCONNECT_IND,
    OPENAT_BLE_FIND_CHARACTERISTIC_IND,
    OPENAT_BLE_FIND_SERVICE_IND,
    OPENAT_BLE_FIND_CHARACTERISTIC_UUID_IND,
    OPENAT_BLE_RECV_DATA = 100,
}E_OPENAT_BT_EVENT;

typedef VOID (*F_BT_CB)(VOID* param);
void OPENAT_SetBLECallback(F_BT_CB handler);
BOOL OPENAT_OpenBT(E_OPENAT_BT_MODE mode);
BOOL OPENAT_CloseBT(void);
BOOL OPENAT_IotctlBT(E_OPENAT_BT_CMD cmd,U_OPENAT_BT_IOTCTL_PARAM param);
UINT8 OPENAT_GetVisibilityBT(void);
int8 OPENAT_GetAvrcpVolBT(void);

BOOL OPENAT_WriteBLE(UINT16 handle,T_OPENAT_BLE_UUID uuid,char *data,UINT8 len);
BOOL OPENAT_IotctlBLE(UINT16 handle,E_OPENAT_BT_CMD cmd,U_OPENAT_BT_IOTCTL_PARAM param);
BOOL OPENAT_DisconnectBLE(UINT16 handle);
BOOL OPENAT_ConnectBLE(UINT8 addr_type, char *addr);


#endif /* AM_OPENAT_FS_H */


