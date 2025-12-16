/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2023 Damien P. George
 * Copyright (c) 2024-2025 freshev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// This file is never compiled standalone, it's included directly from
// extmod/machine_uart.c via MICROPY_PY_MACHINE_UART_INCLUDEFILE.

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/misc.h"

#include "mphalport.h"

#include "luat_uart.h"
#include "luat_debug.h"

#define UART_NPORTS (3)
#define UART_STATIC_RXBUF_LEN (1024 * 2)

typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
    uint8_t uart_num;
    uint32_t baudrate;
    uint8_t bits;
    uint8_t parity;
    uint8_t stop;
    uint16_t txbuf;
    uint16_t rxbuf;
    uint16_t timeout;       // timeout waiting for first char (in ms)
    uint16_t timeout_char;  // timeout waiting between chars (in ms)
} machine_uart_obj_t;

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
    2. Serial port multiplexing can be achieved through the luat_uart_pre_setup(int uart_id, uint8_t use_alt_type) function. If it is 1, UART0 is multiplexed to GPIO16, GPIO17; UART2 is multiplexed to GPIO12 GPIO13

    UART_ID0 = 0 aka DEBUG_UART pins 38, 39
    UART_ID1 = 1 aka MAIN_UART  pins 17, 18 
    UART_ID2 = 2 aka AUX_UART   pins 28, 29 (used for GNSS ?)

    */
/******************************************************************************/
// UART Luat specific

// rx data buffers
uint8_t uart1_ringbuf_array[UART_STATIC_RXBUF_LEN];
uint8_t uart2_ringbuf_array[UART_STATIC_RXBUF_LEN];
uint8_t uartv_ringbuf_array[UART_STATIC_RXBUF_LEN];
uint8_t *uart_ringbuf_array[] = {uart1_ringbuf_array, uart2_ringbuf_array, uartv_ringbuf_array};
// rx ring buffers
ringbuf_t uart_ringbuf[] = {
    {uart1_ringbuf_array, sizeof(uart1_ringbuf_array), 0, 0},
    {uart2_ringbuf_array, sizeof(uart2_ringbuf_array), 0, 0},
    {uartv_ringbuf_array, sizeof(uartv_ringbuf_array), 0, 0},
};

uint8_t uart_tx_done[3]; 
uint8_t uart_attached_to_dupterm[3]; 

/*
void uart_debug(int uart, const char * prefix, const char *buf_in, mp_uint_t size) {
    char mess[255];
    char mess2[10];
    memset(mess, 0, sizeof(mess));
    strcat(mess, "\n");
    strcat(mess, prefix);
    itoa(uart, mess2, 10);
    strcat(mess, mess2);
    strcat(mess, ":");

    if(buf_in != NULL) {
        strncat(mess, buf_in, size);
        strcat(mess, " ");

        for(int i = 0; i < size; i++) {
            itoa(buf_in[i], mess2, 16);
            strcat(mess, mess2);
            strcat(mess, " ");
        }
        strcat(mess, "(size = ");
        itoa(size, mess2, 10);
        strcat(mess, mess2);
        strcat(mess, ")");
    } else {
      strcat(mess, "NULL");
    }
    if(uart == 1) luat_uart_write(LUAT_VUART_ID_0, mess, strlen(mess)); 
}
*/

void luat_uart_recv_callback(int uart_num, uint32_t data_len) {
    for (uint32_t i = 0; i < data_len; i++) {
        // For efficiency, when connected to dupterm we put incoming chars
        // directly on stdin_ringbuf, rather than going via uart_ringbuf
        // Do not care of speed, getting char by char
        uint8_t c;
        luat_uart_read(uart_num, &c, 1);

        // uart_debug(uart_num, "rcb", (const char*)&c, 1);

        int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;

        if (uart_attached_to_dupterm[uart]) {
            if (c == mp_interrupt_char) {
                // if(uart_num == HW_UART_REPL) LUAT_DEBUG_PRINT("UART%d INTERRUPT", uart_num);
                mp_sched_keyboard_interrupt();
            } else {
                // send to stdin
                ringbuf_put(&stdin_ringbuf, c);
            }
        } else {
            // send to uart_ringbuf
            // if(uart_num == HW_UART_REPL) LUAT_DEBUG_PRINT("UART%d ONE CHAR 0x%02x ('%c')", uart_num, c, (char)c);            
            ringbuf_put(&uart_ringbuf[uart], c);
        }
    }
}

void luat_uart_sent_callback(int uart_num, void *param) {
    // uart_debug(uart_num, "scb", NULL, 0);
    int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;
    uart_tx_done[uart] = 1;
}

uint8_t uart_wait_tx_done(int uart_num, int timeout) {
    uint64_t start = mp_hal_ticks_ms_64();
    int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;
    // uart_debug(uart_num, "wait tx done start", NULL, 0);
    while (uart_tx_done[uart] == 0 && mp_hal_ticks_ms_64() - start < (uint64_t)timeout) {
        luat_rtos_task_sleep(1);
        MICROPY_EVENT_POLL_HOOK
    }
    // uart_debug(uart_num, "wait tx done stop", NULL, 0);
    return uart_tx_done[uart] == 1;
}

bool uart_rx_wait(int uart_num, uint32_t timeout) {
    // waits for rx to become populated
    uint64_t start = mp_hal_ticks_ms_64();
    int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;
    ringbuf_t *ringbuf = uart_ringbuf + uart;
    while(true) {
        if (ringbuf->iget != ringbuf->iput) {
            return true; // have at least 1 char ready for reading
        }
        if (mp_hal_ticks_ms_64() - start >= timeout) {
            return false; // timeout
        }
    }
}

int uart_rx_any(uint8_t uart_num) {    
    int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;
    ringbuf_t *ringbuf = uart_ringbuf + uart;
    return ringbuf->iget != ringbuf->iput;
}

int uart_get_rxbuf_len(uint8_t uart_num) {
    int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;
    return uart_ringbuf[uart].size;
}

void uart_set_rxbuf(uint8_t uart_num, uint8_t *buf, int len) {
    int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;
    uart_ringbuf[uart].buf = buf;
    uart_ringbuf[uart].size = len;
    uart_ringbuf[uart].iget = 0;
    uart_ringbuf[uart].iput = 0;
    // uart_debug(uart_num, "z", &len, 1);
}

int uart_tx_any_room(uint8_t uart) {
    (void) uart;
    return true;
}


uint8_t uart_tx_one_char(uint8_t uart_num, char c, uint16_t timeout_char) {
    int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;    
    uart_tx_done[uart] = 0;

    int ret = luat_uart_write(uart_num, (uint8_t*) &c, 1);

    if(uart_num == 1) {
#if RS485_UART1_WAIT_TX
        uart_wait_tx_done(1, timeout_char);
#endif
    }
    if(uart_num == 2) {
#if RS485_UART2_WAIT_TX
        uart_wait_tx_done(2, timeout_char);
#endif
    }
    return ret;
}
uint8_t uart_rx_one_char(uint8_t uart_num) {
    int uart = (uart_num == LUAT_VUART_ID_0) ? 2 : uart_num - 1;
    return ringbuf_get(uart_ringbuf + uart);
}




static const char *_parity_name[] = {"None", "1", "0"};

/******************************************************************************/
// MicroPython bindings for UART

// The UART class doesn't have any constants for this port.
#define MICROPY_PY_MACHINE_UART_CLASS_CONSTANTS

void modmachine_uart_deinit0(void) {
    luat_uart_close(1);
    luat_uart_close(2);
}

void modmachine_uart_init0(void) {
    //modmachine_uart_deinit0();

    mp_obj_t args[2];
    args[0] = MP_OBJ_NEW_SMALL_INT(HW_UART_REPL);
    args[1] = MP_OBJ_NEW_SMALL_INT(115200);
    args[0] = MP_OBJ_TYPE_GET_SLOT(&machine_uart_type, make_new)(&machine_uart_type, 1, 0, args);
    args[1] = MP_OBJ_NEW_SMALL_INT(1); // should be < MICROPY_PY_OS_DUPTERM
    mp_os_dupterm_obj.fun.var(1, args);    
}

void mp_os_dupterm_stream_detached_attached(mp_obj_t stream_detached, mp_obj_t stream_attached) {
    if (mp_obj_get_type(stream_attached) == &machine_uart_type) {
        machine_uart_obj_t *uart_obj = MP_OBJ_TO_PTR(stream_attached);
        int uart = (uart_obj->uart_num == LUAT_VUART_ID_0) ? 2 : uart_obj->uart_num - 1;
        ++uart_attached_to_dupterm[uart];
    }
    if (mp_obj_get_type(stream_detached) == &machine_uart_type) {
        machine_uart_obj_t *uart_obj = MP_OBJ_TO_PTR(stream_detached);
        int uart = (uart_obj->uart_num == LUAT_VUART_ID_0) ? 2 : uart_obj->uart_num - 1;
        --uart_attached_to_dupterm[uart];
    }
}


static void mp_machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=%s, stop=%u, txbuf=%u, rxbuf=%u, timeout=%u, timeout_char=%u)",
        self->uart_num, self->baudrate, self->bits, _parity_name[self->parity],
        self->stop, self->txbuf, uart_get_rxbuf_len(self->uart_num) - 1, self->timeout, self->timeout_char);
}

static void mp_machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop, ARG_txbuf, ARG_rxbuf, ARG_timeout, ARG_timeout_char };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 115200} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_txbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_rxbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // wait for all data to be transmitted before changing settings
    // uart_wait_tx_done(self->uart_num, 100); ????

    luat_uart_t uart = {
        .id = self->uart_num,
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity    = LUAT_PARITY_NONE,
    };

    if(self->uart_num == 1) {
        #if RS485_UART1_USE
            uart.pin485 = (RS485_UART1_PIN);
            uart.delay = (RS485_UART1_DELAY);
            uart.rx_level = (RS485_UART1_PIN_LEVEL);
        #else
            uart.pin485 = 0;
            uart.delay = 0;
            uart.rx_level = 0;
        #endif
    }
    if(self->uart_num == 2) {
        #if RS485_UART2_USE
            uart.pin485 = (RS485_UART2_PIN);
            uart.delay = (RS485_UART2_DELAY);
            uart.rx_level = (RS485_UART2_PIN_LEVEL);
        #else
            uart.pin485 = 0;
            uart.delay = 0;
            uart.rx_level = 0;
        #endif
    }

    // must reinitialise driver to change the tx/rx buffer size
    // #if MICROPY_HW_ENABLE_UART_REPL
    // if (self->uart_num == HW_UART_REPL) {
    //     mp_raise_ValueError(MP_ERROR_TEXT("UART buffer size is fixed"));
    // }
    // #endif

    if (args[ARG_txbuf].u_int >= 0) {
        self->txbuf = args[ARG_txbuf].u_int;
    }
    if (args[ARG_rxbuf].u_int >= 0) {
        self->rxbuf = args[ARG_rxbuf].u_int;
        uint16_t len = args[ARG_rxbuf].u_int + 1; // account for usable items in ringbuf
        uint8_t *buf;
        int uart = (self->uart_num == LUAT_VUART_ID_0) ? 2 : self->uart_num - 1;
        if (len <= UART_STATIC_RXBUF_LEN) {
            buf = uart_ringbuf_array[uart];
            len = UART_STATIC_RXBUF_LEN;
            MP_STATE_PORT(uart_rxbuf[uart]) = NULL; // clear any old pointer
        } else {
            buf = m_new(uint8_t, len);
            MP_STATE_PORT(uart_rxbuf[uart]) = buf; // retain root pointer
        }
        uart_set_rxbuf(self->uart_num, buf, len);
    }

    // set baudrate
    if (args[ARG_baudrate].u_int > 0) {
        self->baudrate = uart.baud_rate = args[ARG_baudrate].u_int;     
    }

    // set data bits
    switch (args[ARG_bits].u_int) {
        case 0:
            break;
        case 5:
        case 6:
        case 7:
        case 8:
            self->bits = uart.data_bits = args[ARG_bits].u_int;
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid data bits"));
            break;
    }
    
    // set parity
    if (args[ARG_parity].u_obj != MP_OBJ_NULL) {
        if (args[ARG_parity].u_obj == mp_const_none) {
            uart.parity = LUAT_PARITY_NONE;
            self->parity = 0;
        } else {
            mp_int_t parity = mp_obj_get_int(args[ARG_parity].u_obj);
            if (parity & 1) {
                uart.parity = LUAT_PARITY_ODD;                
                self->parity = 1;
            } else {
                uart.parity = LUAT_PARITY_EVEN;                
                self->parity = 2;
            }
        }
    }

    // set stop bits
    switch (args[ARG_stop].u_int) {
        case 1:
            uart.stop_bits = LUAT_UART_STOP_BIT1;
            self->stop = 1;
            break;
        case 2:
            uart.stop_bits = LUAT_UART_STOP_BIT2;
            self->stop = 2;
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid stop bits"));
            break;
    }

    // set timeout (in ms)
    self->timeout = args[ARG_timeout].u_int;


    self->timeout_char = args[ARG_timeout_char].u_int;

    // set timeout_char
    // make sure it is at least as long as a whole character (13 bits here)
    uint32_t min_timeout_char = 13000 / self->baudrate + 1;
    if (self->timeout_char < min_timeout_char) {
        self->timeout_char = min_timeout_char;
    }

    luat_uart_close(self->uart_num);
    luat_uart_setup(&uart);
    luat_uart_ctrl(self->uart_num, LUAT_UART_SET_RECV_CALLBACK, luat_uart_recv_callback);
    luat_uart_ctrl(self->uart_num, LUAT_UART_SET_SENT_CALLBACK, luat_uart_sent_callback);    

}

static mp_obj_t mp_machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get uart id
    mp_int_t uart_num = mp_obj_get_int(args[0]);
    if (!luat_uart_exist(uart_num) || uart_num < 1) { // Currently only supports UART1, UART2, LUAT_VUART_ID_0 (from "luat_uart_ec618.c" = 0x20)
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("UART(%d) does not exist"), uart_num);
    }

    // create instance
    machine_uart_obj_t *self = mp_obj_malloc(machine_uart_obj_t, &machine_uart_type);
    self->uart_num = uart_num;
    self->bits = 8;
    self->parity = 0;
    self->stop = 1;
    //self->txbuf = 0;
    //self->rxbuf = 0; 
    self->timeout = 0;
    self->timeout_char = 0;    

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);
    return MP_OBJ_FROM_PTR(self);
}

static void mp_machine_uart_deinit(machine_uart_obj_t *self) {
    luat_uart_close(self->uart_num);
}

static mp_int_t mp_machine_uart_any(machine_uart_obj_t *self) {
    return uart_rx_any(self->uart_num);    
}

static bool mp_machine_uart_txdone(machine_uart_obj_t *self) {
    int uart = (self->uart_num == LUAT_VUART_ID_0) ? 2 : self->uart_num - 1;
    return uart_tx_done[uart];
}

static mp_int_t mp_machine_uart_readchar(machine_uart_obj_t *self) {
    mp_int_t data = uart_rx_one_char(self->uart_num);
    return data;
}

static void mp_machine_uart_writechar(machine_uart_obj_t *self, uint16_t data) {
    uart_tx_one_char(self->uart_num, (byte)data, self->timeout_char);
}


static mp_uint_t mp_machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    // wait for first char to become available
    if (!uart_rx_wait(self->uart_num, self->timeout)) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // read the data
    uint8_t *buf = buf_in;
    for (;;) {
        *buf++ = uart_rx_one_char(self->uart_num);
        if (--size == 0 || !uart_rx_wait(self->uart_num, self->timeout_char)) {
            // return number of bytes read
            return buf - (uint8_t*)buf_in;
        }
    }
}

static mp_uint_t mp_machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const byte *buf = buf_in;
    // uart_debug(self->uart_num, "w", buf_in, size);    

    int uart = (self->uart_num == LUAT_VUART_ID_0) ? 2 : self->uart_num - 1;    
    uart_tx_done[uart] = 0;

    int ret = luat_uart_write(self->uart_num, (uint8_t*) buf_in, size);

    if(self->uart_num == 1) {
#if RS485_UART1_WAIT_TX
        uart_wait_tx_done(1, size * self->timeout_char);
#endif
    }
    if(self->uart_num == 2) {
#if RS485_UART2_WAIT_TX
        uart_wait_tx_done(2, size * self->timeout_char);
#endif
    }
    return ret;
}

static mp_uint_t mp_machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    machine_uart_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && uart_rx_any(self->uart_num)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && uart_tx_any_room(self->uart_num)) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else if (request == MP_STREAM_FLUSH) {
        // The timeout is estimated using the buffer size and the baudrate.
        // Take the worst case assumptions at 13 bit symbol size times 2.
        uint32_t baudrate = self->baudrate;
        uint32_t timeout = (3 + self->txbuf) * 13000 * 2 / baudrate;
        if (uart_wait_tx_done(self->uart_num, timeout)) {
            ret = 0;
        } else {
            *errcode = MP_ETIMEDOUT;
            ret = MP_STREAM_ERROR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

MP_REGISTER_ROOT_POINTER(byte * uart_rxbuf[UART_NPORTS]);
