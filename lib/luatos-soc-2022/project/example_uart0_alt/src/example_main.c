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

#include "luat_uart.h"

/** If you want to maintain the unilog function and use SPI1, you need to multiplex the IO of UART0 to other places. See the following operation.*/

extern int32_t soc_unilog_callback(void *pdata, void *param);
bool soc_init_unilog_uart(uint8_t port, uint32_t baudrate, bool startRecv)
{
	soc_get_unilog_br(&baudrate);
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_16, 0), 3, 0, 0);
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_17, 0), 3, 0, 0);
	GPIO_PullConfig(GPIO_ToPadEC618(HAL_GPIO_16, 0), 1, 1);
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_14, 0), 0, 0, 0);	//The original UART0 TXRX changes back to GPIO function
	GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_15, 0), 0, 0, 0);
	Uart_BaseInitEx(port, baudrate, 0, 256, UART_DATA_BIT8, UART_PARITY_NONE, UART_STOP_BIT1, soc_unilog_callback);
	return true;
}

