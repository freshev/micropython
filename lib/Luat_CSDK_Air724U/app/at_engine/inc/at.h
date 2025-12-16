#ifndef __AT_H
#define __AT_H

#include "ctype.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "iot_sys.h"
#include "iot_uart.h"
#include "iot_os.h"
#include "iot_fs.h"
#include "iot_debug.h"

#define ICACHE_FLASH_ATTR
#define at_recvTaskPrio 3
#define at_procTaskPrio 4

#define at_cmdLenMax 128
#define at_dataLenMax 2048

extern UINT8 g_s_at_resultBuff[];

#define TRANS_EVENT_CONN_NEXT (0x10)
#define TRANS_EVENT_SEND_NEXT (0x11)
#define TRANS_EVENT_DISCON_NEXT (0x12)
#define TRANS_EVENT_DATA_SENT (0x13)
#define TRANS_EVENT_WRITE_DATA (0x14)
#define TRANS_EVENT_WRITE_DATA_NEXT (0x15)

#define UART_EVENT_RECV_DATA (0X100)
#define UART_EVENT_INT_ERR (0x101)
/*-/bug/abyss/2014.11.15/ðNHTTPPGNTPJJJeQ*/
#define os_memcmp memcmp
#define os_memset memset
#define os_memcpy memcpy
#define os_sprintf sprintf

/*+/NEW WIFI-10/BREZEN/2014.11.12/Ö's «« ä £ ê½+¹ ¹øöæær*/
#define at_putNewLine() at_uart_send("\r\n", 2);

#define at_backOk at_uart_send("\r\nOK\r\n", strlen("\r\nOK\r\n"));
#define at_backError at_uart_send("\r\nERROR\r\n", strlen("\r\nERROR\r\n"));
#define at_backTeError "+CTE ERROR: %d\r\n"
/*+/Bug/abyss/2014.9*/
#define AM_ERR_TIMEOUT ("+CME ERROR: connect timeout\r\n")
#define AM_ERR_NO_DEVICE_CONNECTED ("+CME ERROR: no device connected\r\n")
#define AM_ERR_MODE_NOT_SUPPORT ("+CME ERROR: mode not support\r\n")
/*+/Bug/brezen/2014.9.30/Cipserverö§³ööö¼ö¸ö² »nsk¶ú¿ú*/
#define AM_ERR_MEM_FULL ("+CME ERROR: memory full\r\n")
/*-/bug/brezen/2014.9.30/Cipserverö§³ööö¼ö¸ö² »nsk¶ú¿ú*/
#define AM_ERR_INVALID_INPUT_VALUE ("+CME ERROR: invalid input value\r\n")
#define AM_ERR_IPMODE_NOT_SUPPORT ("+CME ERROR: transparent mode not support\r\n")
#define AM_ERR_SOCKET_NOT_CONNECT ("+CME ERROR: The socket is not connected\r\n")
#define AM_ERR_SOCKET_ALREADY_CONN ("+CME ERROR: The socket is already connected\r\n")
#define AM_ERR_SOCKET_DNS_FAILD ("+CME ERROR: DNS is fail\r\n")
#define AM_ERR_SOCKET_PORT_IN_USE ("+CME ERROR: port in use\r\n")
#define AM_ERR_DEVICE_BUSY ("+CME ERROR: device busy\r\n")
#define AM_ERR_CONNECT_FAILD ("CONNECT FAILD\r\n")
#define AM_ERR_MUX_CONNECT_FAILD ("%d,CONNECT FAILD\r\n")
#define AM_ERR_ERROR ("ERROR\r\n")
#define AM_WARNING_MODE_CHANNED ("mode changed need reset!\r\n")

#define AM_STATE_IP_INIT ("STATE: IP INITIAL\r\n")

#define AM_MUX_CONNECT_OK ("\r\n%d,CONNECT OK\r\n")
#define AM_CONNECT_OK ("\r\nCONNECT OK\r\n")
#define AM_MUX_CLOSE_OK ("%d,CLOSE OK\r\n")
#define AM_CLOSE_OK ("CLOSE OK\r\n")
#define AM_MUX_SEND_OK ("\r\n%d,SEND OK\r\n")
#define AM_SEND_OK ("\r\nSEND OK\r\n")
/*+ Bug WIFI-3 / BREZEN / 2014.12.4 / at + cipcloseîşîþ§¹Û*/
#define AM_MUX_SEND_FAIL ("\r\n%d,SEND FAIL\r\n")
#define AM_SEND_FAIL ("\r\nSEND FAIL\r\n")
/*- Bug Wifi-3 / BREZEN / 2014.12.4 / at + cipcloseîşîþ§¹Û*/
#define AM_MUX_DATA_ACCEPT ("\r\nDATA ACCEPT:%d,%d\r\n")
#define AM_DATA_ACCEPT ("\r\nDATA ACCEPT:%d\r\n")
#define AM_MUX_CONNECT_FAILD ("%d,CONNECT FAIL\r\n")
#define AM_CONNECT_FAILD ("CONNECT FAIL\r\n")
/*+/Bug WiFi-2/abyss/2014.11.7/Atricê½´nîó*/
#define AM_MUX_ALREAY_CONNECT ("%d,ALREADY CONNECT\r\n")
#define AM_ALREAY_CONNECT ("ALREADY CONNECT\r\n")
/*-/Bug WiFi-2/abyss/2014.11.7/Atricê½´nîó*/
#define AM_CLIENT_CLOSED ("%d,CLOSED\r\n")
/*+/Bug WiFi-2/abyss/2014.11.7/Atricê½´nîó*/
#define AM_MUX_DATA ("\r\n+RECEIVE,%d,%d:\r\n")
#define AM_DATA ("\r\n+IPD,%d:")

#define AM_SC_STATUS_FIND_CHANNEL ("SC_STATUS_FIND_CHANNEL\r\n")
#define AM_SC_STATUS_GETTING_SSID_PSWD ("SC_STATUS_GETTING_SSID_PSWD\r\n")
#define AM_SC_STATUS_GOT_SSID_PSWD ("SC_STATUS_GOT_SSID_PSWD\r\n")
#define AM_SC_STATUS_LINK ("SC_STATUS_LINK\r\n")
/*-/Bug WiFi-2/abyss/2014.11.7/Atricê½´nîó*/
/*+/NEW WIFI-75/liuaochuan/2015.4.13/ ¶ÔÓÐÐ©ÃüÁîÔö¼Ó¹Ø»ú±£´æ¹¦ÄÜ*/
#define AM_ERR_ALREADY_OPENED ("+CME ERROR:The function is already opened\r\n")
#define AM_ERR_ALREADY_CLOSED ("+CME ERROR:The function is already closed\r\n")
/*+/NEW WIFI-75/liuaochuan/2015.4.13/ ¶ÔÓÐÐ©ÃüÁîÔö¼Ó¹Ø»ú±£´æ¹¦ÄÜ*/
#define AM_AT_RESULT_BUF_SIZE (128)
#define AM_AT_SEND_RESULT(str, ...)                              \
	do                                                           \
	{                                                            \
		UINT16 len;                                              \
		os_memset(g_s_at_resultBuff, 0, AM_AT_RESULT_BUF_SIZE);  \
		len = os_sprintf(g_s_at_resultBuff, str, ##__VA_ARGS__); \
		at_uart_send(g_s_at_resultBuff, len);                    \
	} while (0)
/*-/bug/abyss/2014.9*/
//#define AT_DEBUG_MODE
#define at_debug iot_debug_print

#ifndef ASSERT
#define ASSERT(c) iot_debug_assert(c, (CHAR *)__FUNCTION__, __LINE__);
#endif

typedef UINT32 os_signal_t;
typedef UINT32 os_param_t;

typedef struct ETSEventTag os_event_t;

struct ETSEventTag
{
	os_signal_t sig;
	os_param_t par;
};

typedef signed short int16_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed char sint8_t;

typedef enum
{
	cmdResultOk,
	cmdResultNotSupport,
	cmdResultNotFound,
	cmdResultProcessAgain, // µ × × × ×
	cmdResultProcessing,
	cmdResultReplace, // ò ± ± × × × ²
	cmdResultError,
	cmdResultNull,
} at_cmdResult;

//pill · μä ghä ghä ghijes

typedef enum
{
	at_statIdle,
	at_statRecving,
	at_statProcess,
	at_statIpSending,
	at_statIpSended,
	at_statIpTraning
} at_stateType;

typedef enum
{
	m_init,
	m_wact,
	m_gotip,
	m_linked,
	m_unlink,
	m_wdact
} at_mdStateType;

typedef struct
{
	char *at_cmdNameCore;
	char *at_cmdName;
	int8_t at_cmdLen;
	at_cmdResult (*at_testCmd)(uint8_t id);
	at_cmdResult (*at_queryCmd)(uint8_t id);
	at_cmdResult (*at_setupCmd)(uint8_t id, char *pPara);
	at_cmdResult (*at_exeCmd)(uint8_t id);
} at_funcationType;

typedef enum
{
	teClient,
	teServer
} teType;
/*+/NEW WIFI-10/BREZEN/2014.11.12/Ö's «« ä £ ê½+¹ ¹øöæær*/
typedef enum
{
	TIMER_SEND_DATA_REPEAT_ID,
	TIMER_RECV_DATA_TIMEOUT_ID,
	TIMER_TRANSPARENT_RECON_ID,
	TIMER_SEND_DATA_RESEND_ID,
	TIMER_MAX_ID
} E_TIMER_ID;

/*-/NEW WIFI-25/liuaochuan/2014.12.5/Ö§³ÖCIFSRÐÞ¸ÄIPµØÖ·*/

#define AT_LINKID_INVALID (0xff)
typedef struct
{
	BOOL specialAtState;  /*specialAtState*/
	at_stateType atState; /*at_state*/

	uint8 uartDataBuffer[at_dataLenMax];
	uint16 uartDataRecivedLen; /*'Óuartòñ does µ½ µäêýility at_datalen*/
	BOOL enterSleep;		   /*½øÈëË¯Ãß*/
	BOOL vatIndToUartPort;	   /*TRUE: vatIndHandleÊä³öµ½´«Êä FALSE: vatIndHandleÊä³öµ½ril*/
	BOOL echoMode;
} T_AT_CONTEXT;
/*-/New WiFi-10/Brezen/2014.11.12/Ö§³öns «ä £ ê½+¹ ¹øöæær*/

T_AT_CONTEXT *at_ptrAtContext(void);

/*+/New WiFi-13/abyss/2014.12.22/'² out × ôêêó few*/

typedef struct
{
	uint32 baud;
	uint32 clk;
} T_UART_BAUD_CLK;
/*-/New WiFi-13/abyss/2014.12.22/'² out × ôêêó few*/

void at_init(void);
uint32 at_uart_send(char *data, uint32 len);

bool system_os_post(uint8 prio, os_signal_t sig, os_param_t par);

#endif
