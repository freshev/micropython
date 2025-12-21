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
#include "luat_gpio.h"
#define TEST_PIN 	HAL_GPIO_24

/*Note: ws2812 is mounted on the Cat.1 Modules. If used in a network environment, there will be interference because the network priority is the highest.
It will cause timing interference and cause the color of a certain lamp bead to be abnormal. The effect is not very good and is not recommended. If the impact is considered to be large, it is recommended to implement it through an external MCU.*/



static void task_test_ws2812(void *param)
{
	/*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! ! 1*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET);
	uint8_t cnt;
	uint8_t color[3 * 64];
	luat_rtos_task_sleep(1);
	cnt = 0;
    while(1)
	{
    	memset(color, 0, sizeof(color));
    	for(int i = 0; i < 64; i++)
    	{
    		color[i * 3 + cnt] = 0xff;
    	}
    	luat_gpio_driver_ws2812b(TEST_PIN, color, sizeof(color), 0, 10, 0, 10, 0); //If it is unstable, change frame_cnt to 0, but it will turn off the interrupt for a long time.
    	luat_rtos_task_sleep(1000);
    	cnt = (cnt + 1) % 3;

	} 
    
}

static void task_demo_ws2812b(void)
{
	luat_rtos_task_handle task_handle;
	luat_gpio_cfg_t gpio_cfg = {0};
	gpio_cfg.pin = TEST_PIN;
	gpio_cfg.output_level = 0;
	luat_gpio_open(&gpio_cfg);
    luat_rtos_task_create(&task_handle, 2048, 20, "ws2812b", task_test_ws2812, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_ws2812b,"1");



