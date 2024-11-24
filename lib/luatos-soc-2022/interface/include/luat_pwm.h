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
#ifndef LUAT_PWM_H
#define LUAT_PWM_H
/**
 *@version V1.0
 *@attention
   1. Using int luat_pwm_open(int channel, size_t period, size_t freq, int pnum);
    During the function, set the PWM channels, set the PWM frequency, set the duty cycle, and the number of PWMs in sequence.
    If you need to set the number of PWM, the PWM frequency should not be higher than 50K. If it is higher than 50K, there may be a few more numbers.

   2.int luat_pwm_capture(int channel,int freq); Get pwm frequency
    This function to obtain the PWM frequency function has not yet been implemented*/
/**
 * @ingroup luatos_device peripheral interface
 * @{*/

/**
 * @defgroup luatos_device_PWM PWM interface
 * @{*/
#include "luat_base.h"
/**
 * @brief PWM control parameters*/
typedef struct luat_pwm_conf {
    int channel;       /**<PWM channel, the optional channels are 0 / 1 / 2 / 4, a total of 4 channels*/
    size_t period;   /**<frequency, 1Hz - 13MHz*/
    size_t pulse;    /**<Duty cycle, 0-100 If pulse is set to 50, the output high level time accounts for 50% of the cycle.*/
    size_t pnum;     /**<Output period 0 is continuous output, 1 is single output, others are specified pulse number output*/
    size_t precision;  /**<Frequency division accuracy, 100/256/1000, the default is 100, if the device does not support it, there will be a log prompt*/
} luat_pwm_conf_t;


/**
 * @brief configure pwm parameters
 *
 * @param conf->channel: Select the PWM channel. The optional channels are 0 / 1 / 2 / 4, a total of 4 channels.
 * conf->period: Set the generated PWM frequency
 * conf->pulse: Set the generated PWM duty cycle
 * conf->pnum: Set the number of PWM generated. If pnum is set to 0, PWM will always be output.
 * @return int
 * Return value is 0: PWM configuration is successful
 * The return value is -1: PWM channel selection error
 * The return value is -2: PWM frequency setting error
 * The return value is -3: PWM duty cycle setting error
 * The return value is -4: The PWM channel has been used*/

int luat_pwm_setup(luat_pwm_conf_t* conf);

/**
 * @brief open pwm channel
 *
 * @param channel: Select the PWM channel. The available channels are 0 / 1 / 2 / 4, a total of 4 channels.
 * period: Set the generated PWM frequency
 * pulse: Set the generated PWM duty cycle, unit 0.1%
 * pnum: Set the number of PWM generated. If pnum is set to 0, PWM will always be output.
 * @return int
 * Return value is 0: PWM configuration is successful
 * The return value is -1: PWM channel selection error
 * The return value is -2: PWM frequency setting error
 * The return value is -3: PWM duty cycle setting error
 * The return value is -4: The PWM channel has been used*/

int luat_pwm_open(int channel, size_t period, size_t pulse, int pnum);

/**
 * @brief Get pwm frequency This function has not been implemented yet
 *
 * @param id i2c_id
 * @return int*/

int luat_pwm_capture(int channel,int freq);

/**
 * @brief Close the pwm interface
 *
 * @param channel: Select the PWM channel. The available channels are 0 / 1 / 2 / 4, a total of 4 channels.
 * @return int*/
int luat_pwm_close(int channel);

/**
 * @brief Modify duty cycle
 * @param channel: Select the pwm channel. The available channels are 0 / 1 / 2 / 4, a total of 4 channels.
 * pulse: modify the pwm duty cycle value
 * @return int*/
int luat_pwm_update_dutycycle(int channel,size_t pulse);

/**
 * @brief Set the PWM output completion callback. There is a callback only when open and pnum is not 0. It must be set before pwm open.
 * @param channel: Select the pwm channel. The available channels are 0 / 1 / 2 / 4, a total of 4 channels.
 * callback: callback function
 * param: user parameters during callback
 * @return int*/
int luat_pwm_set_callback(int channel, CBFuncEx_t callback, void *param);
/** @}*/

/**
 * @brief Whether to enable fast update duty cycle
 * @param channel: Select the pwm channel. The available channels are 0 / 1 / 2 / 4, a total of 4 channels.
 * on_off: 0 is not enabled, others are enabled. It is not enabled by default. 
 * Once enabled, the duty cycle will no longer be updated at the appropriate time, but will be updated immediately. 
 * EC618 platform limitation, if the duty cycle is updated at the wrong time, it may cause an abnormal PWM duty cycle of 1 cycle
 * @return void*/
void luat_pwm_config_update_no_delay(int channel, uint8_t on_off);
/** @}*/
/** @}*/
#endif
