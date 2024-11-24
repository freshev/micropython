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
#define LUAT_0_5_STOP_BITS                   0xf0   /**< 0.5 */
#define LUAT_1_5_STOP_BITS                   0xf1   /**< 1.5 */

#define LUAT_VUART_ID_0						0x20    

#define LUAT_UART_RX_ERROR_DROP_DATA		(0xD6)
#define LUAT_UART_DEBUG_ENABLE				(0x3E)
/**
 * @brief luat_uart
 */
typedef struct luat_uart {
    int id;                     /**< serial port id*/
    int baud_rate;              /**< baud rate*/

    uint8_t data_bits;          /**< data bits*/
    uint8_t stop_bits;          /**< stop bit*/
    uint8_t bit_order;          /**< High and low bits*/
    uint8_t parity;             /**< parity bit*/

    size_t bufsz;               /**<Receive data buffer size*/
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
 * @return int*/
int luat_uart_write(int uart_id, void* data, size_t length);

/**
 * @brief Serial port reading data
 *
 * @param uart_id serial port id
 * @param buffer data
 * @param length data length
 * @return int*/
int luat_uart_read(int uart_id, void* buffer, size_t length);

/**
 * @brief Clear uart’s receiving cache data
 * @return int*/
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
 * @brief receives callback function
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
 * @param use_alt_type If it is 1, UART0 is multiplexed to GPIO16, GPIO17; UART2 is multiplexed to GPIO12 GPIO13
 * @return int 0 fails, others succeed*/
int luat_uart_pre_setup(int uart_id, uint8_t use_alt_type);

#ifdef LUAT_USE_SOFT_UART
#ifndef __BSP_COMMON_H__
#include "c_common.h"
#endif
/**
 * @brief Hardware timer configuration required for software serial port
 *
 * @param hwtimer_id hardware timer id
 * @param callback Timing callback, if it is NULL, it releases the timer resource
 * @return int Returns 0 on success, other values     are failure*/
int luat_uart_soft_setup_hwtimer_callback(int hwtimer_id, CommonFun_t callback);
void luat_uart_soft_gpio_fast_output(int pin, uint8_t value);
uint8_t luat_uart_soft_gpio_fast_input(int pin);
void luat_uart_soft_gpio_fast_irq_set(int pin, uint8_t onoff);
/**
 * @brief Hardware timing cycle required by software serial port
 *
 * @param baudrate baud rate
 * @return uint32_t calculated timing period*/
uint32_t luat_uart_soft_cal_baudrate(uint32_t baudrate);
/**
 * @brief Hardware timer switch required for software serial port
 *
 * @param hwtimer_id hardware timer id
 * @param period timing period, calculated by luat_uart_soft_cal_baudrate
 * @return int Returns 0 on success, other values     are failure*/
void luat_uart_soft_hwtimer_onoff(int hwtimer_id, uint32_t period);

void luat_uart_soft_sleep_enable(uint8_t is_enable);
#endif
/** @}*/
/** @}*/
#endif
