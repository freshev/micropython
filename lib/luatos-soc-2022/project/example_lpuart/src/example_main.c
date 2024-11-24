/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"

#include "luat_pm.h"

#include "Driver_Common.h"
#include "bsp_lpusart.h"
#include "bsp.h"

#define RECV_BUFFER_LEN     (64)

#define RTE_UART_RX_IO_MODE     RTE_UART1_RX_IO_MODE
extern ARM_DRIVER_USART Driver_LPUSART1;
static ARM_DRIVER_USART *USARTdrv = &Driver_LPUSART1;

/** \brief usart receive buffer */
uint8_t recBuffer[RECV_BUFFER_LEN];

/** \brief receive timeout flag */
volatile bool isRecvTimeout = false;

/** \brief receive complete flag */
volatile bool isRecvComplete = false;

static void *g_s_test_task_handle;

/** When using the original UART, just compile the following 3 functions*/


bool soc_init_unilog_uart(uint8_t port, uint32_t baudrate, bool startRecv)
{
	extern void SetUnilogUartOrg(usart_port_t port, uint32_t baudrate, bool startRecv);
	SetUnilogUartOrg(port, baudrate, startRecv);
	return true;
}

int soc_uart_sleep_check(uint8_t *result)
{
	return -1;
}

int soc_uart_rx_check(uint8_t *result)
{
	return -1;
}

void USART_callback(uint32_t event)
{
    if(event & ARM_USART_EVENT_RX_TIMEOUT)
    {
    	isRecvTimeout = true;
    	luat_send_event_to_task(g_s_test_task_handle, 0xf2345678, 0, 0, 0);

    }
    if(event & ARM_USART_EVENT_RECEIVE_COMPLETE)
    {
    	isRecvComplete = true;
    	luat_send_event_to_task(g_s_test_task_handle, 0xf2345678, 0, 0, 0);
    }
}

static void task_test_uart(void *param)
{
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, "test");
	luat_event_t event;
    /*Initialize the USART driver */
    USARTdrv->Initialize(USART_callback);

    /*Power up the USART peripheral */
    USARTdrv->PowerControl(ARM_POWER_LOW);

    /*Configure the USART to 115200 Bits/sec */
    USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 9600);

	/*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! ! 1*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET); 
    uint8_t* greetStr = (uint8_t *)"UART Echo Demo\n";

    USARTdrv->Send(greetStr, strlen((const char*)greetStr));
    USARTdrv->Receive(recBuffer, RECV_BUFFER_LEN);
    //When testing with USB plugged in, the USB function needs to be turned off
    luat_pm_set_usb_power(0);
	while(1)
    {
		if (ERROR_NONE == luat_rtos_event_recv(g_s_test_task_handle, 0, &event, NULL, 5000))
		{

	        if(isRecvTimeout == true)
	        {
	            USARTdrv->Send(recBuffer, USARTdrv->GetRxCount());
	            isRecvTimeout = false;
	        }
	        else
	        {
	            isRecvComplete = false;
	            USARTdrv->Send(recBuffer, RECV_BUFFER_LEN);
	        }
	        USARTdrv->Receive(recBuffer, RECV_BUFFER_LEN);
		}
		else
		{
			USARTdrv->Send(greetStr, strlen((const char*)greetStr));
		}
    }
}

static void lpuart_demo_init(void)
{
	luat_mobile_set_rrc_auto_release_time(1);
	luat_rtos_task_create(&g_s_test_task_handle, 4096, 50, "uart", task_test_uart, NULL, 16);
}

INIT_TASK_EXPORT(lpuart_demo_init, "1");
