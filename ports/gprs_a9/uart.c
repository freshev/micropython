/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 pulkin
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

#include "uart.h"
#include "mphalport.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/ringbuf.h"
#include "py/runtime.h"
#include "api_hal_uart.h"

// ------------------------------
// Hardware UART static variables
// ------------------------------

// The two ports
static const UART_Port_t uart_port[] = {UART1, UART2};

// rx data buffers
uint8_t uart1_ringbuf_array[UART_STATIC_RXBUF_LEN];
uint8_t uart2_ringbuf_array[UART_STATIC_RXBUF_LEN];

uint8_t *uart_ringbuf_array[] = {uart1_ringbuf_array, uart2_ringbuf_array};

// rx ring buffers
ringbuf_t uart_ringbuf[] = {
    {uart1_ringbuf_array, sizeof(uart1_ringbuf_array), 0, 0},
    {uart2_ringbuf_array, sizeof(uart2_ringbuf_array), 0, 0},
};

// UART rx interrupt handler
static void uart_rx_intr_handler(UART_Callback_Param_t param);

// UART configurations
UART_Config_t uart_dev[] = {
    {
        .baudRate = UART_BAUD_RATE_115200,
        .dataBits = UART_DATA_BITS_8,
        .stopBits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .rxCallback = uart_rx_intr_handler,
        .useEvent = false,
    },
    {
        .baudRate = UART_BAUD_RATE_115200,
        .dataBits = UART_DATA_BITS_8,
        .stopBits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .rxCallback = uart_rx_intr_handler,
        .useEvent = false,
    }
};

// -----------------------
// Hardware UART functions
// -----------------------

static bool uart_config(uint8_t uart) {
    // Updates the configuration of UART port
    return UART_Init(uart_port[uart], uart_dev[uart]);
}

uint8_t uart_tx_one_char(uint8_t uart, char c) {
    // Transmit single char
    // if(uart == 1) Trace(1, "UART%d TX one char 0x%02x ('%c')", uart, (uint8_t)c, (char)c);
    return UART_Write(uart_port[uart], (uint8_t*) &c, 1);
}
uint8_t uart_rx_one_char(uint8_t uart) {
    // Receive single char
    int c = ringbuf_get(uart_ringbuf + uart);
    // if(uart == 1) Trace(1, "UART%d RX one char 0x%02x ('%c')", uart, (uint8_t)c, (char)c);
    return c;
}

bool uart_close(uint8_t uart) {
    return UART_Close(uart_port[uart]);
}

static void uart_rx_intr_handler(UART_Callback_Param_t param) {
    // handles rx interrupts
    for (uint32_t i = 0; i < param.length; i++) {
        // For efficiency, when connected to dupterm we put incoming chars
        // directly on stdin_ringbuf, rather than going via uart_ringbuf
        uint8_t RcvChar = (uint8_t)param.buf[i] & 0xFF;
        if (uart_attached_to_dupterm[param.port - 1]) {
            if (RcvChar == mp_interrupt_char) {
                //if(param.port - 1 == 1) Trace(1, "UART%d INTERRUPT", param.port - 1);
                mp_sched_keyboard_interrupt();
            } else {
                // send to stdin
                ringbuf_put(&stdin_ringbuf, RcvChar);
            }
        } else {
            // send to uart_ringbuf
            //if(param.port - 1 == 1) Trace(1, "UART%d ONE CHAR 0x%02x ('%c')", param.port - 1, RcvChar, (char)RcvChar);
            ringbuf_put(&uart_ringbuf[param.port - 1], RcvChar);
        }
    }
}

bool uart_rx_wait(uint8_t uart, uint32_t timeout_us) {
    // waits for rx to become populated
    uint64_t start = mp_hal_ticks_us_64();
    ringbuf_t *ringbuf = uart_ringbuf + uart;
    for (;;) {
        if (ringbuf->iget != ringbuf->iput) {
            return true; // have at least 1 char ready for reading
        }
        if (mp_hal_ticks_us_64() - start >= timeout_us) {
            return false; // timeout
        }
    }
}

int uart_rx_any(uint8_t uart) {
    // checks if rx is not empty
    ringbuf_t *ringbuf = uart_ringbuf + uart;
    if (ringbuf->iget != ringbuf->iput) {
        return true;
    }
    return false;
}

int uart_tx_any_room(uint8_t uart) {
    // checks if ready for tx
    (void) uart;
    return true;
}

void uart_init(UART_Baud_Rate_t uart1_br, UART_Baud_Rate_t uart2_br) {
    // just init with the baud rate
    uart_dev[0].baudRate = uart1_br;
    uart_config(0);
    uart_dev[1].baudRate = uart2_br;
    uart_config(1);
    // install handler for "os" messages
    // os_install_putc1((void *)uart_os_write_char);
}

bool uart_setup(uint8_t uart) {
    return uart_config(uart);
}

int uart_get_rxbuf_len(uint8_t uart) {
    return uart_ringbuf[uart].size;
}

void uart_set_rxbuf(uint8_t uart, uint8_t *buf, int len) {
    uart_ringbuf[uart].buf = buf;
    uart_ringbuf[uart].size = len;
    uart_ringbuf[uart].iget = 0;
    uart_ringbuf[uart].iput = 0;
}
