/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 * Copyright (c) 2020 pulkin
 * Copyright (c) 2024 freshev
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

#include "py/mperrno.h"
#include "py/mphal.h"
#include "uart.h"
#include "extmod/misc.h"

typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
    uint8_t uart_id;
    uint8_t bits;
    uint8_t parity;
    uint8_t stop;
    uint32_t baudrate;
    uint16_t timeout;       // timeout waiting for first char (in ms)
    uint16_t timeout_char;  // timeout waiting between chars (in ms)
} machine_uart_obj_t;

const char *_parity_name[] = {"None", "1", "0"};

/******************************************************************************/
// MicroPython bindings for UART

// The UART class doesn't have any constants for this port.
#define MICROPY_PY_MACHINE_UART_CLASS_CONSTANTS

// ----
// Init
// ----
void modmachine_uart_init0(void) {
    UART_Close(UART2);
    UART_Close(UART1);

    mp_obj_t args[2];
    args[0] = MP_OBJ_NEW_SMALL_INT(0);
    args[1] = MP_OBJ_NEW_SMALL_INT(115200);
    //args[0] = MP_OBJ_TYPE_GET_SLOT(&machine_uart_type, make_new)(&machine_uart_type, 2, 0, args);
    args[0] = MP_OBJ_TYPE_GET_SLOT(&machine_uart_type, make_new)(&machine_uart_type, MICROPY_PY_OS_DUPTERM, 0, args);
    args[1] = MP_OBJ_NEW_SMALL_INT(1);
    //mp_os_dupterm_obj.fun.var(2, args);
    mp_os_dupterm_obj.fun.var(MICROPY_PY_OS_DUPTERM, args);
}


void mp_os_dupterm_stream_detached_attached(mp_obj_t stream_detached, mp_obj_t stream_attached) {
    if (mp_obj_get_type(stream_attached) == &machine_uart_type) {
        machine_uart_obj_t *uart = MP_OBJ_TO_PTR(stream_attached);
        ++uart_attached_to_dupterm[uart->uart_id];
    }
    if (mp_obj_get_type(stream_detached) == &machine_uart_type) {
        machine_uart_obj_t *uart = MP_OBJ_TO_PTR(stream_detached);
        --uart_attached_to_dupterm[uart->uart_id];
    }
}

// -------
// Classes
// -------
static void mp_machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    // ========================================
    // print(UART)
    // ========================================
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=%s, stop=%u, rxbuf=%u, timeout=%u, timeout_char=%u)",
        self->uart_id, self->baudrate, self->bits, _parity_name[self->parity],
        self->stop, uart_get_rxbuf_len(self->uart_id) - 1, self->timeout, self->timeout_char);
}

static void mp_machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // ========================================
    // Prepares UART.
    // Args:
    //     baudrate (int): baud rate (communication speed) of UART;
    //     bits (int)
    //     parity (int)
    //     stop (int)
    //     tx (int): transmit pin;
    //     rx (int): receive pin;
    //     txbuf (int): transmit buffer size;
    //     rxbuf (int): receive buffer size;
    //     timeout (int): default timeout in ms for transmission start;
    //     timeout_char (int): default timeout in ms between characters;
    // ========================================
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop, ARG_tx, ARG_rx, ARG_txbuf, ARG_rxbuf, ARG_timeout, ARG_timeout_char };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_parity, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_tx, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_rx, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_txbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_rxbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    // set tx/rx pins
    if (args[ARG_tx].u_obj != MP_OBJ_NULL && args[ARG_rx].u_obj != MP_OBJ_NULL) {
        mp_hal_pin_obj_t tx = mp_hal_get_pin_obj(args[ARG_tx].u_obj);
        mp_hal_pin_obj_t rx = mp_hal_get_pin_obj(args[ARG_rx].u_obj);
        if (tx == 1 && rx == 0) {
            self->uart_id = 0;
        } else if (tx == 4 && rx == 5) {
            self->uart_id = 1;
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid tx/rx"));
        }
    }

    UART_Config_t *config = uart_dev + self->uart_id;

    // set baudrate
    if (args[ARG_baudrate].u_int > 0) {
        switch (args[ARG_baudrate].u_int) {
            case UART_BAUD_RATE_1200:
            case UART_BAUD_RATE_2400:
            case UART_BAUD_RATE_4800:
            case UART_BAUD_RATE_9600:
            case UART_BAUD_RATE_14400:
            case UART_BAUD_RATE_19200:
            case UART_BAUD_RATE_28800:
            case UART_BAUD_RATE_33600:
            case UART_BAUD_RATE_38400:
            case UART_BAUD_RATE_57600:
            case UART_BAUD_RATE_115200:
            case UART_BAUD_RATE_230400:
            case UART_BAUD_RATE_460800:
            case UART_BAUD_RATE_921600:
            case UART_BAUD_RATE_1300000:
            case UART_BAUD_RATE_1625000:
            case UART_BAUD_RATE_2166700:
            case UART_BAUD_RATE_3250000:
                config->baudRate = (UART_Baud_Rate_t) args[ARG_baudrate].u_int;
            case 0:
                break;
            default:
                mp_raise_ValueError(MP_ERROR_TEXT("unsupported baud rate"));
                break;
        }
    }
    self->baudrate = config->baudRate;
    
    // set data bits
    switch (args[ARG_bits].u_int) {
        case 7:
        case 8:
            config->dataBits = (UART_Data_Bits_t) args[ARG_bits].u_int;
        case 0:
             break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid data bits"));
            break;
    }
    self->bits = config->dataBits;
    
    // set parity
    if (args[ARG_parity].u_obj != MP_OBJ_NULL) {
        if (args[ARG_parity].u_obj == mp_const_none) {
            config->parity = UART_PARITY_NONE;
        } else {
            mp_int_t parity = mp_obj_get_int(args[ARG_parity].u_obj);
            if (parity & 1) {
                config->parity = UART_PARITY_ODD;
            } else {
                config->parity = UART_PARITY_EVEN;
            }
        }
    }
    self->parity = config->parity;
    
    // set stop bits
    switch (args[ARG_stop].u_int) {
        case 1:
            config->stopBits = UART_STOP_BITS_1;
            break;
        case 2:
            config->stopBits = UART_STOP_BITS_2;
            break;
        case 0:
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid stop bits"));
            break;
    }
    self->stop = config->stopBits; // was + 1

    // set tx rx ring buffer
    // if (args[ARG_txbuf].u_int >= 0 || args[ARG_rxbuf].u_int >= 0) {
        
    // #if MICROPY_HW_ENABLE_UART_REPL
    // if (self->uart_num == HW_UART_REPL) {
    //    mp_raise_ValueError(MP_ERROR_TEXT("UART buffer size is fixed"));
    // }
    // #endif

    
    if (args[ARG_txbuf].u_int >= 0) {
        // self->txbuf = args[ARG_txbuf].u_int;
    }
    if (args[ARG_rxbuf].u_int >= 0) {
        // self->rxbuf = args[ARG_rxbuf].u_int;
        uint16_t len = args[ARG_rxbuf].u_int + 1; // account for usable items in ringbuf
        uint8_t *buf;
        int uart = self->uart_id;
        if (len <= UART_STATIC_RXBUF_LEN) {
            buf = uart_ringbuf_array[uart];
            len = UART_STATIC_RXBUF_LEN;
            MP_STATE_PORT(uart_rxbuf[uart]) = NULL; // clear any old pointer
        } else {
            buf = m_new(uint8_t, len);
            MP_STATE_PORT(uart_rxbuf[uart]) = buf; // retain root pointer
        }
        uart_set_rxbuf(uart, buf, len);
    }
    
    // set timeout (in ms)
    self->timeout = args[ARG_timeout].u_int;

    // set timeout_char
    // make sure it is at least as long as a whole character (12 bits to be safe)
    self->timeout_char = args[ARG_timeout_char].u_int;
    uint32_t min_timeout_char = 12000 / self->baudrate + 1; // was 13000
    if (self->timeout_char < min_timeout_char) {
        self->timeout_char = min_timeout_char;
    }

    // setup
    if (!uart_setup(self->uart_id))
        mp_raise_ValueError(MP_ERROR_TEXT("Failed to setup uart"));
}

static mp_obj_t mp_machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // ========================================
    // Prepares UART.
    // Args:
    //     uart_id (int): hardware UART id;
    //     baudrate (int): baud rate (communication speed) of UART;
    //     bits (int)
    //     parity (int)
    //     stop (int)
    //     tx (int): transmit pin;
    //     rx (int): receive pin;
    //     rxbuf (int, array): receive buffer or its size;
    //     timeout (int): default timeout in us for transmission start;
    //     timeout_char (int): default timeout in us between characters;
    // ========================================
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get uart id
    mp_int_t uart_id = mp_obj_get_int(args[0]);
    if (uart_id < 0 || uart_id >= UART_NPORTS) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", uart_id));
    }

    // create instance
    machine_uart_obj_t *self = m_new_obj(machine_uart_obj_t);
    self->base.type = &machine_uart_type;
    self->uart_id = uart_id;
    self->baudrate = 115200;
    self->bits = 8;
    self->parity = 0;
    self->stop = 1;
    self->timeout = 0;
    self->timeout_char = 0;

    // init the peripheral
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

static void mp_machine_uart_deinit(machine_uart_obj_t *self) {
    uart_close(self->uart_id);
}

static mp_int_t mp_machine_uart_any(machine_uart_obj_t *self) {
    // ========================================
    // Checks if any data ready to be read.
    // Returns:
    //     True if any data available.
    // ========================================
    return uart_rx_any(self->uart_id);
}
// static MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_any_obj, machine_uart_any);

static bool mp_machine_uart_txdone(machine_uart_obj_t *self) {
    return true; // TODO
}


static mp_int_t mp_machine_uart_readchar(machine_uart_obj_t *self) {
    mp_int_t data = uart_rx_one_char(self->uart_id);
    return data;
}

static void mp_machine_uart_writechar(machine_uart_obj_t *self, uint16_t data) {
    uart_tx_one_char(self->uart_id, (byte)data);
}

static mp_uint_t mp_machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    // ========================================
    // UART.read()
    // ========================================
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    // wait for first char to become available
    if (!uart_rx_wait(self->uart_id, self->timeout * 1000)) { // self->timeout * 1000
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // read the data
    uint8_t *buf = buf_in;
    for (;;) {
        *buf++ = uart_rx_one_char(self->uart_id);
        if (--size == 0 || !uart_rx_wait(self->uart_id, self->timeout_char * 1000)) { // was self->timeout_char * 1000
            // return number of bytes read
            return buf - (uint8_t*)buf_in;
        }
    }
}

static mp_uint_t mp_machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    // ========================================
    // UART.write()
    // ========================================
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const byte *buf = buf_in;

    /* TODO implement non-blocking
    // wait to be able to write the first character
    if (!uart_tx_wait(self, timeout)) {
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }
    */

    // write the data
    for (size_t i = 0; i < size; ++i) {
        uart_tx_one_char(self->uart_id, *buf++);
    }
    // return number of bytes written
    return size;
}

static mp_uint_t mp_machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    // ========================================
    // UART io control (close, etc)
    // ========================================
    machine_uart_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && uart_rx_any(self->uart_id)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && uart_tx_any_room(self->uart_id)) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else if (request == MP_STREAM_CLOSE) {
        uart_close(self->uart_id);
        ret = 0;
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

MP_REGISTER_ROOT_POINTER(byte * uart_rxbuf[UART_NPORTS]);