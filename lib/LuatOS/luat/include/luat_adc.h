/**************************************************** ***************************
 * ADC device operation abstraction layer
 * @author wendal
 * @since 0.1.5
 *************************************************** ****************************/
#ifndef Luat_ADC_H
#define Luat_ADC_H

#include "luat_base.h"
/**
 * @defgroup luatos_device_adc ADC interface
 * @{*/
#define LUAT_ADC_CH_CPU		(-1)
#define LUAT_ADC_CH_VBAT	(-2)

typedef enum
{
	ADC_SET_GLOBAL_RANGE = 0x80,
}ADC_SET_CMD_ENUM;

/**
 * @brief ADC control command*/
typedef enum LUAT_ADC_CTRL_CMD
{
	LUAT_ADC_SET_GLOBAL_RANGE,/**< Range*/
}LUAT_ADC_CTRL_CMD_E;

/// @brief ADC range
typedef enum LUAT_ADC_RANGE
{
	LUAT_ADC_AIO_RANGE_1_2,
	LUAT_ADC_AIO_RANGE_1_4,
	LUAT_ADC_AIO_RANGE_1_6,
	LUAT_ADC_AIO_RANGE_1_9,
	LUAT_ADC_AIO_RANGE_2_4,
	LUAT_ADC_AIO_RANGE_2_6,
	LUAT_ADC_AIO_RANGE_2_7,
	LUAT_ADC_AIO_RANGE_3_2,
	LUAT_ADC_AIO_RANGE_3_8,
	LUAT_ADC_AIO_RANGE_4_0,
	LUAT_ADC_AIO_RANGE_4_8,
	LUAT_ADC_AIO_RANGE_6_4,
	LUAT_ADC_AIO_RANGE_9_6,
	LUAT_ADC_AIO_RANGE_19_2,
	LUAT_ADC_AIO_RANGE_MAX,
	
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
 * luat_adc_open
 * Description: Open an adc channel
 * @param pin[in] The sequence number of the adc channel
 * @param args[in] reserved, pass NULL
 * @return 0 success, other values     failure*/
int luat_adc_open(int pin, void* args);

/**
 * luat_adc_read
 * Description: Read the value of adc channel
 * @param pin[in] The sequence number of the adc channel
 * @param val[out] The original value of adc channel
 * @param val2[out] The calculated value of adc channel, related to the specific channel
 * @return 0 success, other values     failure*/
int luat_adc_read(int pin, int* val, int* val2);

/**
 * luat_adc_close
 * Description: Close adc channel
 * @param pin[in] The sequence number of the adc channel
 * @return 0 success, other values     failure*/
int luat_adc_close(int pin);

/**
 * luat_adc_global_config
 * Description: Set adc global parameters
 * @param tp[in] parameter type
 * @param val[in] parameter value
 * @return 0 success, other values     failure*/
int luat_adc_global_config(int tp, int val);

/**
 * luat_adc_ctrl
 * Description: Set ADC parameters, some functions will have the same thought as luat_adc_global_config
 * @param id[in] Serial number of adc channel
 * @param cmd[in] parameter type
 * @param param[in] parameter value
 * @return 0 success, other values     failure*/
int luat_adc_ctrl(int id, LUAT_ADC_CTRL_CMD_E cmd, luat_adc_ctrl_param_t param);
/** @}*/
#endif
