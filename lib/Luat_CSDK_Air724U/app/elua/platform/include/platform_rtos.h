/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_rtos.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/3/7
 *
 * Description:
 *          luaÆ½Ì¨²ãrtos¿â½Ó¿Ú
 **************************************************************************/

#ifndef __PLATFORM_RTOS_H__
#define __PLATFORM_RTOS_H__

#include "am_openat.h"



typedef enum
{
  MSG_ID_GPS_DATA_IND,
  MSG_ID_RTOS_TOUCH,
  MSG_ID_OPENAT_GPS_OPEN_IND,
  MSG_ID_GPS_OPEN_IND,
  MSG_ID_RTOS_WAIT_MSG_TIMEOUT,
  MSG_ID_RTOS_TIMER,
  MSG_ID_RTOS_UART_RX_DATA,
  MSG_ID_RTOS_UART_TX_DONE,
  MSG_ID_RTOS_KEYPAD,
  MSG_ID_RTOS_INT,
  MSG_ID_RTOS_PMD,   //10
  MSG_ID_RTOS_AUDIO,
  MSG_ID_RTOS_WEAR_STATUS,
  MSG_ID_RTOS_RECORD,
  /*+\new\wj\2020.4.26\ÊµÏÖÂ¼Òô½Ó¿Ú*/
  MSG_ID_ROTS_STRAM_RECORD_IND, //14
  /*-\new\wj\2020.4.26\ÊµÏÖÂ¼Òô½Ó¿Ú*/
  MSG_ID_RTOS_ALARM,
  MSG_ID_APP_MTHL_ACTIVATE_PDP_CNF,
  MSG_ID_APP_MTHL_GET_HOST_BY_NAME_CNF,
  MSG_ID_APP_MTHL_CREATE_CONN_CNF,
  MSG_ID_APP_MTHL_CREATE_SOCK_IND,
  MSG_ID_APP_MTHL_SOCK_SEND_CNF,  //20
  MSG_ID_APP_MTHL_SOCK_SEND_IND,
  MSG_ID_APP_MTHL_SOCK_RECV_IND,
  MSG_ID_APP_MTHL_DEACTIVATE_PDP_CNF,
  MSG_ID_APP_MTHL_SOCK_CLOSE_CNF,
  MSG_ID_APP_MTHL_SOCK_CLOSE_IND,
  MSG_ID_APP_MTHL_DEACTIVATED_PDP_IND,
  MSG_ID_SIM_INSERT_IND,
  
  MSG_ID_TCPIP_PDP_ACTIVATE_CNF,
  MSG_ID_TCPIP_PDP_DEACTIVATE_CNF,
  MSG_ID_TCPIP_PDP_DEACTIVATE_IND,
  MSG_ID_TCPIP_SOCKET_RECV_IND, //30
  MSG_ID_TCPIP_SOCKET_SEND_CNF,  
  
  MSG_ID_TCPIP_SOCKET_CLOSE_CNF,
  
  MSG_ID_TCPIP_SOCKET_CONNECT_CNF,
  MSG_ID_TCPIP_SOCKET_SEND_IND,
  MSG_ID_TCPIP_SOCKET_CLOSE_IND,
/*+\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
  MSG_ID_RTOS_TTSPLY_STATUS,
  MSG_ID_RTOS_TTSPLY_ERROR,
/*-\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/

/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
  MSG_ID_RTOS_MSG_ZBAR,
/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/

  /*what*/
  MSG_ID_RTOS_MSG_WIFI,
  /*-\NEWhenyuan\2020.05.25\wi() ¸»*/
  /*+\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¦ÄÜ*/
  MSG_ID_RTOS_BLUETOOTH,
  /*-\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¹¦ÄÜ*/
  /*+\new\wj\2020.9.19\luaÌn¼Ó¶ú»ú×Ô¶¯¼ì²â¹¦ÄÜ£¬Ìn¼Ó¿ª»úºÍ¶ú»úÉÏ±¨ÏûÏ¢*/
  MSG_ID_RTOS_HEADSET,
  /*-\new\wj\2020.9.19\luaÌn¼Ó¶ú»ú×Ô¶¯¼ì²â¹¦ÄÜ£¬Ìn¼Ó¿ª»úºÍ¶ú»úÉÏ±¨ÏûÏ¢*/
  /*+\ wj \ new \ 2020.10.16 \ ÌNÓRTMP¹¦äüatöçeî*/
  MSG_ID_RTOS_RTMP,
  /*-\ wj \ new \ 2020.10.16 \ ÌNÓRTMP¹¦äüatöçeî*/
  MSG_ID_NULL,

  MSG_ID_MAX = 0x100,
  
}platform_msg_type;


/*what*/
typedef enum PlatformPoweronReasonTag
{
    PLATFORM_POWERON_KEY,
    PLATFORM_POWERON_CHARGER,
    PLATFORM_POWERON_ALARM,
    PLATFORM_POWERON_RESTART,
    PLATFORM_POWERON_OTHER,
    PLATFORM_POWERON_UNKNOWN,
    /*+\ Newreq New \ zohh 2014.6.18 \ ôö¼óªª »*/
    PLATFORM_POWERON_EXCEPTION,
    PLATFORM_POWERON_HOST,
    PLATFORM_POWERON_WATCHDOG,
    /*-\ Newreq New \ zohh 2014.6.18 \ ôö¼óªª »*/

}PlatformPoweronReason;    
/*- \NEW\\\\\\\\ amonge*/

// õõê¼ »¯ ¿¿Éid
typedef enum PlatformRtosModuleTag
{
    RTOS_MODULE_ID_KEYPAD,
	/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
    RTOS_MODULE_ID_ALARM,
	/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/

    RTOS_MODULE_ID_TOUCH,
    // touch screen...

    NumOfRTOSModules
}PlatformRtosModule;

typedef struct KeypadMatrixDataTag
{
    unsigned char       row;
    unsigned char       col;
}KeypadMatrixData;

typedef struct KeypadMsgDataTag
{
    u8   type;  // keypad type
    BOOL bPressed; /*Êç · êêç °'ïâï âï ïï ïï ¢ ¢ ¢*/
    union {
        struct {
            u8 row;
            u8 col;
        }matrix, gpio;
        u16 adc;
    }data;
}KeypadMsgData;

typedef struct TouchMsgDataTag
{
    UINT8   type;  // touch action type
    UINT16   x;
    UINT16   y;
}TouchMsgData;

/*+\NEW\liweiqiang\2013.4.5\Ôö¼Ólua gpio ÖÐ¶ÏÅäÖÃ*/
typedef struct PlatformIntDataTag
{
    elua_int_id             id;
    elua_int_resnum         resnum;
}PlatformIntData;
/*-\NEW\liweiqiang\2013.4.5\Ôö¼Ólua gpio ÖÐ¶ÏÅäÖÃ*/

/*+\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/
#define PLATFORM_BATT_NOT_CHARGING      0
#define PLATFORM_BATT_CHARING           1
#define PLATFORM_BATT_CHARGE_STOP       2

typedef struct PlatformPmdDataTag
{
    BOOL    battStatus;
    BOOL    chargerStatus;
    u8      chargeState;
    u8      battLevel;
    u16     battVolt;
}PlatformPmdData;
/*-\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/

/*+\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/
typedef struct PlatformAudioDataTag
{
    BOOL    playEndInd;
    BOOL    playErrorInd;
}PlatformAudioData;
/*-\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/

typedef struct PlatformRecordDataTag
{
    BOOL    recordEndInd;
    BOOL    recordErrorInd;
}PlatformRecordData;

/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
typedef struct PlatformZbarDataTag
{
    BOOL    result;
    u8 *pType;
    u8 *pData;
}PlatformZbarData;
/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/

/*what*/
typedef struct PlatformWifiDataTag
{
    UINT32    num;
    CHAR* pData;
}PlatformWifiData;
/*-\NEWhenyuan\2020.05.25\wi() ¸»*/
/*+\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¦ÄÜ*/

typedef struct PlatformBluetoothDataTag
{
    unsigned char    eventid;
    char    state;
    UINT8   len;
    unsigned char * pData;
    UINT16 uuid;
    UINT16 handle;
    UINT8 long_uuid[16];
    UINT8 uuid_flag;
}PlatformBluetoothData;
/*-\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¹¦ÄÜ*/



typedef struct PlatformWearStatusDataTag
{
    BOOL    wearStatus;
}PlatformWearStatusData;
/*+\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
//#if defined(__AM_LUA_TTSPLY_SUPPORT__)
typedef struct tagPlatformTtsPlyData
{
    s32    ttsPlyStatusInd;
    s32    ttsPlyErrorInd;
}PlatformTtsPlyData;
//#endif //__AM_LUA_TTSPLY_SUPPORT__
/*-\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
#define PLATFORM_RTOS_WAIT_MSG_INFINITE         (OPENAT_OS_SUSPENDED)

#if 0
typedef enum PlatformMsgIdTag
{
    RTOS_MSG_WAIT_MSG_TIMEOUT, // receive message timeout
    RTOS_MSG_TIMER,
    RTOS_MSG_UART_RX_DATA,
    RTOS_MSG_KEYPAD,
/*+\NEW\liweiqiang\2013.4.5\Ôö¼Ólua gpio ÖÐ¶ÏÅäÖÃ*/
    RTOS_MSG_INT,             
/*-\NEW\liweiqiang\2013.4.5\Ôö¼Ólua gpio ÖÐ¶ÏÅäÖÃ*/
/*+\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/
    RTOS_MSG_PMD,
/*-\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/
/*+\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/
    RTOS_MSG_AUDIO,
/*-\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/

    NumOfMsgIds
}PlatformMsgId;
#endif


typedef struct PlatformGpsDataTag
{
    UINT16 dataMode;
    UINT16 dataLen;
}PlatformGpsData;

typedef struct 
{
    UINT8 success; //0->OK 
}PlatformGpsOpenInd;

typedef struct 
{
    UINT8 inserted;
}PlatformRemSimInsertInd;

typedef struct
{
  UINT8 socket_index;
  BOOL  result;
}PlatformSocketConnectCnf;

typedef struct
{
  UINT8 socket_index;
  INT32 recv_len;
}PlatformSocketRecvInd;

typedef struct
{
  UINT8 socket_index;
  BOOL  result;
}PlatformSocketCloseCnf, PlatformSocketCloseInd;

typedef struct
{
  BOOL  result;
  UINT32 errorCause;
}PlatformPdpActiveCnf;

typedef struct
{
  BOOL  result;
  UINT8 socket_index;
  UINT32 length;
}PlatformSocketSendCnf;
/*+\new\wj\2020.9.19\luaÌn¼Ó¶ú»ú×Ô¶¯¼ì²â¹¦ÄÜ£¬Ìn¼Ó¿ª»úºÍ¶ú»úÉÏ±¨ÏûÏ¢*/
typedef struct
{
	UINT8 msg_id;
	UINT32 param;
}PlatformHeadsetData;
/*-\new\wj\2020.9.19\luaÌn¼Ó¶ú»ú×Ô¶¯¼ì²â¹¦ÄÜ£¬Ìn¼Ó¿ª»úºÍ¶ú»úÉÏ±¨ÏûÏ¢*/
/*+\ wj \ new \ 2020.10.16 \ ÌNÓRTMP¹¦äüatöçeî*/
typedef struct
{
	BOOL result;
	UINT8 result_code;
}PlatformRtmpData;
/*-\ wj \ new \ 2020.10.16 \ ÌNÓRTMP¹¦äüatöçeî*/
typedef union PlatformMsgDataTag
{
    int                 timer_id;
    int                 uart_id;
    KeypadMsgData       keypadMsgData;
    TouchMsgData        touchMsgData;
/*+\NEW\liweiqiang\2013.4.5\Ôö¼Ólua gpio ÖÐ¶ÏÅäÖÃ*/
    PlatformIntData     interruptData;
/*-\NEW\liweiqiang\2013.4.5\Ôö¼Ólua gpio ÖÐ¶ÏÅäÖÃ*/
/*+\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/
    PlatformPmdData     pmdData;
/*-\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/
/*+\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/
    PlatformAudioData   audioData;
/*-\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/

    PlatformRecordData recordData;
	/*+\new\wj\2020.4.26\ÊµÏÖÂ¼Òô½Ó¿Ú*/
	int streamRecordLen;
	/*-\new\wj\2020.4.26\ÊµÏÖÂ¼Òô½Ó¿Ú*/
    PlatformWearStatusData wearStatusData;
/*+\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
    //#if defined(__AM_LUA_TTSPLY_SUPPORT__)
    PlatformTtsPlyData ttsPlyData;
    //#endif //__AM_LUA_TTSPLY_SUPPORT__
/*-\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
    PlatformGpsData    gpsData;
    PlatformGpsOpenInd gpsOpenInd;
    PlatformRemSimInsertInd remSimInsertInd;
    PlatformSocketConnectCnf socketConnectCnf;
    PlatformSocketRecvInd   socketRecvInd;
    PlatformSocketCloseCnf  socketCloseCnf;
    PlatformSocketCloseInd  socketCloseInd;
    PlatformPdpActiveCnf    pdpActiveCnf;
    PlatformSocketSendCnf   socketSendCnf;
	
	/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
    PlatformZbarData      zbarData;
	/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
	/*what*/
	PlatformWifiData      wifiData;
	/*-\NEWhenyuan\2020.05.25\wi() ¸»*/
	/*+\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¦ÄÜ*/
	PlatformBluetoothData      blueData;
	/*-\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¹¦ÄÜ*/
	/*+\new\wj\2020.9.19\luaÌn¼Ó¶ú»ú×Ô¶¯¼ì²â¹¦ÄÜ£¬Ìn¼Ó¿ª»úºÍ¶ú»úÉÏ±¨ÏûÏ¢*/
	PlatformHeadsetData headsetData;
	/*-\new\wj\2020.9.19\luaÌn¼Ó¶ú»ú×Ô¶¯¼ì²â¹¦ÄÜ£¬Ìn¼Ó¿ª»úºÍ¶ú»úÉÏ±¨ÏûÏ¢*/
	/*+\ wj \ new \ 2020.10.16 \ ÌNÓRTMP¹¦äüatöçeî*/
	PlatformRtmpData	  rtmpData;
	/*-\ wj \ new \ 2020.10.16 \ ÌNÓRTMP¹¦äüatöçeî*/
}PlatformMsgData;

/*± Øðëòª½ ¢ Ââã´ò »Çö½e¹¹ ¬ ¬ Ocolocal_para_hdr £ ¬òòony*/
typedef struct  LOCAL_PARAM_STRUCT_TAG
{
    //LOCAL_PARA_HDR
    PlatformMsgData msgData;         
}LOCAL_PARAM_STRUCT;


typedef struct PlatformKeypadInitParamTag
{
    int type;
    struct{
        int inMask;         /* active key in mask */
        int outMask;        /* active key out mask */        
    }matrix;
}PlatformKeypadInitParam;

/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
typedef struct PlatformSetAlarmParamTag
{
    BOOL alarmon;
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 min;
    u8 sec;
}PlatformSetAlarmParam;
/*+\NEW\zhuwangbin\2019.12.10\lua °æ±ëàë»ë»¹ý.*/
#undef ASSERT
#define ASSERT(cOND)	{if (!(cOND)) {platform_assert(__FILE__, (short)__LINE__);}}
/*-\NEW\zhuwangbin\2019.12.10\lua °æ³¾±ë»ë»¹ý.*/
/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/
int platform_rtos_init(void);

/*+\BUG3096\zhuwangbin\2020.9.9.17\ÉµÓ¹ØØúµäµò¦ü¦ü.*/
int platform_rtos_poweroff(int type);
/*-\BUG3096\zhuwangbin\2020.9.17\ÌnµØØØúµäµä¹¦Üke*/

/*+\NEW\liweiqiang\2013.9.7\Ôö¼Órtos.restart½Ó¿Ú*/
int platform_rtos_restart(void);
/*-\NEW\liweiqiang\2013.9.7\Ôö¼Órtos.restart½Ó¿Ú*/

int platform_rtos_init_module(int module, void *pParam);

int platform_rtos_receive(platform_msg_type* msg_id, void **ppMessage, u32 timeout);

void platform_rtos_free_msg(void *msg_body);

int platform_rtos_send(platform_msg_type msg_id,  PlatformMsgData * pMsg);

int platform_rtos_start_timer(int timer_id, int milliSecond);

int platform_rtos_stop_timer(int timer_id);

/*+\NEW\liweiqiang\2013.4.5\Ôö¼Órtos.tick½Ó¿Ú*/
int platform_rtos_tick(void);
/*-\NEW\liweiqiang\2013.4.5\Ôö¼Órtos.tick½Ó¿Ú*/

/*what*/
int platform_get_poweron_reason(void);

int platform_rtos_poweron(int flag);

void platform_poweron_try(void);
/*- \NEW\\\\\\\\ amonge*/
/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
int platform_rtos_setalarm(void *pParam);
/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/

// +panjun, 2015.08.26, Simplify MMI's frame for Video.
int platform_rtos_repoweron(void);
// -panjun, 2015.08.26, Simplify MMI's frame for Video.
char*  platform_base64_encode(const char *str, int length);

extern void LuaChargingPwnOnThanPwnOn(void);

/*+\ New \ Brezen \ 2016.4.25 \ ôö¼obase64½ó¿ú*/  
char* platform_base64_decode (const char *src, int length,int* decodedLen);
/*-\ new \ brezen \ 2016.4.25 \ ôö¼óbase64½ó¿ú*/
//+panjun,160503,Add an API "rtos.disk_free".
long platform_rtos_disk_free(int drvtype);
int platform_rtos_disk_volume(int drvtype);
//-panjun,160503,Add an API "rtos.disk_free".


int platform_make_dir(char *pDir, int len);
int platform_remove_dir(char *pDir, int len);
void platform_rtos_set_time(u32 timestamp);

int platform_rtos_fota_start(void);
int platform_rtos_fota_process(const char* data, int len, int total);
int platform_rtos_fota_end(void);
BOOL platform_rtos_set_trace_port(u8 port, u8 usb_port_diag_output);
u8 platform_rtos_get_trace_port(void);
/*+\NEUW\NEW\Kingeransyaman\2019.4.19\¬*/
int platform_rtos_sendok(char *src);
/*-labs*/
/*+\BUG\wangyuan\2020.07.28 ÁÈ¿Í»§ 1802:*/
int platform_rtos_set_luainfo(char *src);
/*-\BUG\wangyuan\2020.07.28 ÁÈ¿Í»§ 1802:*/
#endif //__PLATFORM_RTOS_H__

