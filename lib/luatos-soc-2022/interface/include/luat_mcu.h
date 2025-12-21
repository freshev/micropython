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

#ifndef LUAT_MCU_H
#define LUAT_MCU_H
#include "luat_base.h"

/**
 * @defgroup luatos_mcu MCU special operations
 * @{*/

/**
 * @brief Set the main frequency
 *
 * @param mhz clock frequency, unit MHz, only supported by Air101/Air103, not supported by EC618
 * @return int <= 0 error >0 actual outgoing size*/
int luat_mcu_set_clk(size_t mhz);
/**
 * @brief Get the main frequency
 *
 * @return int clock frequency, unit MHz*/
int luat_mcu_get_clk(void);

/**
 * @brief Get unique id
 *
 * @param t id length, if failed, set to 0
 * @return char* unique id, no need to release!!*/
const char* luat_mcu_unique_id(size_t* t);

/**
 * @brief Get system tick count
 *
 * @return long tick count value*/
long luat_mcu_ticks(void);

/**
 * @brief Get the system tick frequency
 *
 * @return long tick frequency, usually 1000, that is, each tick occupies 1ms, frequency 1000hz*/
uint32_t luat_mcu_hz(void);

/**
 * @brief Get the tick count, 64-bit, corresponding frequency 26Mhz
 *
 * @return uint64_t tick count*/
uint64_t luat_mcu_tick64(void);

/**
 * @brief Frequency data corresponding to tick64
 *
 * @return int The number of ticks corresponding to each us*/
int luat_mcu_us_period(void);

/**
 * @brief Number of milliseconds since booting
 *
 * @return uint64_t milliseconds, unit 1ms, will continue to accumulate after sleep, but the accuracy is lower than other ticks*/
uint64_t luat_mcu_tick64_ms(void);

/**
 * @brief Set clock source (only supported by Air105)
 *
 * @param source_main main clock source
 * @param source_32k 32k clock source
 * @param delay How long to delay after setting*/
void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay);

#endif

