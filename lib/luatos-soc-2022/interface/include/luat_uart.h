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

#ifndef LUAT_UART_H
#define LUAT_UART_H

#include "luat_base.h"
#include "luat_uart_legacy.h"
/**
 *@version V1.0
 *@attention
 *Logic for reporting receiving data interruption:
 * 1. When the serial port is initialized, create a new buffer
 * 2. You can consider applying for a buffer length of several hundred bytes for users to prevent packet loss during user processing.
 * 3. Each time the serial port receives data, it is first stored in the buffer and the length is recorded.
 * 4. When encountering the following situations, call the serial port interrupt again
    a) The buffer is full (when the user applies for more)/there are only a few hundred bytes left in the buffer (when the buffer is requested based on the actual length)
    b) Received fifo receiving timeout interrupt (the serial port data should not continue to be received at this time)
 * 5. When triggering the data received interrupt, the returned data should be the data in the buffer
 * 6. Release buffer resources when closing the serial port*/
/**
 * @ingroup luatos_device peripheral interface
 * @{*/
/**
 * @defgroup luatos_device_uart UART interface
 * @{*/

/**
 * @brief check digit*/
#define LUAT_PARITY_NONE                     0  /**< No validation*/
#define LUAT_PARITY_ODD                      1  /**< odd parity*/
#define LUAT_PARITY_EVEN                     2  /**< even parity*/

/**
 * @brief high and low order*/
#define LUAT_BIT_ORDER_LSB                   0  /**< Low bit is valid*/
#define LUAT_BIT_ORDER_MSB                   1  /**< High bit is valid*/

/**
 * @brief stop bit*/
#define LUAT_UART_STOP_BIT1                   1   /**<1 */
#define LUAT_UART_STOP_BIT2                   2   /**<2 */

#define LUAT_VUART_ID_0						0x20    
#define LUAT_UART_RX_ERROR_DROP_DATA		(0xD6)
#define LUAT_UART_DEBUG_ENABLE				(0x3E)
/**
 * @brief luat_uart
 * @attention uart0 is the underlying log port interface. If you really need to use it and understand the consequences, please call soc_uart0_set_log_off(1) to close the underlying log port. For specific examples, see project/example_uart demo;*/
typedef struct luat_uart {
    int id;                     /**< serial port id*/
    int baud_rate;              /**< baud rate*/

    uint8_t data_bits;          /**< data bits*/
    uint8_t stop_bits;          /**< Stop bit, can be set to 1 or 2, 1: 1 stop bit, 2: 2 stop bits*/
    uint8_t bit_order;          /**< High and low bits*/
    uint8_t parity;             /**< Parity bit 0 No parity check 1 Odd check 2 Even check*/

    size_t bufsz;               /**< Receive data buffer size, the minimum supported is 2046 (default value) and the maximum supported is 8K (8*1024)*/
    uint32_t pin485;            /**< Convert pin 485, if not, it is 0xffffffff*/
    uint32_t delay;             /**< 485 flip delay time, unit us*/
    uint8_t rx_level;           /**<Level in receiving direction*/
    uint8_t debug_enable;		/**< Whether to enable debug function ==LUAT_UART_DEBUG_ENABLE is enabled, others are not enabled*/
    uint8_t error_drop;			/**<Whether to give up data when encountering an error ==LUAT_UART_RX_ERROR_DROP_DATA gives up, others do not give up*/
} luat_uart_t;

/**
 * @brief uart initialization
 *
 * @param uart luat_uart structure
 * @return int*/
int luat_uart_setup(luat_uart_t* uart);

/**
 * @brief Serial port writing data
 *
 * @param uart_id serial port id
 * @param data data
 * @param length data length
 * @return int The actual written length, usually no need to check, only wrong uart_id will cause problems*/
int luat_uart_write(int uart_id, void* data, size_t length);

/**
 * @brief Serial port reading data
 *
 * @param uart_id serial port id
 * @param buffer data
 * @param length data length
 * @return int The actual length read, if the buffer is NULL, it is the current data length in the receive buffer*/
int luat_uart_read(int uart_id, void* buffer, size_t length);

/**
 * @brief Clear uart’s receiving cache data
 * @return none*/
void luat_uart_clear_rx_cache(int uart_id);

/**
 * @brief close the serial port
 *
 * @param uart_id serial port id
 * @return int*/
int luat_uart_close(int uart_id);

/**
 * @brief Check whether the serial port exists
 *
 * @param uart_id serial port id
 * @return int*/
int luat_uart_exist(int uart_id);

/**
 * @brief serial port control parameters*/
typedef enum LUAT_UART_CTRL_CMD
{
    LUAT_UART_SET_RECV_CALLBACK,/**<Receive callback*/
    LUAT_UART_SET_SENT_CALLBACK/**< Send callback*/
}LUAT_UART_CTRL_CMD_E;

/**
 * @brief Receive callback function. For uart with sleep wake-up function, if data_len==0 means it is woken up, but the data used to wake up the uart may be lost and needs to be sent again.
 * @note The uart1 of ec618 can sleep and continue to receive data at the baud rate of 600, 1200, 2400, 4800, 9600, so there will be no wake-up reminder and the received data will be returned directly without losing the data. Other baud rates only have wake-up function
 **/
typedef void (*luat_uart_recv_callback_t)(int uart_id, uint32_t data_len);

/**
 * @brief Send callback function
 **/
typedef void (*luat_uart_sent_callback_t)(int uart_id, void *param);

/**
 * @brief serial port control parameters
 **/
typedef struct luat_uart_ctrl_param
{
    luat_uart_recv_callback_t recv_callback_fun;/**<Receive callback function*/
    luat_uart_sent_callback_t sent_callback_fun;/**<Send callback function*/
}luat_uart_ctrl_param_t;

/**
 * @brief serial port control
 *
 * @param uart_id serial port id
 * @param cmd serial port control command
 * @param param serial port control parameters
 * @return int*/
int luat_uart_ctrl(int uart_id, LUAT_UART_CTRL_CMD_E cmd, void* param);

/**
 * @brief Serial port multiplexing function, currently supports UART0, UART2
 *
 * @param uart_id serial port id
 * @param use_alt_type If it is 1, UART0 is multiplexed to GPIO16, GPIO17; UART2 is multiplexed to GPIO12 GPIO13; if it is 2, UART2 is multiplexed to GPIO6 GPIO7
 * @return int 0 fails, others succeed*/
int luat_uart_pre_setup(int uart_id, uint8_t use_alt_type);

/** @}*/
/** @}*/
#endif
