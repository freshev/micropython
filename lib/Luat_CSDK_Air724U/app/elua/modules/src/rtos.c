/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    rtos.c
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/3/7
 *
 * Description:
 *          lua.rtos¿â
 **************************************************************************/

#include <ctype.h>
#include <string.h>
#include <malloc.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "lrotable.h"
#include "lplatform.h"
#include "platform_rtos.h"
#include "platform_malloc.h"
#include "platform_socket.h"
#include "platform_conf.h"

#include "Lstate.h"
#include "am_openat_bluetooth.h"
lua_State *appL;
int appnum;
HANDLE monitorid1=0;
/*+\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¦ÄÜ*/
#define BT_SUCCESS 0
#define BT_ERROR 1

typedef struct rtos_BT_ADDRESS
{
    uint8 addr[6];
} rtos_BT_ADDRESS_t;

typedef struct rtos_ble_scan_report_info
{
    UINT8 name_length;
    UINT8 name[32];
    UINT8 addr_type;
    rtos_BT_ADDRESS_t bdAddress;
    UINT8 event_type;
    UINT8 data_length;
    UINT8 manu_data[32];
    UINT8 manu_len;
    UINT8 raw_data[32];
    UINT8 rssi;
} rtos_ble_scan_report_info_t;
unsigned char  bleRcvBuffer[255]={0};
/*-\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¹¦ÄÜ*/
void setfieldInt(lua_State *L, const char *key, int value)
{
    lua_pushstring(L, key);
    lua_pushinteger(L, value);
    lua_rawset(L, -3);// µ¯³ökey,value ÉèÖÃµ½tableÖÐ
}

static void setfieldBool(lua_State *L, const char *key, int value)
{
    if(value < 0) // invalid value
        return;

    lua_pushstring(L, key);
    lua_pushboolean(L, value);
    lua_rawset(L, -3);// µ¯³ökey,value ÉèÖÃµ½tableÖÐ
}
/*+\ New \ Brezen \ 2016.4.25 \ ôö¼obase64½ó¿ú*/
static void setfieldString(lua_State* L, const char* key, const char* str, const size_t len)
{
  lua_pushstring(L, key);
  lua_pushlstring(L, str, len);
  lua_rawset(L, -3);
}
/*-\ new \ brezen \ 2016.4.25 \ ôö¼óbase64½ó¿ú*/
/*+\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¦ÄÜ*/
static void  handle_BlueTooth(lua_State* L,PlatformBluetoothData BluetoothData)
{
	int ret = 1;
    UINT8 *manu_data = NULL;
    UINT8 *uuid_long = NULL;
    //UINT8 uuid[2] = {0};
	lua_newtable(L);
	setfieldInt(L, "id", MSG_ID_RTOS_BLUETOOTH);
	
	switch(BluetoothData.eventid)
	{
		case OPENAT_BLE_SET_SCAN_REPORT:
            setfieldInt(L, "event", BluetoothData.eventid);
			setfieldInt(L, "result", BluetoothData.state);
			rtos_ble_scan_report_info_t *pstScanRspInfo =(rtos_ble_scan_report_info_t *)BluetoothData.pData;
            char cScanRspaddr[20] = {'\0'};
            sprintf(cScanRspaddr, "%02x:%02x:%02x:%02x:%02x:%02x", pstScanRspInfo->bdAddress.addr[0], pstScanRspInfo->bdAddress.addr[1], pstScanRspInfo->bdAddress.addr[2], pstScanRspInfo->bdAddress.addr[3], pstScanRspInfo->bdAddress.addr[4], pstScanRspInfo->bdAddress.addr[5]);
	
			setfieldString(L, "name", (char *)pstScanRspInfo->name,pstScanRspInfo->name_length);
			setfieldInt(L, "addr_type",pstScanRspInfo->addr_type);				
			setfieldString(L, "addr", cScanRspaddr,strlen(cScanRspaddr));
            setfieldString(L, "manu_data", pstScanRspInfo->manu_data,pstScanRspInfo->manu_len);
            setfieldInt(L, "raw_len",pstScanRspInfo->data_length);	
            setfieldString(L, "raw_data", pstScanRspInfo->raw_data,pstScanRspInfo->data_length);
            setfieldInt(L, "rssi",(int8)pstScanRspInfo->rssi);	
            if(BluetoothData.pData != NULL)
			    OPENAT_free(BluetoothData.pData);
			break;
		case OPENAT_BLE_RECV_DATA:
            setfieldInt(L, "event", BluetoothData.eventid);
            setfieldInt(L, "result", BluetoothData.state);
			/*+\ENEW, administrators ±ÁÁãx´´´±‐offion½é´é´é basis °O´´´© £±£n µn 4µ*/
            // if(BluetoothData.uuid_flag == 0)
            // {
            //     setfieldInt(L, "uuid", BluetoothData.uuid);
            // }
			// else
            // {
            //     setfieldString(L, "uuid", BluetoothData.long_uuid,sizeof(BluetoothData.long_uuid));
            // }
            // setfieldInt(L, "len", BluetoothData.len);

			// /*+\\c.11.259 I.252: 1.3outonªªªªªªªªªªªºº 3.DOºº £ ¬¶en £ £ ¬¶en by ~ е³¶ee ~ ¬¶en eāýónº,000
            // if((BluetoothData.len != 0) && (BluetoothData.pData != NULL))
            // {
            //     memset(bleRcvBuffer,0,255);
            //     memcpy(bleRcvBuffer, BluetoothData.pData, BluetoothData.len);			
            //     setfieldString(L, "data", bleRcvBuffer,BluetoothData.len);
			//     OPENAT_free(BluetoothData.pData);
            // }
			///*-\20.11.259 I.252: 1.3outonªªªªªªªªªªº 3.5º of 3.óº provided £ ¬¶en £º d'¨óenço / 3.ó £enº d'oóóenºò ~
			/*-\NEW cgg\ czg\2020.11.25\BUG 3702: ±ÁÁãx´´´±‐offion½é´é´é basis °O´´´© £±£n µn 4µ*/
			break;
        case OPENAT_BLE_SET_SCAN_ENABLE:
        case OPENAT_BLE_SET_SCAN_DISENABLE:
            setfieldInt(L, "event", OPENAT_BLE_SET_SCAN_ENABLE);
            if(BluetoothData.eventid == OPENAT_BLE_SET_SCAN_ENABLE)
            {
                setfieldInt(L, "enable", 1);
            }
            else
            {
                setfieldInt(L, "enable", 0);
            }
            setfieldInt(L, "result", BluetoothData.state);
            break;
        case OPENAT_BLE_CONNECT_IND:
        case OPENAT_BLE_CONNECT:
            setfieldInt(L, "handle", BluetoothData.handle);
            setfieldInt(L, "event", BluetoothData.eventid);
			setfieldInt(L, "result", BluetoothData.state);
            break;
        case OPENAT_BLE_FIND_SERVICE_IND:
        case OPENAT_BLE_FIND_CHARACTERISTIC_IND:
            setfieldInt(L, "uuid", BluetoothData.uuid);
            setfieldInt(L, "event", BluetoothData.eventid);
			setfieldInt(L, "result", BluetoothData.state);
            break;
        case OPENAT_BLE_FIND_CHARACTERISTIC_UUID_IND:
            setfieldInt(L, "event", BluetoothData.eventid);
			setfieldInt(L, "result", BluetoothData.state);
            if(BluetoothData.uuid_flag == 0)
            {
                setfieldInt(L, "uuid", BluetoothData.uuid);
            }
			else
            {
                setfieldString(L, "uuid", BluetoothData.long_uuid,sizeof(BluetoothData.long_uuid));
            }
            break;
		default:
            setfieldInt(L, "event", BluetoothData.eventid);
			setfieldInt(L, "result", BluetoothData.state);
            break;
	
	}

	
}
/*-\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¹¦ÄÜ*/

static int handle_msg(lua_State *L, platform_msg_type msg_id, LOCAL_PARAM_STRUCT* pMsg)
{
    int ret = 1;
    PlatformMsgData* msgData = &pMsg->msgData;

    switch(msg_id)
    {
    case MSG_ID_RTOS_WAIT_MSG_TIMEOUT:
        lua_pushinteger(L, msg_id);
        // no error msg data.
        break;

    case MSG_ID_RTOS_TIMER:
        lua_pushinteger(L, msg_id);
        lua_pushinteger(L, msgData->timer_id);
        ret = 2;
        break;

    case MSG_ID_RTOS_UART_RX_DATA:
/*+\NEW\zhuwangbin\2018.8.10\Ìn¼ÓOPENAT_DRV_EVT_UART_TX_DONE_INDÉÏ±¨*/
    case MSG_ID_RTOS_UART_TX_DONE:
/*-\NEW\zhuwangbin\2018.8.10\Ìn¼ÓOPENAT_DRV_EVT_UART_TX_DONE_INDÉÏ±¨*/
        lua_pushinteger(L, msg_id);
        lua_pushinteger(L, msgData->uart_id);
        ret = 2;
        break;

    case MSG_ID_RTOS_KEYPAD:
        /*Utpassable · · · couses · · »VIENDIA ¢ OKENCY*/
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        setfieldBool(L, "pressed", msgData->keypadMsgData.bPressed);
        setfieldInt(L, "key_matrix_row", msgData->keypadMsgData.data.matrix.row);
        setfieldInt(L, "key_matrix_col", msgData->keypadMsgData.data.matrix.col);
        break;

/*+\NEW\liweiqiang\2013.4.5\Ôö¼Órtos.tick½Ó¿Ú*/
    case MSG_ID_RTOS_INT:
        OPENAT_print("lua receive MSG_ID_RTOS_INT:%x %x", msgData->interruptData.id, msgData->interruptData.resnum);

        /*Utpassable · · · couses · · »VIENDIA ¢ OKENCY*/
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        setfieldInt(L, "int_id", msgData->interruptData.id);
        setfieldInt(L, "int_resnum", msgData->interruptData.resnum);
        break;
/*-\NEW\liweiqiang\2013.4.5\Ôö¼Órtos.tick½Ó¿Ú*/

/*+\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/
    case MSG_ID_RTOS_PMD:
        /*Utpassable · · · couses · · »VIENDIA ¢ OKENCY*/
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        setfieldBool(L, "present", msgData->pmdData.battStatus);
        setfieldInt(L, "voltage", msgData->pmdData.battVolt);
        setfieldInt(L, "level", msgData->pmdData.battLevel);
        setfieldBool(L, "charger", msgData->pmdData.chargerStatus);
        setfieldInt(L, "state", msgData->pmdData.chargeState);
        break;
/*-\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/

/*+\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/
    case MSG_ID_RTOS_AUDIO:
        /*Utpassable · · · couses · · »VIENDIA ¢ OKENCY*/
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        if(msgData->audioData.playEndInd == TRUE)
            setfieldBool(L,"play_end_ind",TRUE);
        else if(msgData->audioData.playErrorInd == TRUE)
            setfieldBool(L,"play_error_ind",TRUE);
        break;
/*-\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/
/*+\new\wj\2020.9.19\luaÌn¼Ó¶ú»ú×Ô¶¯¼ì²â¹¦ÄÜ£¬Ìn¼Ó¿ª»úºÍ¶ú»úÉÏ±¨ÏûÏ¢*/
	case MSG_ID_RTOS_HEADSET:
		lua_newtable(L);
        setfieldInt(L, "id", msg_id);
		setfieldInt(L, "type",msgData->headsetData.msg_id);
        setfieldInt(L,"param",msgData->headsetData.param);
        break;	
/*-\new\wj\2020.9.19\luaÌn¼Ó¶ú»ú×Ô¶¯¼ì²â¹¦ÄÜ£¬Ìn¼Ó¿ª»úºÍ¶ú»úÉÏ±¨ÏûÏ¢*/
/*+\ wj \ new \ 2020.10.16 \ ÌNÓRTMP¹¦äüatöçeî*/
	case MSG_ID_RTOS_RTMP:
		lua_newtable(L);
        setfieldInt(L, "id", msg_id);
		setfieldBool(L, "result", msgData->rtmpData.result);
		setfieldInt(L, "result_code", msgData->rtmpData.result_code);
        break;
/*-\ wj \ new \ 2020.10.16 \ ÌNÓRTMP¹¦äüatöçeî*/
    case MSG_ID_RTOS_WEAR_STATUS:
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        setfieldBool(L,"wear_status_ind",msgData->wearStatusData.wearStatus);
        break;

    case MSG_ID_RTOS_RECORD:
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);

        if(msgData->recordData.recordEndInd == TRUE)
            setfieldBool(L,"record_end_ind",TRUE);
        else if(msgData->recordData.recordErrorInd == TRUE)
            setfieldBool(L,"record_error_ind",TRUE);
        break;
	/*+\new\wj\2020.4.26\ÊµÏÖÂ¼Òô½Ó¿Ú*/
    case MSG_ID_ROTS_STRAM_RECORD_IND:
		lua_newtable(L);
		setfieldInt(L, "id", msg_id);
		setfieldInt(L, "wait_read_len",msgData->streamRecordLen);
		break;
	/*-\new\wj\2020.4.26\ÊµÏÖÂ¼Òô½Ó¿Ú*/
/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
    case MSG_ID_RTOS_ALARM:
        lua_pushinteger(L, msg_id);
        ret = 1;
        break;
/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/
    case MSG_ID_RTOS_TOUCH:
        /*Utpassable · · · couses · · »VIENDIA ¢ OKENCY*/
#ifdef AM_RH_TP_SUPPORT // ½ "table¸ä³ eaten ± ¨ £ · · ount ± ¨ £ ¬Luaà» ¼ ° »Øêõ £ £ £ cost
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        setfieldInt(L, "type", msgData->touchMsgData.type);
        setfieldInt(L, "x", msgData->touchMsgData.x);
        setfieldInt(L, "y", msgData->touchMsgData.y);
#else
        lua_pushinteger(L, msg_id);
        lua_pushinteger(L, msgData->touchMsgData.type);
        lua_pushinteger(L, msgData->touchMsgData.x);
        lua_pushinteger(L, msgData->touchMsgData.y);
        ret = 4;
#endif
        break;
    //+TTS, panjun 160326.
	/*+\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
   // #if defined(__AM_LUA_TTSPLY_SUPPORT__)
    case MSG_ID_RTOS_TTSPLY_STATUS:
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        setfieldInt(L,"ttsply_status_ind", msgData->ttsPlyData.ttsPlyStatusInd);
        break;
    case MSG_ID_RTOS_TTSPLY_ERROR:
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        setfieldInt(L,"ttsply_error_ind", msgData->ttsPlyData.ttsPlyErrorInd);
        break;
   // #endif //__AM_LUA_TTSPLY_SUPPORT__
   /*-\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
    //-TTS, panjun 160326.

    case MSG_ID_APP_MTHL_ACTIVATE_PDP_CNF:
        ret = platform_on_active_pdp_cnf(L, (PlatformPdpActiveCnf*)pMsg);
        break;

    case MSG_ID_APP_MTHL_CREATE_CONN_CNF:
        ret = platform_on_create_conn_cnf(L, (PlatformSocketConnectCnf *)pMsg);
        break;

    case MSG_ID_APP_MTHL_CREATE_SOCK_IND:
        ret = platform_on_create_sock_ind(L, (mthl_create_sock_ind_struct *)pMsg);
        break;

    case MSG_ID_APP_MTHL_SOCK_SEND_CNF:
        ret = platform_on_send_data_cnf(L, (PlatformSocketSendCnf *)pMsg);
        break;


    case MSG_ID_APP_MTHL_SOCK_SEND_IND:
        ret = platform_on_send_data_ind(L, (mthl_send_data_ind_struct *)pMsg);
        break;


    case MSG_ID_APP_MTHL_SOCK_RECV_IND:
        ret = platform_on_recv_data_ind(L, (PlatformSocketRecvInd *)pMsg);
        break;


    case MSG_ID_APP_MTHL_DEACTIVATE_PDP_CNF:
        ret = platform_on_deactivate_pdp_cnf(L, (mthl_deactivate_pdp_cnf_struct *)pMsg);
        break;


    case MSG_ID_APP_MTHL_SOCK_CLOSE_CNF:
        ret = platform_on_sock_close_cnf(L, (PlatformSocketCloseCnf*)pMsg);
        break;


    case MSG_ID_APP_MTHL_SOCK_CLOSE_IND:
        ret = platform_on_sock_close_ind(L, (PlatformSocketCloseInd*)pMsg);
        break;


    case MSG_ID_APP_MTHL_DEACTIVATED_PDP_IND:
        ret = platform_on_deactivate_pdp_ind(L, (mthl_deactivate_pdp_ind_struct *)pMsg);
        break;

    case MSG_ID_GPS_DATA_IND:
#if 0
        lua_newtable(L);
        setfieldInt(L, "id", msg_id);
        setfieldInt(L, "type", msgData->gpsData.dataMode);
        setfieldBool(L,"length",msgData->gpsData.dataLen);
#endif
        lua_pushinteger(L, msg_id);
        lua_pushinteger(L, msgData->gpsData.dataMode);
        lua_pushinteger(L, msgData->gpsData.dataLen);
        ret = 3;
        break;
    case MSG_ID_GPS_OPEN_IND:
        lua_pushinteger(L, msg_id);
        lua_pushinteger(L, msgData->gpsOpenInd.success);
        ret = 2;
        break;
/*+: \ New \ abysses \ 2016.10.13 \ Ö§³ösimwards*/
    case MSG_ID_SIM_INSERT_IND:
        lua_pushinteger(L, msg_id);
        lua_pushinteger(L, msgData->remSimInsertInd.inserted);
        ret = 2;
        break;
/*-: \ New \ abysses \ 2016.10.13 \ Ö§³ösimwards*/
	/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
	case MSG_ID_RTOS_MSG_ZBAR:

		lua_newtable(L);

        setfieldInt(L, "id", msg_id);
        setfieldBool(L, "result", msgData->zbarData.result);
        if (msgData->zbarData.pType)
        {
            setfieldString(L, "type", (const char*)msgData->zbarData.pType, strlen((const char *)msgData->zbarData.pType));
            platform_free(msgData->zbarData.pType);
        }
        if (msgData->zbarData.pData)
        {
            setfieldString(L, "data", (const char*)msgData->zbarData.pData, strlen((const char *)msgData->zbarData.pData));
            platform_free(msgData->zbarData.pData);
        }
		break;
	/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/

		/*what*/
	case MSG_ID_RTOS_MSG_WIFI:

		lua_newtable(L);

        setfieldInt(L, "id", msg_id);
		setfieldInt(L, "cnt", msgData->wifiData.num);
		if(msgData->wifiData.num > 0)
		{
        	setfieldString(L, "info", (const char*)msgData->wifiData.pData, strlen((const char *)msgData->wifiData.pData));
			platform_free(msgData->wifiData.pData);
		}
		break;
		/*-\NEWhenyuan\2020.05.25\wi() ¸»*/
	/*+\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¦ÄÜ*/
	case MSG_ID_RTOS_BLUETOOTH:		
		handle_BlueTooth(L,msgData->blueData);
        break;
	/*-\NEW\liangjian\2020.09.10\lua Ìn¼Ó À¶ÑÀ¹¹¦ÄÜ*/
    default:
        ret = 0;
        break;
    }

    return ret;
}

static int l_rtos_receive(lua_State *L) 		/* rtos.receive() */
{
    u32 timeout = luaL_checkinteger( L, 1 );
    LOCAL_PARAM_STRUCT* pMsg = NULL;
    platform_msg_type msg_id;

/*what*/
    static BOOL firstRecv = TRUE;
    int ret = 0;

    if(firstRecv)
    {
        // MINE μóò »'î½óails Italian ¢¢ ³èòªuï¯uï · <br>
        firstRecv = FALSE;
        platform_poweron_try();
    }
/*- \NEW\\\\\\\\ amonge*/

    if(platform_rtos_receive(&msg_id, (void**)&pMsg, timeout) != PLATFORM_OK)
    {
        return luaL_error( L, "rtos.receive error!" );
    }

    ret = handle_msg(L, msg_id, pMsg);

    if(pMsg)
    {
    /*+\NEW\liweiqiang\2013.12.6\libc malloc×ßdlmallocÍ¨µÀ*/
        // ¸ ¸ ¸ ¸ ¸¸ - äuæojo gi²eo ameri amero erem³аé Foëen ¹frereplathen as her statues this eight
        platform_rtos_free_msg(pMsg);
    /*-\NEW\liweiqiang\2013.12.6\libc malloc×ßdlmallocÍ¨µÀ*/
    }

    return ret;
}

static int l_rtos_sleep(lua_State *L)   /* rtos.sleep()*/
{
    int ms = luaL_checkinteger( L, 1 );

    platform_os_sleep(ms);

    return 0;
}

static int l_rtos_timer_start(lua_State *L)
{
    int timer_id = luaL_checkinteger(L,1);
    int ms = luaL_checkinteger(L,2);
    int ret;

    ret = platform_rtos_start_timer(timer_id, ms);

    lua_pushinteger(L, ret);

    return 1;
}

static int l_rtos_timer_stop(lua_State *L)
{
    int timer_id = luaL_checkinteger(L,1);
    int ret;

    ret = platform_rtos_stop_timer(timer_id);

    lua_pushinteger(L, ret);

    return 1;
}

static int l_rtos_init_module(lua_State *L)
{
    int module_id = luaL_checkinteger(L, 1);
    int ret;

    switch(module_id)
    {
    case RTOS_MODULE_ID_KEYPAD:
        {
            PlatformKeypadInitParam param;

            int type = luaL_checkinteger(L, 2);
            int inmask = luaL_checkinteger(L, 3);
            int outmask = luaL_checkinteger(L, 4);

            param.type = type;
            param.matrix.inMask = inmask;
            param.matrix.outMask = outmask;

            ret = platform_rtos_init_module(RTOS_MODULE_ID_KEYPAD, &param);
        }
        break;
/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
    case RTOS_MODULE_ID_ALARM:
        {
            ret = platform_rtos_init_module(RTOS_MODULE_ID_ALARM, NULL);
        }
		break;
/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/
    case RTOS_MODULE_ID_TOUCH:
        {
            ret = platform_rtos_init_module(RTOS_MODULE_ID_TOUCH, NULL);
        }
		break;
    default:
        return luaL_error(L, "rtos.init_module: module id must < %d", NumOfRTOSModules);
        break;
    }

    lua_pushinteger(L, ret);

    return 1;
}

/*what*/
static int l_rtos_poweron_reason(lua_State *L)
{
    lua_pushinteger(L, platform_get_poweron_reason());
    return 1;
}

static int l_rtos_poweron(lua_State *L)
{
    int flag = luaL_checkinteger(L, 1);
    platform_rtos_poweron(flag);
    return 0;
}
/*- \NEW\\\\\\\\ amonge*/

/*+\NEW\2015.4.17\Wâ µâ ²³úúúóèóâ³´³LY³Led³L.*/
static int l_rtos_repoweron(lua_State *L)
{
    platform_rtos_repoweron();
    return 0;
}
/*-\NEW\2015.4.17\Wâ µâ ²³ú³´³²³LY³Led³L.*/

static int l_rtos_poweroff(lua_State *L)
{
/*+\BUG3096\zhuwangbin\2020.9.9.17\ÉµÓ¹ØØúµäµò¦ü¦ü.*/
	int n,type = 0;
	
	n = lua_gettop(L);
    if (n >= 1)
    {
      type = luaL_checkinteger(L, 1);
    }

	platform_rtos_poweroff(type);
/*-\BUG3096\zhuwangbin\2020.9.17\ÌnµØØØúµäµä¹¦Üke*/

	return 0;
}

/*+\NEW\liweiqiang\2013.9.7\Ôö¼Órtos.restart½Ó¿Ú*/
static int l_rtos_restart(lua_State *L)
{
	platform_rtos_restart();
	return 0;
}
/*-\NEW\liweiqiang\2013.9.7\Ôö¼Órtos.restart½Ó¿Ú*/

/*+\NEW\liweiqiang\2013.4.5\Ôö¼Órtos.tick½Ó¿Ú*/
static int l_rtos_tick(lua_State *L)
{
    lua_pushinteger(L, platform_rtos_tick());
    return 1;
}
static int l_rtos_sms_is_ready(lua_State *L)
{
    lua_pushinteger(L, platform_rtos_sms_is_ready());
    return 1;
}
/*-\NEW\liweiqiang\2013.4.5\Ôö¼Órtos.tick½Ó¿Ú*/
static void app_monitor_call()
{
    OPENAT_print("app_monitor_call");
    lua_getglobal(appL, "check_app"); /* function to be called */
    lua_call(appL, 0, 0);
    lua_sethook(appL,NULL,0,0);  /* close hooks */
}

/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
static void app_monitor(void *pParameter)
{
    int   mask = LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT;
    lua_sethook(appL, app_monitor_call, mask, 1);  /* set hooks */
}
/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
static int l_rtos_app_monitor(lua_State *L)
{
    OPENAT_print("l_rtos_app_monitor");
    appL=L;
    appnum=luaL_checkinteger(appL, 1);
   if (monitorid1==0)
    {
      monitorid1= OPENAT_create_timerTask(app_monitor, NULL);
    }
    OPENAT_start_timer(monitorid1, appnum);
    return 0;
}

/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
static int l_rtos_setalarm(lua_State *L)
{
    PlatformSetAlarmParam alarmparam;

    alarmparam.alarmon = luaL_checkinteger(L, 1);
    alarmparam.year = luaL_checkinteger(L, 2);
    alarmparam.month = luaL_checkinteger(L, 3);
    alarmparam.day = luaL_checkinteger(L, 4);
    alarmparam.hour = luaL_checkinteger(L, 5);
    alarmparam.min = luaL_checkinteger(L, 6);
    alarmparam.sec = luaL_checkinteger(L, 7);

    OPENAT_print("l_rtos_setalarm:%d %d %d %d %d %d %d",
        alarmparam.alarmon,
        alarmparam.year,
        alarmparam.month,
        alarmparam.day,
        alarmparam.hour,
        alarmparam.min,
        alarmparam.sec);
    platform_rtos_setalarm(&alarmparam);
    return 0;
}



static int l_rtos_base64_encode(lua_State *L)
{
    size_t  len      = 0;
    char* string     = (char*)luaL_checklstring(L, 1, &len);
    char* endcode_string;

    endcode_string = platform_base64_encode(string, len);

    lua_pushstring(L, endcode_string);

    L_FREE(endcode_string);
    return 1;
}
/*+\ New \ Brezen \ 2016.4.25 \ ôö¼obase64½ó¿ú*/
/* lua exsample
local function base64()
	local enc_str = rtos.base64_encode("1234556661")
	print('enc_str',enc_str)
	local dec_res = rtos.base64_decode(enc_str)
	print('dec_res len=',dec_res.len)
	print('dec_res string=',dec_res.str)
end
*/
static int l_rtos_base64_decode(lua_State *L)
{
    size_t  len      = 0;
    size_t  decoded_len = 0;
    char* string     = (char*)luaL_checklstring(L, 1, &len);
    char* endcode_string;

    endcode_string = platform_base64_decode(string, len, &decoded_len);

    lua_newtable(L);
    setfieldInt(L, "len", decoded_len);
    setfieldString(L, "str", endcode_string, decoded_len);

    L_FREE(endcode_string);
    return 1;
}
/*-\ new \ brezen \ 2016.4.25 \ ôö¼óbase64½ó¿ú*/

//+panjun,160503,Add an API "rtos.disk_free".
static int l_rtos_disk_free(lua_State *L)
{
    int drvtype = luaL_optinteger(L,1,0);

    lua_pushinteger(L, platform_rtos_disk_free(drvtype));
    return 1;
}

static int l_rtos_disk_volume(lua_State *L)
{
    int drvtype = luaL_optinteger(L,1,0);

    lua_pushinteger(L, platform_rtos_disk_volume(drvtype));
    return 1;
}
//-panjun,160503,Add an API "rtos.disk_free".

static int l_rtos_keypad_state_get(lua_State *L)
{
    lua_pushinteger(L, platform_lua_get_keypad_is_press());
    return 1;
}

static int l_rtos_keypad_init_over(lua_State *L)
{
  paltform_is_lua_init_end();
}
static int l_get_env_usage(lua_State *L)
{
	lua_pushinteger(L, 50);
	return 1;
}
static int l_get_version(lua_State *L)
{
	/*+\NEW\zhuwangbin\2019.12.10\lua °æ±ëàë»ë»¹ý.*/
 	char *ver = OPENAT_getProdOrVer();
	/*-\NEW\zhuwangbin\2019.12.10\lua °æ³¾±ë»ë»¹ý.*/

	lua_pushlstring(L, ver, strlen(ver));

	return 1;
}

static int l_rtos_get_build_time(lua_State *L)
{
	#define BUILD_TIME_MAX_LEN 50
 	char build_time[BUILD_TIME_MAX_LEN+1] = {0};

 	sprintf(build_time, "%s %s", __DATE__, __TIME__);

	lua_pushlstring(L, build_time, strlen(build_time));

	return 1;
}

/*what*/
static int l_set_trace(lua_State *L)
{
    u32 print = luaL_optinteger(L, 1, 0);
	u32 port = luaL_optinteger(L, 2, 0);

	lua_pushboolean(L, platform_rtos_set_trace(print,port));

    return 1;
}
/*-\NEWhenyouanean\20.40.4.14\Nnn the Linked_üTros. Bears_the_the-art_Vi*/

static int l_rtos_sys32k_clk_out(lua_State *L)
{
    int enable = luaL_checkinteger(L, 1);
    // TODO:
    return 0;
}
static int l_rtos_send_event(lua_State *L)
{
    u32 event = luaL_checkinteger(L, 1);
    printf("Detected Event 0x%08x\n", event);
    return 0;
}
/*+\new\liangjian\2020.06.17\Ôö¼Ó¶ÁÈ¡SD ¿¨¿Õ¼ä·½·¨*/
/*+\new\liangjian\2020.06.22\Ôö¼ÓKb ÏÔÊ¾*/
static int l_get_fs_free_size(lua_State *L)
{
  // TODO
  	int n,isSD ;
    isSD  = 0;
	int formatKb=0;
	UINT32 size = 0;
  	n = lua_gettop(L);
    if (n >= 1)
    {
	    isSD = luaL_checkinteger(L, 1);
    	if(n > 1)
		{
		if(luaL_checkinteger(L, 2) == 1)
			formatKb = 1;
		}
    }
	size  =  platform_fs_get_free_size(isSD,formatKb);
	lua_pushinteger(L, size);


	return 1;
}

static int l_get_fs_total_size(lua_State *L)
{
  // TODO
  	int n,isSD ;
    isSD  = 0;
	int formatKb=0;
	UINT32 size = 0;
  	n = lua_gettop(L);
    if (n >= 1)
    {
    	isSD = luaL_checkinteger(L, 1);
 	 	if(n > 1)
 	 	{
			if(luaL_checkinteger(L, 2) == 1)
				formatKb = 1;
 	 	}
    }


	size  =  platform_fs_get_total_size(isSD,formatKb);
	lua_pushinteger(L, size);

	return 1;
}
/*-\new\liangjian\2020.06.22\Ôö¼ÓKb ÏÔÊ¾*/
/*+\new\liangjian\2020.06.17\Ôö¼Ó¶ÁÈ¡SD ¿¨¿Õ¼ä·½·¨*/

static int l_make_dir(lua_State *L)
{
    size_t  len = 0;
    char* pDir = (char*)luaL_checklstring(L, 1, &len);

    lua_pushboolean(L, platform_make_dir(pDir, len));
    return 1;
}

static int l_remove_dir(lua_State *L)
{
    size_t  len = 0;
    char* pDir = (char*)luaL_checklstring(L, 1, &len);

    lua_pushboolean(L, platform_remove_dir(pDir, len));
    return 1;
}
static int l_get_fs_flush_status(lua_State *L)
{
    lua_pushboolean(L, 1);
    return 1;
}
static int l_rtos_set_time(lua_State *L){
    platform_rtos_set_time(luaL_checknumber(L, 1));
    return 0;
}


static int l_rtos_fota_start(lua_State *L){
    lua_pushinteger(L, platform_rtos_fota_start());
    return 1;
}
static int l_rtos_fota_process(lua_State *L){

    size_t  len = 0;
    int total;

    char* data = (char*)luaL_checklstring(L, 1, &len);
    total = luaL_checkinteger(L, 2);

    lua_pushinteger(L, platform_rtos_fota_process(data, len, total));
    return 1;
}

static int l_rtos_fota_end(lua_State *L){
    lua_pushinteger(L, platform_rtos_fota_end());
    return 1;
}

static int l_rtos_get_fatal_info(lua_State *L){
    char fatalinfo[150+1] = {0};
    int len = platform_rtos_get_fatal_info(fatalinfo,150);
    lua_pushlstring(L, fatalinfo, len);
    return 1;
}

static int l_rtos_remove_fatal_info(lua_State *L){
    lua_pushboolean(L, platform_rtos_remove_fatal_info()==0);
    return 1;
}

static int l_rtos_set_trace_port(lua_State *L){
    int port = luaL_checkinteger( L, 1 );
    int usb_port_diag_output = luaL_optinteger(L, 2, 0);

    lua_pushboolean(L, platform_rtos_set_trace_port(port,usb_port_diag_output));
    return 1;
}

static int l_rtos_get_trace_port(lua_State *L){
    lua_pushinteger(L, platform_rtos_get_trace_port());
    return 1;
}


static int l_rtos_toint64(lua_State *L) {
    unsigned char result[8] = {0};
    unsigned char temp[2] = {0};
    const char *str, *endian;
    size_t len;
    long long n = 0;
    int i = 0;
    long long exp[19] =
    {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000,
        10000000000,
        100000000000,
        1000000000000,
        10000000000000,
        100000000000000,
        1000000000000000,
        10000000000000000,
        100000000000000000,
    };

    str = luaL_checklstring(L, 1, &len);
    endian = luaL_optstring(L, 2, "little");

    for  (i=0; i<len; i++)
    {
        temp[0] = str[i];
        n +=((long long)atoi(temp))*(long long)(exp[len-1-i]);
        //printf("_int64 str[%d]=%d,n=%ld\n",i,atoi(temp),n);
    }

    for (i=0; i<8; i++)
    {
        if(strcmp(endian,"big")==0)
        {
            result[i] = *((unsigned char*)(&n)+7-i);
        }
        else
        {
            result[i] = *((unsigned char*)(&n)+i);
        }
        //printf("_int64 n64[%d]=%x\n",i,*((unsigned char*)(&n)+i));
    }

    lua_pushlstring(L, (const char *)result, 8);
    return 1;
}


static int l_rtos_compare_distance(lua_State *L) {
    int lat1 = luaL_checkinteger(L, 1);
    int lng1 = luaL_checkinteger(L, 2);
    int lat2 = luaL_checkinteger(L, 3);
    int lng2 = luaL_checkinteger(L, 4);
    int radius = luaL_checkinteger(L, 5);
    int result = 0;
    unsigned char int64Str[8] = {0};
    int i = 0;
    long long radius_2 =  (long long)((long long)radius* (long long)radius);

    long long distance = (long long)lat2-(long long)lat1;
    long long distance2 = (long long)lng2-(long long)lng1;
    distance = (long long)((((long long)distance*(long long)111-((long long)distance*(long long)111%(long long)100)))/(long long)100);
    distance = (long long)distance * (long long)distance;
    distance =  (long long)distance+ (long long)distance2* (long long)distance2;

    for (i=0; i<8; i++)
    {
        int64Str[i] = *((unsigned char*)(&distance)+i);
        printf("l_rtos_compare_distance distance_2[%d]=%x\n",i,int64Str[i]);
    }

    for (i=0; i<8; i++)
    {
        int64Str[i] = *((unsigned char*)(&radius_2)+i);
        printf("l_rtos_compare_distance radius_2[%d]=%x\n",i,int64Str[i]);
    }

    //printf("l_rtos_compare_distance distance_2=%ldn", (long long)distance);
    //printf("l_rtos_compare_distance radius_2=%ld\n", (long long)((long long)radius* (long long)radius));

    if((long long)distance >  (long long)radius_2)
    {
        result = 1;
    }
    else if((long long)distance < (long long)radius_2)
    {
        result = -1;
    }

    lua_pushinteger(L, result);
    return 1;
}
/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/

/*+\NEUW\NEW\Kingeransyaman\2019.4.19\¬*/
static int l_rtos_sendok(lua_State *L)
{
    size_t  len      = 0;
    char* string     = (char*)luaL_checklstring(L, 1, &len);

    platform_rtos_sendok(string);
    return 0;
}
/*-labs*/

/*+\ New \ Hedonghao \ 2020.4.10 \ nnladuèn10 · ½ó¿ú*/
static int l_open_SoftDog(lua_State *L)
{
    u32 timeout = luaL_optinteger(L, 1, 0);

	lua_pushboolean(L, platform_rtos_open_SoftDog(timeout));
    return 1;
}

static int l_eat_SoftDog(lua_State *L)
{
	platform_rtos_eat_SoftDog();
    return 1;
}

static int l_close_SoftDog(lua_State *L)
{
	platform_rtos_close_SoftDog();
    return 1;
}
/*-\ New \ Hedonghao \ 2020.4.10 \ nnladuèn10 · ½ó¿ú*/

	/*+\NEW\zhuwangbin\2020.6.28\Ìn¼ÓÐ´Âë¹¦ÄÜ*/
#ifdef AM_LUA_POC_SUPPORT
static int l_rtos_flash_memcpy(lua_State *L)
{
    int addr = luaL_checkinteger(L, 1);
    int len = luaL_checkinteger( L, 2 );
    u8* data = malloc(len+1);

	if (data)
	{
		memset(data, 0, sizeof(data));
		memcpy(data, addr, len);
	}
 	else
 	{
		len = 0;
	}
    luaL_Buffer b;
    luaL_buffinit( L, &b );
    luaL_addlstring(&b, data, len);
    luaL_pushresult( &b );

	if (data)
	{
    	free(data);
	}

    return 1;
}

/*+\BUG3109\zhuwgbin\2020.9.21\¶Ô²²²«ïçÐ¿«” ú×´âugh«ug´´´´USB´USB´UR²²culu´POCÑughÑugg*/
static int l_rtos_poc_ota(lua_State *L){
	
	extern BOOL platform_rtos_poc_ota(void);

	lua_pushinteger(L, platform_rtos_poc_ota());
	return 1;
}

static int l_rtos_poc_param_erase(lua_State *L){
	
	extern int platform_rtos_poc_flash_erase(void);

	lua_pushinteger(L, platform_rtos_poc_flash_erase());
	return 1;
}


static int l_rtos_poc_param_write(lua_State *L)
{
	int offset,data,size;
    char* buf;
    
	offset = luaL_checkinteger(L, 1);
    buf = lua_tolstring( L, 2, &size );

	
	extern int platform_rtos_poc_flash_write(UINT32 offset, UINT32 size, UINT8 * buf);
	
	lua_pushinteger(L, platform_rtos_poc_flash_write(offset, size, buf));
	
	return 1;
}

static int l_rtos_poc_param_read(lua_State *L)
{
	int offset,data,size;
    
	offset = luaL_checkinteger(L, 1);
	size = luaL_checkinteger(L, 2);

	char* buf = malloc(size);	

	extern int platform_rtos_poc_flash_read(UINT32 offset, UINT32 size, UINT8 * buf);
	platform_rtos_poc_flash_read(offset, size, buf);
	
	luaL_Buffer b;

	luaL_buffinit( L, &b );
	luaL_addlstring(&b, buf, size);
	luaL_pushresult( &b );

	free(buf);
	
	return 1;
}




/*-\BUG3109\zhuwgbin\2020.9.21\¶ú²²«ïÐÐ¿¤" ú×´âugh«ug´´´´USB´USB´UR²²culu´POCÑughÑugg*/

#endif
	/*+\NEW\zhuwangbin\2020.6.28\Ìn¼ÓÐ´Âë¹¦ÄÜ*/
/*+\BUG\wangyuan\2020.07.28 ÁÈ¿Í»§ 1802:*/
static int l_rtos_set_luainfo(lua_State *L)
{
    size_t  len      = 0;
    char* string     = (char*)luaL_checklstring(L, 1, &len);
	
    platform_rtos_set_luainfo(string);	
    return 0;
}
/*-\BUG\wangyuan\2020.07.28 ÁÈ¿Í»§ 1802:*/

/*+\new\wj\luaÌn¼ÓÈÈ²å°Î½Ó¿Ú*/
static int l_rtos_notify_sim_detect(lua_State * L)
{
	int simNum = luaL_checkinteger(L, 1);
	int connect = luaL_checkinteger(L,2);
	platform_rtos_notify_sim_detect(simNum,connect == 0 ? FALSE : TRUE);
	return 0;
}
/*-\new\wj\luaÌn¼ÓÈÈ²å°Î½Ó¿Ú*/

/*+\bug\rww\2020.9.22\Ìn¼Órtos.setTransData*/
// static int l_rtos_set_trans_data(lua_State *L)
// {
//     int len = luaL_checkinteger(L, 1);
//     char *data = luaL_checklstring(L, 2, NULL);
//     void *buf = malloc(len);
//     if (!buf)
//         luaL_error(L, "alloc fail");
//     memcpy(buf, data, len);
//     platform_rtos_set_trans_data(len, buf);
//     return 0;
// }
/*-\bug\rww\2020.9.22\Ìn¼Órtos.setTransData*/

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE rtos_map[] =
{
    { LSTRKEY( "init_module" ),  LFUNCVAL( l_rtos_init_module ) },
/*what*/
    { LSTRKEY( "poweron_reason" ),  LFUNCVAL( l_rtos_poweron_reason ) },
    { LSTRKEY( "poweron" ),  LFUNCVAL( l_rtos_poweron ) },
/*+\NEW\2015.4.17\Wâ µâ ²³úúúóèóâ³´³LY³Led³L.*/
    { LSTRKEY( "repoweron" ),  LFUNCVAL( l_rtos_repoweron ) },
/*-\NEW\2015.4.17\Wâ µâ ²³ú³´³²³LY³Led³L.*/
/*- \NEW\\\\\\\\ amonge*/
    { LSTRKEY( "poweroff" ),  LFUNCVAL( l_rtos_poweroff ) },
/*+\NEW\liweiqiang\2013.9.7\Ôö¼Órtos.restart½Ó¿Ú*/
    { LSTRKEY( "restart" ),  LFUNCVAL( l_rtos_restart ) },
/*-\NEW\liweiqiang\2013.9.7\Ôö¼Órtos.restart½Ó¿Ú*/
    { LSTRKEY( "receive" ),  LFUNCVAL( l_rtos_receive ) },
    // {Lstrkey ("Send"), Lfuncval (L_RTOS_SEND)}, // ôý² »
    { LSTRKEY( "sleep" ), LFUNCVAL( l_rtos_sleep ) },
    { LSTRKEY( "timer_start" ), LFUNCVAL( l_rtos_timer_start ) },
    { LSTRKEY( "timer_stop" ), LFUNCVAL( l_rtos_timer_stop ) },
/*+\NEW\liweiqiang\2013.4.5\Ôö¼Órtos.tick½Ó¿Ú*/
    { LSTRKEY( "tick" ), LFUNCVAL( l_rtos_tick ) },
/*-\NEW\liweiqiang\2013.4.5\Ôö¼Órtos.tick½Ó¿Ú*/
    { LSTRKEY( "sms_is_ready" ), LFUNCVAL( l_rtos_sms_is_ready ) },
    { LSTRKEY( "app_monitor" ), LFUNCVAL( l_rtos_app_monitor ) },
/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
    { LSTRKEY( "set_alarm" ), LFUNCVAL( l_rtos_setalarm ) },
/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/

    { LSTRKEY( "base64_encode" ), LFUNCVAL( l_rtos_base64_encode ) },
/*+\ New \ Brezen \ 2016.4.25 \ ôö¼obase64½ó¿ú*/
    { LSTRKEY( "base64_decode"),  LFUNCVAL( l_rtos_base64_decode ) },
/*-\ new \ brezen \ 2016.4.25 \ ôö¼óbase64½ó¿ú*/
    { LSTRKEY( "disk_free" ), LFUNCVAL( l_rtos_disk_free ) },
    { LSTRKEY( "disk_volume" ), LFUNCVAL( l_rtos_disk_volume ) },
    { LSTRKEY( "keypad_state" ), LFUNCVAL( l_rtos_keypad_state_get ) },
    { LSTRKEY( "keypad_init_end" ), LFUNCVAL( l_rtos_keypad_init_over ) },

	{ LSTRKEY( "get_env_usage" ), LFUNCVAL( l_get_env_usage ) },
/*-\NEW\zhuwangbin\2017.2.12\Ìn¼Ó°æ±¾²éÑ¯½Ó¿¿ Ú*/
    { LSTRKEY( "get_version" ), LFUNCVAL( l_get_version ) },
/*-\NEW\zhuwangbin\2017.2.12\Ìn¼Ó°æ±¾²éÑ¯½Ó¿¿ Ú*/
    /*begin\NE\zhutianhua\2017.28.28:4 addi.s_traces_trace´¿à¿â ²³âLuaLous*/
    { LSTRKEY( "set_trace" ), LFUNCVAL( l_set_trace ) },
    // 9501gPs Iona © 32kê ± Öó
    {LSTRKEY( "sys32k_clk_out" ), LFUNCVAL( l_rtos_sys32k_clk_out ) },
    /*The \NEW\thager\2017.2.28 14:4\d Migmattos.stand_tra¬ izé´´tua and 11 to 11.*/
    { LSTRKEY( "sendevent" ), LFUNCVAL( l_rtos_send_event ) },
    /*begin\NEW\zhutianhua\2017.9.5 30:53\Ôö¼Óget_fs_free_size½Ó¿Ú*/
    { LSTRKEY( "get_fs_free_size" ), LFUNCVAL( l_get_fs_free_size ) },
    /*end\NEW\zhutianhua\2017.9.5 30:53\Ôö¼Óget_fs_free_size½Ó¿Ú*/
	/*Begin \ new \ liangjian \ 2020.6.16 \ Ôö¼óget_fs_total_size½ó¿ú*/
    { LSTRKEY( "get_fs_total_size" ), LFUNCVAL( l_get_fs_total_size ) },
    /*end\NEW\liangjian\2020.6.16 \Ôö¼Óget_fs_total_size½Ó¿Ú*/
    /*Begin \ new \ zlutianhua \ 2017.9.27 19: 41 \ ôöclemake_dir½ó¿ú*/
    { LSTRKEY( "make_dir" ), LFUNCVAL( l_make_dir) },
    { LSTRKEY( "remove_dir" ), LFUNCVAL( l_remove_dir) },
    /*End \ new \ zlutianhua \ 2017.9.27 19: 41 \ ôöclemake_dir½ó¿ú*/
    { LSTRKEY( "get_fs_flush_status" ), LFUNCVAL( l_get_fs_flush_status) },
/*+\NEW\shenyuanyuan\2017.10.10\Ìn¼ÓÄÖÖÓ½Ó¿Ú*/
    { LSTRKEY( "set_alarm" ), LFUNCVAL( l_rtos_setalarm ) },
    /*-\NEW\shenyuanyuan\2017.10.10\Ìn¼ÓÄÖÖÓ½Ó¿Ú*/
    { LSTRKEY( "set_time" ), LFUNCVAL( l_rtos_set_time ) },
    { LSTRKEY( "fota_start" ), LFUNCVAL( l_rtos_fota_start )},
    { LSTRKEY( "fota_process" ), LFUNCVAL( l_rtos_fota_process )},
    { LSTRKEY( "fota_end" ), LFUNCVAL( l_rtos_fota_end )},
    { LSTRKEY( "get_fatal_info" ), LFUNCVAL( l_rtos_get_fatal_info )},
    { LSTRKEY( "remove_fatal_info" ), LFUNCVAL( l_rtos_remove_fatal_info )},
    { LSTRKEY( "set_trace_port" ), LFUNCVAL( l_rtos_set_trace_port )},
    { LSTRKEY( "get_trace_port" ), LFUNCVAL( l_rtos_get_trace_port )},
    { LSTRKEY( "get_build_time" ), LFUNCVAL( l_rtos_get_build_time )},
	{ LSTRKEY( "toint64" ),  LFUNCVAL( l_rtos_toint64 ) },
    { LSTRKEY( "compare_distance" ),  LFUNCVAL( l_rtos_compare_distance ) },
/*+\NEUW\NEW\Kingeransyaman\2019.4.19\¬*/
    { LSTRKEY( "send_ok" ),  LFUNCVAL( l_rtos_sendok ) },
/*-labs*/
/*+\ New \ Hedonghao \ 2020.4.10 \ nnladuèn10 · ½ó¿ú*/
	{ LSTRKEY( "openSoftDog" ),  LFUNCVAL( l_open_SoftDog ) },
	{ LSTRKEY( "eatSoftDog" ),  LFUNCVAL( l_eat_SoftDog ) },
	{ LSTRKEY( "closeSoftDog" ),  LFUNCVAL( l_close_SoftDog ) },
/*-\ New \ Hedonghao \ 2020.4.10 \ nnladuèn10 · ½ó¿ú*/
	/*+\NEW\zhuwangbin\2020.6.28\Ìn¼ÓÐ´Âë¹¦ÄÜ*/
#ifdef AM_LUA_POC_SUPPORT
	{ LSTRKEY( "flash_memcpy" ), LFUNCVAL( l_rtos_flash_memcpy )},
	/*+\BUG3109\zhuwgbin\2020.9.21\¶Ô²²²«ïçÐ¿«” ú×´âugh«ug´´´´USB´USB´UR²²culu´POCÑughÑugg*/
	{ LSTRKEY( "poc_ota" ), LFUNCVAL( l_rtos_poc_ota )},
	{ LSTRKEY( "poc_param_erase" ), LFUNCVAL( l_rtos_poc_param_erase )},
	{ LSTRKEY( "poc_param_read" ), LFUNCVAL( l_rtos_poc_param_read )},
	{ LSTRKEY( "poc_param_write" ), LFUNCVAL( l_rtos_poc_param_write )},
	/*-\BUG3109\zhuwgbin\2020.9.21\¶ú²²«ïÐÐ¿¤" ú×´âugh«ug´´´´USB´USB´UR²²culu´POCÑughÑugg*/
#endif
	/*-\NEW\zhuwangbin\2020.6.28\Ìn¼ÓÐ´Âë¹¦ÄÜ*/
	/*+\BUG\wangyuan\2020.07.28 ÁÈ¿Í»§ 1802:*/
	{ LSTRKEY( "set_lua_info" ),	LFUNCVAL( l_rtos_set_luainfo ) },
	/*-\BUG\wangyuan\2020.07.28 ÁÈ¿Í»§ 1802:*/
	/*+\new\wj\luaÌn¼ÓÈÈ²å°Î½Ó¿Ú*/
	{ LSTRKEY( "notify_sim_detect" ),	LFUNCVAL( l_rtos_notify_sim_detect ) },
/*+\bug\rww\2020.9.22\Ìn¼Órtos.setTransData*/
	/*{ LSTRKEY( "setTransData" ), LFUNCVAL( l_rtos_set_trans_data ) },*/
/*-\bug\rww\2020.9.22\Ìn¼Órtos.setTransData*/
	/*-\new\wj\luaÌn¼ÓÈÈ²å°Î½Ó¿Ú*/
	  { LNILKEY, LNILVAL }
};

int luaopen_rtos( lua_State *L )
{
    luaL_register( L, AUXLIB_RTOS, rtos_map );

    // module id
    MOD_REG_NUMBER(L, "MOD_KEYPAD", RTOS_MODULE_ID_KEYPAD);
	/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
    MOD_REG_NUMBER(L, "MOD_ALARM", RTOS_MODULE_ID_ALARM);
	/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/
    MOD_REG_NUMBER(L, "MOD_TP", RTOS_MODULE_ID_TOUCH);

/*what*/
    // ¿ª»úÔ­Òò
    #define REG_POWERON_RESON(rEASON) MOD_REG_NUMBER(L, #rEASON, PLATFORM_##rEASON)
    REG_POWERON_RESON(POWERON_KEY);
    REG_POWERON_RESON(POWERON_CHARGER);
    REG_POWERON_RESON(POWERON_ALARM);
    REG_POWERON_RESON(POWERON_RESTART);
    REG_POWERON_RESON(POWERON_OTHER);
    REG_POWERON_RESON(POWERON_UNKNOWN);
/*- \NEW\\\\\\\\ amonge*/
    /*+\ Newreq New \ zohh 2014.6.18 \ ôö¼óªª »*/
    REG_POWERON_RESON(POWERON_EXCEPTION);
    REG_POWERON_RESON(POWERON_HOST);
    REG_POWERON_RESON(POWERON_WATCHDOG);
    /*-\ Newreq New \ zohh 2014.6.18 \ ôö¼óªª »*/

    // msg id
    MOD_REG_NUMBER(L, "WAIT_MSG_TIMEOUT", MSG_ID_RTOS_WAIT_MSG_TIMEOUT);
    MOD_REG_NUMBER(L, "MSG_TIMER", MSG_ID_RTOS_TIMER);
    MOD_REG_NUMBER(L, "MSG_KEYPAD", MSG_ID_RTOS_KEYPAD);
    MOD_REG_NUMBER(L, "MSG_UART_RXDATA", MSG_ID_RTOS_UART_RX_DATA);
    MOD_REG_NUMBER(L, "MSG_UART_TX_DONE", MSG_ID_RTOS_UART_TX_DONE);
/*+\NEW\liweiqiang\2013.4.5\Ôö¼Ólua gpio ÖÐ¶ÏÅäÖÃ*/
    MOD_REG_NUMBER(L, "MSG_INT", MSG_ID_RTOS_INT);
/*-\NEW\liweiqiang\2013.4.5\Ôö¼Ólua gpio ÖÐ¶ÏÅäÖÃ*/
/*+\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/
    MOD_REG_NUMBER(L, "MSG_PMD", MSG_ID_RTOS_PMD);
/*-\NEW\liweiqiang\2013.7.8\Ôö¼Órtos.pmdÏûÏ¢*/
/*+\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/
    MOD_REG_NUMBER(L, "MSG_AUDIO", MSG_ID_RTOS_AUDIO);
/*-\NEW\liweiqiang\2013.11.4\Ôö¼Óaudio.core½Ó¿Ú¿â*/
	MOD_REG_NUMBER(L, "MSG_HEADSET",MSG_ID_RTOS_HEADSET);
	MOD_REG_NUMBER(L, "MSG_RTMP", MSG_ID_RTOS_RTMP);
    MOD_REG_NUMBER(L, "MSG_RECORD", MSG_ID_RTOS_RECORD);
	MOD_REG_NUMBER(L, "MSG_STREAM_RECORD", MSG_ID_ROTS_STRAM_RECORD_IND);

/*+\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÓÏûÏ¢*/
    MOD_REG_NUMBER(L, "MSG_ALARM", MSG_ID_RTOS_ALARM);
/*-\NEW\rufei\2015.3.13\Ôö¼ÓÄÖÖÓÏûÏ¢*/

    MOD_REG_NUMBER(L, "MSG_TP", MSG_ID_RTOS_TOUCH);

	//+TTS, panjun 160326.
	/*+\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
    MOD_REG_NUMBER(L, "MSG_TTSPLY_STATUS", MSG_ID_RTOS_TTSPLY_STATUS);
    MOD_REG_NUMBER(L, "MSG_TTSPLY_ERROR", MSG_ID_RTOS_TTSPLY_ERROR);
	/*-\new\wj\2019.12.27\Ìn¼ÓTTS¹¦ÄÜ*/
	//-TTS, panjun 160326.

    MOD_REG_NUMBER(L, "MSG_WEAR_STATUS", MSG_ID_RTOS_WEAR_STATUS);

    MOD_REG_NUMBER(L, "MSG_PDP_ACT_CNF", MSG_ID_TCPIP_PDP_ACTIVATE_CNF);
    MOD_REG_NUMBER(L, "MSG_PDP_DEACT_CNF", MSG_ID_TCPIP_PDP_DEACTIVATE_CNF);
    MOD_REG_NUMBER(L, "MSG_PDP_DEACT_IND", MSG_ID_TCPIP_PDP_DEACTIVATE_IND);
    MOD_REG_NUMBER(L, "MSG_SOCK_CONN_CNF", MSG_ID_TCPIP_SOCKET_CONNECT_CNF);
    MOD_REG_NUMBER(L, "MSG_SOCK_SEND_CNF", MSG_ID_TCPIP_SOCKET_SEND_CNF);
/*+Bug \ Brezen \ 2016.03.02 \ Send½ó¿úÏÈ · µ »Øsend Errorè» ºóóö · µ »Øsend ok*/
    MOD_REG_NUMBER(L, "MSG_SOCK_SEND_IND", MSG_ID_TCPIP_SOCKET_SEND_IND);
/*-Bug \ Brezen \ 2016.03.02 \ Send½ó¿úïè · µ »Øsend Errorè» ºóóö · µ »Øsend ok*/
    MOD_REG_NUMBER(L, "MSG_SOCK_RECV_IND", MSG_ID_TCPIP_SOCKET_RECV_IND);
    MOD_REG_NUMBER(L, "MSG_SOCK_CLOSE_CNF", MSG_ID_TCPIP_SOCKET_CLOSE_CNF);
    MOD_REG_NUMBER(L, "MSG_SOCK_CLOSE_IND", MSG_ID_TCPIP_SOCKET_CLOSE_IND);

    //timeout
    MOD_REG_NUMBER(L, "INF_TIMEOUT", PLATFORM_RTOS_WAIT_MSG_INFINITE);
/*+: \ New \ abysses \ 2016.10.13 \ Ö§³ösimwards*/
    MOD_REG_NUMBER(L, "MSG_SIM_INSERT_IND", MSG_ID_SIM_INSERT_IND);
/*-: \ New \ abysses \ 2016.10.13 \ Ö§³ösimwards*/

	MOD_REG_NUMBER(L, "MSG_ZBAR", MSG_ID_RTOS_MSG_ZBAR);

	/*what*/
	MOD_REG_NUMBER(L, "MSG_WIFI", MSG_ID_RTOS_MSG_WIFI);
	/*-\NEWhenyuan\2020.05.25\wi() ¸»*/
	MOD_REG_NUMBER(L, "MSG_BLUETOOTH", MSG_ID_RTOS_BLUETOOTH);

    // ½Øðð oda ¯еnterμ³õ italque »¯
    platform_rtos_init();

    return 1;
}


