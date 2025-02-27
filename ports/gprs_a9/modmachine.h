/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
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

#include "py/obj.h"
#include "api_event.h"

typedef struct _pyb_uart_obj_t {
    mp_obj_base_t base;
    uint8_t uart_id;
    uint8_t bits;
    uint8_t parity;
    uint8_t stop;
    uint32_t baudrate;
    uint16_t timeout;       // timeout waiting for first char (in ms)
    uint16_t timeout_char;  // timeout waiting between chars (in ms)
} pyb_uart_obj_t;

extern const mp_obj_type_t machine_pin_type;
extern const mp_obj_type_t machine_adc_type;
extern Power_On_Cause_t powerOnCause;

void modmachine_init0(void);
void modmachine_pin_init0(void);
void modmachine_uart_init0(void);
void modmachine_wdt_init0(void);
void modmachine_rtc_init0(void);

void modmachine_notify_power_on(API_Event_t* event);
void modmachine_notify_power_key_down(API_Event_t* event);
void modmachine_notify_power_key_up(API_Event_t* event);

typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t base;
    SPI_ID_t id;
    SPI_CS_t cs;
    uint32_t baudrate;
    SPI_Line_t line;
    uint8_t cpol;
    uint8_t cpha;
    uint8_t cs_active_low;
    SPI_Data_Bits_t bits;
    uint8_t debug;
    uint8_t debug_hst;
    uint8_t mode;
    uint8_t dma_delay;
} machine_hw_spi_obj_t;

extern mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args);

// Machine Hardware Watchdog
extern uint8_t machine_hw_watchdog_active;
extern uint8_t machine_hw_watchdog_pin_level;
void ChangeWDPinStateInBoot(void);
void ChangeWDPinState(HANDLE*);