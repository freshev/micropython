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
#include "luat_uart.h"

// Note: Only 1 is supported here
// Because uart0 is internally occupied by the SDK as the underlying log port and is used to output logs during the bootloader operation, this function cannot be turned off;
// In addition, it can also be used as the system's log port. The usb capture log is limited by the usb enumeration time and will lose part of the log. However, uart0 will not lose this part of the log.
// If uart0 is configured for application use, exceptions that occur during the boot process will not be able to analyze and locate the problem because the underlying log cannot be captured.
#define UART_ID 1

static luat_rtos_task_handle uart_task_handle;
static HANDLE g_s_delay_timer;

static void wakeup_end(void)
{
	char send_buff[] = "\r\nsleep\r\n";
	luat_uart_write(UART_ID, send_buff, sizeof(send_buff) - 1);
	luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, "test");
}

void luat_uart_recv_cb(int uart_id, uint32_t data_len){
	if (data_len)
	{
	    char* data_buff = malloc(data_len+1);
	    luat_uart_read(uart_id, data_buff, data_len);
	    luat_uart_write(uart_id, data_buff, data_len);
	    free(data_buff);
	}
	else
	{
		char send_buff[] = "\r\nrx wakeup, after 5 second sleep\r\n";
		luat_uart_write(uart_id, send_buff, sizeof(send_buff) - 1);
		luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_NONE, "test");
		luat_rtos_timer_start(g_s_delay_timer, 5000, 0, wakeup_end, NULL);
	}
}

static void task_test_uart(void *param)
{
    char send_buff[] = "hello LUAT!!!\n";
    luat_uart_t uart = {
        .id = UART_ID,
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity    = 0
    };
    luat_rtos_timer_create(&g_s_delay_timer);
    luat_uart_setup(&uart);

    luat_uart_ctrl(UART_ID, LUAT_UART_SET_RECV_CALLBACK, luat_uart_recv_cb);
    luat_uart_write(UART_ID, send_buff, sizeof(send_buff) - 1);
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, "test");
    luat_mobile_set_rrc_auto_release_time(1);
    luat_pm_set_usb_power(0);	//If the USB is plugged in for testing, the USB function needs to be turned off
    while (1)
    {
        luat_rtos_task_sleep(30000);
       // luat_uart_write(UART_ID, send_buff, strlen(send_buff));
    }
    luat_rtos_task_delete(uart_task_handle);
}

static void task_demo_uart(void)
{
    luat_rtos_task_create(&uart_task_handle, 2048, 20, "uart", task_test_uart, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_uart,"1");



