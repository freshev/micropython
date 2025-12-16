#include <stdio.h>
#include "string.h"
#include "iot_debug.h"
#include "iot_uart.h"
#include "iot_os.h"
#include "iot_vat.h"

HANDLE uart_task_handle;

#define uart_print iot_debug_print
#define UART_PORT1 OPENAT_UART_1     // 0
#define UART_PORT2 OPENAT_UART_2     // 1
#define UART_PORT3 OPENAT_UART_3     // 2
#define UART_USB   OPENAT_UART_USB   // 3 (LUAT USB Device 1 AT)
//#define UART_RECV_TIMEOUT (1 * 1000) // 1 S
#define UART_RECV_TIMEOUT (0) // 1 S

typedef enum
{
    UART_RECV_MSG = 1,

}TASK_MSG_ID;

typedef struct
{
    TASK_MSG_ID id;
    UINT32 len;
    VOID *param;
}TASK_MSG;


VOID uart_msg_send(HANDLE hTask, TASK_MSG_ID id, VOID *param, UINT32 len)
{
    TASK_MSG *msg = NULL;

    msg = (TASK_MSG *)iot_os_malloc(sizeof(TASK_MSG));
    msg->id = id;
    msg->param = param;
    msg->len = len;

    //iot_os_send_message(hTask, msg);
}

//ÖÐ¶Ï·½Ê½¶Á´®¿Ú1Êý¾Ý
// ¢
void uart_recv_handle(T_AMOPENAT_UART_MESSAGE* evt)
{
	INT8 *recv_buff = NULL;
    int32 recv_len1, recv_len2, recv_lenu;
    int32 dataLen = evt->param.dataLen;
    iot_debug_print("UART callback fired");
	if(dataLen)
	{
		recv_buff = iot_os_malloc(dataLen + 1);
		memset(recv_buff, 0, dataLen + 1);
		if(recv_buff == NULL)
		{
			iot_debug_print("uart_recv_handle_0 recv_buff malloc fail %d", dataLen);
		}	
		switch(evt->evtId)
		{
		    case OPENAT_DRV_EVT_UART_RX_DATA_IND:
		        recv_len1 = iot_uart_read(UART_PORT1, (UINT8*)recv_buff, dataLen , UART_RECV_TIMEOUT);
		        if(recv_len1 > 0) {
		        	iot_debug_print("UART1 recv callback: recv_len %d", recv_len1);
					uart_msg_send(uart_task_handle, UART_RECV_MSG, recv_buff, recv_len1);
				}
				recv_len2 = iot_uart_read(UART_PORT2, (UINT8*)recv_buff, dataLen , UART_RECV_TIMEOUT);
		        if(recv_len2 > 0) {
		        	iot_debug_print("UART1 recv callback: recv_len %d", recv_len2);
					uart_msg_send(uart_task_handle, UART_RECV_MSG, recv_buff, recv_len2);
				}
				recv_lenu = iot_uart_read(UART_USB, (UINT8*)recv_buff, dataLen , UART_RECV_TIMEOUT);
		        if(recv_lenu > 0) {
		        	iot_debug_print("UART_USB recv callback: recv_len %d", recv_lenu);
					uart_msg_send(uart_task_handle, UART_RECV_MSG, recv_buff, recv_lenu);
				}
		        break;

		    case OPENAT_DRV_EVT_UART_TX_DONE_IND:
		        iot_debug_print("uart_recv_handle_2 OPENAT_DRV_EVT_UART_TX_DONE_IND");
		        break;
		    default:
		        break;
		}
	}
}

VOID uart_write(VOID)
{
    char write_buff0[] = "UART1 hello world\r\n";
    char write_buff1[] = "UART2 hello world\r\n";
    char write_buff2[] = "UART3 hello world\r\n";
    char write_buff3[] = "USB hello world";
  
    iot_uart_write(UART_PORT1, (UINT8*)write_buff0, strlen(write_buff0));
    iot_uart_write(UART_PORT2, (UINT8*)write_buff1, strlen(write_buff1));
    iot_uart_write(UART_PORT3, (UINT8*)write_buff2, strlen(write_buff2));
    iot_uart_write(UART_USB, (UINT8*)write_buff3, strlen(write_buff3));
    iot_debug_print("[uart] uart_write hello world to all UARTs");
}

VOID uart_open(VOID)
{
    BOOL err;
    T_AMOPENAT_UART_PARAM uartCfg;
    
    memset(&uartCfg, 0, sizeof(T_AMOPENAT_UART_PARAM));
    uartCfg.baud = OPENAT_UART_BAUD_115200; //²¨ÌØÂÊ
    uartCfg.dataBits = 8;   //Êý¾ÝÎ»
    uartCfg.stopBits = 1; // ֹͣλ
    uartCfg.parity = OPENAT_UART_NO_PARITY; // î £ £
    uartCfg.flowControl = OPENAT_UART_FLOWCONTROL_NONE; //ÎÞÁ÷¿Ø
    uartCfg.txDoneReport = TRUE; // ÉèÖÃTURE¿ÉÒÔÔÚ»Øµ÷º¯ÊýÖÐÊÕµ½OPENAT_DRV_EVT_UART_TX_DONE_IND
    uartCfg.uartMsgHande = uart_recv_handle; //»Øµ÷º¯Êý

    // åäööuart1
    err = iot_uart_open(UART_PORT1, &uartCfg);
	iot_debug_print("[uart] %d open err: %d", UART_PORT1, err);

	err = iot_uart_open(UART_PORT2, &uartCfg);
	iot_debug_print("[uart] %d open err: %d", UART_PORT2, err);

    err = iot_uart_open(UART_PORT3, &uartCfg);
	iot_debug_print("[uart] %d open err: %d", UART_PORT3, err);

	//uartCfg.txDoneReport = FALSE;
	//uartCfg.uartMsgHande = NULL;
	err = iot_uart_open(UART_USB, &uartCfg);
	iot_debug_print("[uart] uart_open_usb err: %d", err);

	iot_debug_print("uartConfig.uartMsgHande %p", uartCfg.uartMsgHande);
	

}

VOID uart_close(VOID)
{
	iot_uart_close(UART_PORT1);
    iot_uart_close(UART_PORT2);
    iot_uart_close(UART_PORT3);
    iot_uart_close(UART_USB);
    iot_debug_print("[uart] uart_close");
}

VOID uart_init(VOID)
{   
    uart_open(); 
}

static VOID uart_task_main(PVOID pParameter)
{
	TASK_MSG *msg = NULL;
	
	while(1)
	{
		iot_os_wait_message(uart_task_handle, (PVOID*)&msg);
		switch(msg->id)
	    {
	        case UART_RECV_MSG:    
				iot_debug_print("[uart] uart_task_main_1 recv_len %s", msg->param);
	            break;
	        default:
	            break;
	    }

	    if(msg)
	    {
	        if(msg->param)
	        {
	            iot_os_free(msg->param);
	            msg->param = NULL;
	        }
	        iot_os_free(msg);
	        msg = NULL;
			iot_debug_print("[uart] uart_task_main_2 uart free");
	    }
		uart_write(); // ´®¿ú2 ð´êým
	}
}

static VOID usb_task_main(PVOID param)
{
	INT8 *recv_buff = iot_os_malloc(32);
	CHAR buff[64];
	UINT32 len = 0;
	while (1)
	{
		len = iot_uart_read(UART_USB, (UINT8 *)recv_buff, 32, UART_RECV_TIMEOUT);
		if (len > 0) {
			snprintf(buff, len, "%s", recv_buff);
			iot_debug_print("[uart %d] usb_task_main %s", UART_USB, buff);
		}

		if (len == 0) {
			iot_os_sleep(10);
		}

	}
}

int appimg_enter(void *param)
{    
    iot_vat_send_cmd((uint8_t*)"AT^TRACECTRL=0,1,2\r\n", sizeof("AT^TRACECTRL=0,1,2\r\n")); // enable OSI trace to AP Diag
    iot_vat_send_cmd((uint8_t*)"AT^TRACECTRL=1,1,2\r\n", sizeof("AT^TRACECTRL=1,1,2\r\n")); // enable Modem trace to AP Diag

    iot_debug_print("[uart] appimg_enter");
    uart_init();
	uart_task_handle =  iot_os_create_task(uart_task_main, NULL, 4096, 1, OPENAT_OS_CREATE_DEFAULT, "uart_task");
	//iot_os_create_task(usb_task_main, NULL, 4096, 1, OPENAT_OS_CREATE_DEFAULT, "usb_task");

    return 0;
}

void appimg_exit(void)
{
    iot_debug_print("[uart] appimg_exit");
}

