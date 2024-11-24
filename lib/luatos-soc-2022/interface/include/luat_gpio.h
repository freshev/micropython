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


#ifndef LUAT_GPIO_H
#define LUAT_GPIO_H


#include "luat_base.h"
#include "luat_gpio_legacy.h"

/**
 * @defgroup luatos_device peripheral interface
 * @{*/

/**
 * @defgroup luatos_device_gpio GPIO接口
 * @{*/

#define LUAT_GPIO_LOW                 (Luat_GPIO_LOW)  ///< GPIO configured as low level
#define LUAT_GPIO_HIGH                (Luat_GPIO_HIGH) ///< GPIO is configured as high level

#define LUAT_GPIO_OUTPUT         (Luat_GPIO_OUTPUT) ///< GPIO is configured as output mode
#define LUAT_GPIO_INPUT          (Luat_GPIO_INPUT)  ///< GPIO is configured as input mode
#define LUAT_GPIO_IRQ            (Luat_GPIO_IRQ) ///< GPIO is configured in interrupt mode

#define LUAT_GPIO_DEFAULT        (Luat_GPIO_DEFAULT) ///< GPIO is configured in default mode, EC618 platform, pull-down is only valid for output mode, input mode only has default pull-down or cancel pull-down, ordinary GPIO is configured as LUAT_GPIO_DEFAULT, which means completely turning off pull-down; AGPIO software does not support it Configuration pull-down, even if configured, is invalid. It is always the default state when the hardware is turned on or reset.
#define LUAT_GPIO_PULLUP         (Luat_GPIO_PULLUP) ///< GPIO is configured in pull-up mode
#define LUAT_GPIO_PULLDOWN       (Luat_GPIO_PULLDOWN)///< GPIO is configured in pull-down mode

#define LUAT_GPIO_RISING_IRQ             (Luat_GPIO_RISING) ///<Rising edge interrupt
#define LUAT_GPIO_FALLING_IRQ            (Luat_GPIO_FALLING)///< Falling edge interrupt
#define LUAT_GPIO_BOTH_IRQ               (Luat_GPIO_BOTH) ///<Rising edge and falling edge interrupt
#define LUAT_GPIO_HIGH_IRQ			(Luat_GPIO_HIGH_IRQ)	///< GPIO is configured in high level interrupt mode
#define LUAT_GPIO_LOW_IRQ			(Luat_GPIO_LOW_IRQ)	///< GPIO is configured in low level mode
#define LUAT_GPIO_NO_IRQ			(0xff) ///< GPIO has no interrupt mode

#define LUAT_GPIO_MAX_ID             (Luat_GPIO_MAX_ID) ///<Maximum GPIO serial number



/**
 * @brief GPIO control parameters*/
typedef struct luat_gpio_cfg
{
    int pin; /**<pin*/
    uint8_t mode;/**<GPIO mode*/
    uint8_t pull;/**<GPIO pull-down mode*/
    uint8_t irq_type;/**<GPIO interrupt mode*/
    uint8_t output_level;/**<GPIO output high and low level selection*/
    luat_gpio_irq_cb irq_cb;/**<GPIO interrupt callback function*/
    void* irq_args;/**<User parameters during GPIO interrupt callback*/
    uint8_t alt_fun;/**<Some SOC GPIOs will be multiplexed on different pins. Use alt_fun to determine which one to use.*/
} luat_gpio_cfg_t;

/**
 * @brief GPIO pull-down\interrupt setting parameters*/
typedef enum
{
	LUAT_GPIO_CMD_SET_PULL_MODE,/**<pull-down mode*/
	LUAT_GPIO_CMD_SET_IRQ_MODE,/**<interrupt mode*/
}LUAT_GPIO_CTRL_CMD_E;

/**
 * @brief GPIO settings default parameters
 * @param luat_gpio_cfg_t*/
void luat_gpio_set_default_cfg(luat_gpio_cfg_t* gpio);
/**
 * @brief Open GPIO
 * @param luat_gpio_cfg_t*/
int luat_gpio_open(luat_gpio_cfg_t* gpio);

/**
 * @brief GPIO output level
 * @param Pin Pin serial number
 * @param Level 1 high level, 0 low level*/

int luat_gpio_set(int pin, int level);
/**
 * @brief Read GPIO input level
 * @param Pin Pin serial number
 * @return 1 high level, 0 low level, others are invalid*/
int luat_gpio_get(int pin);
/**
 * @brief Turn off GPIO
 * @param Pin Pin serial number*/
void luat_gpio_close(int pin);
/**
 * @brief Set GPIO interrupt callback function
 * @param Pin Pin serial number
 * @param cb interrupt handler function
 * @param args interrupt function parameters
 * @return -1 failure 0 success*/
int luat_gpio_set_irq_cb(int pin, luat_gpio_irq_cb cb, void* args);
/**
 * @brief GPIO analog single line output mode
 * @param Pin Pin serial number
 * @param Data output level sequence
 * @param BitLen How many bits are there in the output level sequence?
 * @param Delay The delay between each bit
 * @return none
 * @attention Outputs a group of pulses on the same GPIO. Note that the unit of len is bit, with the high bit first.*/
void luat_gpio_pulse(int pin, uint8_t *level, uint16_t len, uint16_t delay_ns);
/**
 * @brief GPIO pull-down\interrupt separate setting function
 * @param pin Pin serial number
 * @param LUAT_GPIO_CTRL_CMD_E Set command LUAT_GPIO_CMD_SET_PULL_MODE Set pull-down command LUAT_GPIO_CMD_SET_IRQ_MODE
 * @param param Set parameters. Parameters are luat from the macro definitions of pull-down and interruption.
 * @return -1 failure 0 success*/
int luat_gpio_ctrl(int pin, LUAT_GPIO_CTRL_CMD_E cmd, int param);
/**
 * @brief gpio mode outputs bit0 and bit1 to WS2812B without outputting reset. Due to strict timing requirements, interrupts will be turned off to ensure timing. Therefore, driving a large number of LED lights will have an impact on other drivers and even the entire system. It is recommended to use multiple GPIO groups to drive a large number of LED lights. It is best not to exceed 32 lights per GPIO.
 * @param pin GPIO number
 * @param data output byte data, the driver does not make any RGB order adjustment to the data, please adjust it yourself
 * @param len The number of bytes output, must be a multiple of 3
 * @param frame_cnt The number of frames sent between turning off the global interrupt and turning on the global interrupt. One frame is 3 bytes. The purpose of sending in segments is to allow other interrupts time to respond, but it causes reset to be sent in advance and the remaining lights do not light up. Write 0 to send them all at once
 * @param bit0h The high-level additional delay of bit0, the default is 10, if the high-level time is insufficient, increase it as appropriate
 * @param bit0l Low level extra delay of bit0, write 0 by default
 * @param bit1h The high-level additional delay of bit1, the default is 10, if the high-level time is insufficient, increase it as appropriate
 * @param bit1l Low level extra delay of bit1, default to write 0
 * @return -1 failure 0 success*/
int luat_gpio_driver_ws2812b(int pin, uint8_t *data, uint32_t len, uint32_t frame_cnt, uint8_t bit0h, uint8_t bit0l, uint8_t bit1h, uint8_t bit1l);
/** @}*/
/** @}*/
#endif
