
#ifndef Luat_PWM_H
#define Luat_PWM_H

#include "luat_base.h"

/**
 * @defgroup luatos_device_PWM PWM interface
 * @{*/
/**
 * @brief PWM control parameters*/
typedef struct luat_pwm_conf {
    int channel; /**<PWM channel, the optional channels are 0 / 1 / 2 / 4, a total of 4 channels*/
    int stop_level;
    size_t period;/**<frequency, 1Hz - 13MHz*/
    size_t pulse; /**<Duty cycle, 0-100 If pulse is set to 50, the output high level time accounts for 50% of the cycle.*/
    size_t pnum; /**<Output period 0 is continuous output, 1 is single output, others are specified pulse number output*/
    size_t precision;/**<Frequency division accuracy, 100/256/1000, the default is 100, if the device does not support it, there will be a log prompt*/
    uint8_t reverse;
} luat_pwm_conf_t;

#ifdef __LUATOS__
#else
/**
 * @brief Set the PWM output completion callback. There is a callback only when open and pnum is not 0. It must be set before pwm open.
 * @param channel: Select the pwm channel. The available channels are 0 / 1 / 2 / 4, a total of 4 channels.
 * callback: callback function
 * param: user parameters during callback
 * @return int*/
int luat_pwm_set_callback(int channel, CBFuncEx_t callback, void *param);
#endif
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
int luat_pwm_update_dutycycle(int channel, size_t pulse);
/** @}*/
#endif
