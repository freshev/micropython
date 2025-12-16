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

#ifndef MICROPY_INCLUDED_GPRS_A9_UART_H
#define MICROPY_INCLUDED_GPRS_A9_UART_H

#include <stdint.h>
#include <stdio.h>

#include "py/ringbuf.h"

#include "api_hal_uart.h"

#define UART_NPORTS (2)
#define UART_STATIC_RXBUF_LEN (2048)

void uart_set_rxbuf(uint8_t uart, uint8_t *buf, int len);
int uart_get_rxbuf_len(uint8_t uart);
bool uart_rx_wait(uint8_t uart, uint32_t timeout_us);

uint8_t uart_tx_one_char(uint8_t uart, char c);
uint8_t uart_rx_one_char(uint8_t uart);
int uart_rx_any(uint8_t uart);
int uart_tx_any_room(uint8_t uart);
bool uart_setup(uint8_t uart);
bool uart_close(uint8_t uart);

extern uint8_t *uart_ringbuf_array[2];
extern UART_Config_t uart_dev[UART_NPORTS];
extern ringbuf_t uart_ringbuf[UART_NPORTS];

#endif // MICROPY_INCLUDED_GPRS_A9_UART_H