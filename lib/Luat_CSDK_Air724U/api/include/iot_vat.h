#ifndef  __IOT_VAT_H__
#define  __IOT_VAT_H__

#include "am_openat_vat.h"
#include "iot_network.h"

#define URC_QUEUE_COUNT 10                      /* global urc count */
#define AT_CMD_QUEUE_COUNT 50
#define AT_CMD_EXECUTE_DELAY  10                /* 10 ms */

#define AT_CMD_DELAY "DELAY:"
#define AT_CMD_END "\x0d\x0a"
#define AT_CMD_CR  '\x0d'
#define AT_CMD_LF  '\x0a'
#define STR_TO_INT(x) 	(x-'0') 					/*O öμächar × îªINT*/


typedef enum
{
    AT_RSP_ERROR = -1,
    AT_RSP_WAIT= 0, 
    AT_RSP_CONTINUE = 1,                        /*¼ÌÐøÖ´ÐÐÏÂÒ»ÌõAT¶ÓÁÐÀïµÄÃüÁî*/
    AT_RSP_PAUSE= 2,                            /*Smartime <ṣi'ani*/
    AT_RSP_FINISH = 3,                          /*In £ Ö¹ö´ðat¶ódÃüeî*/

    AT_RSP_FUN_OVER = 4,                        /*£ £ ¿¿¯ ¯ ¯ ¯ ×*/
    AT_RSP_STEP_MIN = 10,
    AT_RSP_STEP = 20,                           /*¼ÌÐøÖ´ÐÐ±¾ÌõATÃüÁî*/
    AT_RSP_STEP_MAX = 30,

}AtCmdRsp;

typedef AtCmdRsp (*AtCmdRspCB)(char *pRspStr);
typedef VOID (*UrcCB)(char *pUrcStr, u16 len);
typedef VOID (*ResultNotifyCb)(BOOL result);

typedef struct AtCmdEntityTag
{
    char* p_atCmdStr;				/*ATÃüÁî×Ö·û´®*/
    u16 cmdLen;					/*AtÃüeî³¤¶¤¶ гive*/
    AtCmdRspCB p_atCmdCallBack;	/*Atãüeî »Øµ ÷ º thousand*/
}AtCmdEntity;

typedef struct UrcEntityTag
{
    char* p_urcStr;
    UrcCB p_urcCallBack;

}UrcEntity;

typedef struct _CELL_INFO
{
	u32 CellId;  //cell ID
	u32 Lac;  //LAC
	u16 Mcc;  //MCC
	u16 Mnc;  //MNC
	u16 rssi; //rssi
}CELL_INFO;

typedef struct _gsmloc_cellinfo
{
	CELL_INFO Cellinfo[6];
}gsmloc_cellinfo;

/**
 * @defgroup iot_sdk_sys ÏµÍ³½Ó¿Ú
 * @{*/
/**@example vat/demo_vat.c
* vat½Ó¿ÚÊ¾Àý*/ 
/** OroGE Goldi Everate ~ EDμÄY WROT ø Ø Goll Ø Ø Øý
* @ Car Fraam Vathandle: ÷¶ééäântol ZâJ ÷ »õ õ »uce¹ologian · · · I Do you see Petri
* @ RETURN TRUE :,³É¹| FALSE: ê ° ü
**/
BOOL iot_vat_init(PAT_MESSAGE vatHandle);

/**Óãà´ · ¢ ëIatãüeî
*@param cmdstr: atãüeî × Ö · û´®
*@param cmdlen: atãüeî³hood
*@Return True: ³é¹¦ FALSE: § ° ü
*@Note × ¢ Òâ £ ¬atãüeî × Ö · û´®CMDSTRöðð ° ° ° ° ° ° \ N "» Òõß "\ r" ½Eî²
**/
BOOL iot_vat_send_cmd(UINT8* cmdStr, UINT16 cmdLen);

/**Óãà´åúe · ¢ ëIatãüeî
*@param cmd: atãüeî²îêý
*@param cmd_count: atãüeît
*@Return True: ³é¹¦ FALSE: § ° ü
**/
BOOL iot_vat_push_cmd(AtCmdEntity cmd[],u8 cmd_count);


/** @}*/


#endif

