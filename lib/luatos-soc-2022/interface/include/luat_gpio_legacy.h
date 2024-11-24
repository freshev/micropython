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


#ifndef LUAT_GPIO_LEGACY_H
#define LUAT_GPIO_LEGACY_H


#include "luat_base.h"
#ifdef __LUATOS__
#include "luat_msgbus.h"
int l_gpio_handler(lua_State *L, void* ptr);
#endif
typedef int (*luat_gpio_irq_cb)(int pin, void* args);


#define Luat_GPIO_LOW                 0x00 ///< GPIO configured as low level
#define Luat_GPIO_HIGH                0x01 ///< GPIO is configured as high level

#define Luat_GPIO_OUTPUT         0x00 ///< GPIO is configured as output mode
#define Luat_GPIO_INPUT          0x01 ///< GPIO is configured as input mode
#define Luat_GPIO_IRQ            0x02 ///< GPIO is configured in interrupt mode

#define Luat_GPIO_DEFAULT        0x00 ///< GPIO is configured in default mode
#define Luat_GPIO_PULLUP         0x01 ///< GPIO is configured in pull-up mode
#define Luat_GPIO_PULLDOWN       0x02 ///< GPIO is configured in pull-down mode

#define Luat_GPIO_RISING             0x00 ///<Rising edge interrupt
#define Luat_GPIO_FALLING            0x01 ///< Falling edge interrupt
#define Luat_GPIO_BOTH               0x02 ///<Rising edge and falling edge interrupt
#define Luat_GPIO_HIGH_IRQ			0x03	///< GPIO is configured in high level interrupt mode
#define Luat_GPIO_LOW_IRQ			0x04	///< GPIO is configured in low level mode

#define Luat_GPIO_MAX_ID             255 ///<Maximum GPIO serial number

/**
 * @brief GPIO control parameters*/
typedef struct luat_gpio
{
    int pin;/**<pin*/
    int mode;/**<GPIO mode*/
    int pull;/**<GPIO pull-down mode*/
    int irq;/**<GPIO interrupt mode*/
    int lua_ref;
    luat_gpio_irq_cb irq_cb;/**<Interrupt handling function*/
    void* irq_args;
    int alt_func;
} luat_gpio_t;
/**
 * @brief GPIO initialization
 * @param gpio gpio initialization structure parameters*/
int luat_gpio_setup(luat_gpio_t* gpio);
/**
 * @brief GPIO_Mode configuration function
 * @param pin GPIO pin number
 * @param mode GPIO mode
 * @param pull GPIO pull-down selection
 * @param initOutput initial output mode*/
void luat_gpio_mode(int pin, int mode, int pull, int initOutput);


#endif
