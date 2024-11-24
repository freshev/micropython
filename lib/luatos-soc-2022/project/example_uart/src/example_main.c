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
#include "plat_config.h"
/*1. Logic for reporting receiving data interruption:
    1. When the serial port is initialized, create a new buffer
    2. You can consider applying for a buffer length of several hundred bytes for users to prevent packet loss during user processing.
    3. Each time the serial port receives data, it is first stored in the buffer and the length is recorded.
    4. When encountering the following situations, call the serial port interrupt again
        a) The buffer is full (when the user applies for more)/there are only a few hundred bytes left in the buffer (when the buffer is requested based on the actual length)
        b) Received fifo receiving timeout interrupt (the serial port data should not continue to be received at this time)
    5. When triggering the data received interrupt, the returned data should be the data in the buffer.
    6. Release buffer resources when closing the serial port
 2. uart0 usage
    Because uart0 is internally occupied by the SDK as the underlying log port and is used to output logs during the bootloader operation, this function cannot be turned off;
    In addition, it can also be used as the system's log port. USB capture logs are limited by the USB enumeration time, and part of the logs will be lost, while uart0 will not lose this part of the logs.
    If uart0 is configured for application use, exceptions that occur during the boot process will not be able to analyze and locate the problem because the underlying log cannot be captured.
 3. Serial port multiplexing problem
    1. UART reuse, the latest Hezhou standard CSDK no longer needs to be controlled through RTE_Device.h, the original driver still needs
    2. Serial port multiplexing can be achieved through the luat_uart_pre_setup(int uart_id, uint8_t use_alt_type) function. If it is 1, UART0 is multiplexed to GPIO16, GPIO17; UART2 is multiplexed to GPIO12 GPIO13*/
#define UART_ID 1

static luat_rtos_task_handle uart_task_handle;

void luat_uart_recv_cb(int uart_id, uint32_t data_len){
    char* data_buff = malloc(data_len+1);
    memset(data_buff,0,data_len+1);
    luat_uart_read(uart_id, data_buff, data_len);
    LUAT_DEBUG_PRINT("uart_id:%d data:%s data_len:%d",uart_id,data_buff,data_len);
    free(data_buff);
}

void luat_uart_sent_cb(int uart_id, void *param){
    LUAT_DEBUG_PRINT("uart_id:%d ", uart_id);
}

static void task_test_uart(void *param)
{
    // Unless you are very clear about the consequences of using uart0 as a normal serial port for users, do not open the following commented out code
    // BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_PORT_SEL,PLAT_CFG_ULG_PORT_USB);
    char send_buff[] = "hello LUAT!!!\n";
    luat_uart_t uart = {
        .id = UART_ID,
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity    = 0
    };

    luat_uart_setup(&uart);

    luat_uart_ctrl(UART_ID, LUAT_UART_SET_RECV_CALLBACK, luat_uart_recv_cb);
    luat_uart_ctrl(UART_ID, LUAT_UART_SET_SENT_CALLBACK, luat_uart_sent_cb);

    while (1)
    {
        luat_rtos_task_sleep(1000);
        luat_uart_write(UART_ID, send_buff, strlen(send_buff));
    }
    luat_rtos_task_delete(uart_task_handle);
}

static void task_demo_uart(void)
{
    luat_rtos_task_create(&uart_task_handle, 2048, 20, "uart", task_test_uart, NULL, NULL);
}

// Unless you are very clear about the consequences of using uart0 as a normal serial port for users, do not open the following commented out code
// static void uart0_init(void)
// {
//     soc_uart0_set_log_off(1);
// }
// INIT_TASK_EXPORT(uart0_init,"1");

INIT_TASK_EXPORT(task_demo_uart,"1");



