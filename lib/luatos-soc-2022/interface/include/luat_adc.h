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

#ifndef Luat_ADC_H
#define Luat_ADC_H

#include "luat_base.h"
#include "luat_adc_legacy.h"

#define LUAT_ADC_CH_CPU  (-1)
#define LUAT_ADC_CH_VBAT (-2)

/**
 * @ingroup luatos_device peripheral interface
 * @{*/
/**
 * @defgroup luatos_device_adc ADC interface
 * @{*/
/**
 * luat_adc_open
 * @brief Open an adc channel
 *
 * @param pin[in] The sequence number of the adc channel
 * @param args[in] reserved, pass NULL
 * @return 0 success, other values     failure*/
int luat_adc_open(int pin, void *args);

/**
 * luat_adc_read
 * @brief Read the value of adc channel
 *
 * @param pin[in] The sequence number of the adc channel
 * @param val[out] The original value of adc channel
 * @param val2[out] The calculated value of adc channel, related to the specific channel
 * @return 0 success, other values     failure*/
int luat_adc_read(int pin, int *val, int *val2);

/**
 * luat_adc_close
 * @brief close adc channel
 *
 * @param pin[in] The sequence number of the adc channel
 * @return 0 success, other values     failure*/
int luat_adc_close(int pin);

/**
 * @brief ADC control command*/
typedef enum LUAT_ADC_CTRL_CMD
{
	LUAT_ADC_SET_GLOBAL_RANGE,/**< Range*/
}LUAT_ADC_CTRL_CMD_E;

/**
 * @brief ADC measurement range (range) is related to the specific chip: Yixin 618 chip, LUAT_ADC_RANGE_1_2 means 1.2V, there is no internal voltage divider, and the other ranges have internal voltage dividers*/
typedef enum LUAT_ADC_RANGE
{
	LUAT_ADC_AIO_RANGE_1_2,
	LUAT_ADC_AIO_RANGE_1_4,
	LUAT_ADC_AIO_RANGE_1_6,
	LUAT_ADC_AIO_RANGE_1_9,
	LUAT_ADC_AIO_RANGE_2_4,
	LUAT_ADC_AIO_RANGE_2_7,
	LUAT_ADC_AIO_RANGE_3_2,
	LUAT_ADC_AIO_RANGE_3_8,
	LUAT_ADC_AIO_RANGE_MAX,
	// The following configuration is no longer supported and is meaningless
	// LUAT_ADC_AIO_RANGE_4_8,
	// LUAT_ADC_AIO_RANGE_6_4,
	// LUAT_ADC_AIO_RANGE_9_6,
	// LUAT_ADC_AIO_RANGE_19_2,

	LUAT_ADC_VBAT_RANGE_2_0_RATIO,
	LUAT_ADC_VBAT_RANGE_2_2_RATIO,
	LUAT_ADC_VBAT_RANGE_2_6_RATIO,
	LUAT_ADC_VBAT_RANGE_3_2_RATIO,
	LUAT_ADC_VBAT_RANGE_4_0_RATIO,
	LUAT_ADC_VBAT_RANGE_5_3_RATIO,
	LUAT_ADC_VBAT_RANGE_8_0_RATIO,
	LUAT_ADC_VBAT_RANGE_16_0_RATIO,
}LUAT_ADC_RANGE_E;

/**
 * @brief ADC control parameters*/
typedef union luat_adc_ctrl_param
{	
	LUAT_ADC_RANGE_E range;/**< adc range*/
	void *userdata;/**< Reserved*/
} luat_adc_ctrl_param_t;

/**
 * luat_adc_ctrl
 * @brief adc control
 *
 * @param pin[in] The sequence number of the adc channel
 * @param cmd adc control command
 * @param param adc control parameters
 * @return 0 success, other values     failure*/
int luat_adc_ctrl(int pin, LUAT_ADC_CTRL_CMD_E cmd, luat_adc_ctrl_param_t param);
/** @}*/
/** @}*/
#endif
