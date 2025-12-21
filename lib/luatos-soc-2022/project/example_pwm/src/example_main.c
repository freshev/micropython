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
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"

#include "luat_pwm.h"

/*1. PWM channels 3 and 5 cannot be used.
    2. PWM pin multiplexing instructions https://doc.openluat.com/wiki/37?wiki_page_id=4785*/
luat_rtos_task_handle pwm_task_handle;

static int pwm_test_callback(void *pdata, void *param)
{
	LUAT_DEBUG_PRINT("pwm done!");
}

static void task_test_pwm(void *param)
{
	/*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! ! 1*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET); 
	luat_rtos_task_sleep(2000);

	/*After pwm reconstruction, the accuracy is one thousandth. When using 100% duty cycle, you need to fill in 1000. When using 50% duty cycle, fill in 500.

channel: pwm pin selection, pwm3, pwm5 are occupied by the bottom layer and cannot be used
Example:
The incoming channel is 4, the remainder of 10 is 4, and dividing by 10 is 0, indicating that PWM4 with pin GPIO27 is used.
The incoming channel is 12, the remainder of 10 is 2, and dividing by 10 is 1, which means that the PWM2 with pin GPIO16 is used.

For the selection definition of pwm pin, refer to g_s_pwm_table in luat_pwm_ec618.c*/

	uint8_t channel = 4;
	luat_pwm_set_callback(channel, pwm_test_callback, NULL);
	//Test 13M, 50% duty cycle continuous output, look at the oscilloscope
	LUAT_DEBUG_PRINT("Test 13MHz, 50 duty cycle continuous output, look at the oscilloscope");
	luat_pwm_open(channel, 13000000, 500, 0);
	luat_rtos_task_sleep(10000);
	LUAT_DEBUG_PRINT("Test 1Hz, 50 duty cycle continuous output, output 10 waveforms to stop");
	luat_pwm_open(channel, 1, 500, 10);
	luat_rtos_task_sleep(20000);
	LUAT_DEBUG_PRINT("Test 26KHz, continuous output, duty cycle increases by 1 every 5 seconds, cycling from 0 to 100");
	luat_pwm_open(channel, 26000, 0, 0);
	uint32_t pulse_rate = 0;
    while(1)
	{
        luat_rtos_task_sleep(5000);
        pulse_rate += 10;
        if (pulse_rate > 1000)
        {
        	pulse_rate = 0;
        }
        LUAT_DEBUG_PRINT("Current duty cycle %u", pulse_rate/10);
        luat_pwm_update_dutycycle(channel, pulse_rate);
	} 
    
}

static void task_demo_pwm(void)
{
    luat_rtos_task_create(&pwm_task_handle, 2048, 20, "pwm", task_test_pwm, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_pwm,"1");



