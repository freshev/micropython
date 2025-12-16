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
#include "luat_kv.h"

static void kv_demo(void *param)
{
	int ret = 0;
	char value[256];

	luat_rtos_task_sleep(3000);

	LUAT_DEBUG_PRINT("luat_kv demo");

	//Set a key-value
	//The maximum length of key is 64
	//The maximum length of value is 255

	// Setting method, data ending with \0, come directly
	ret = luat_kv_set("my123", "123", strlen("123"));
	LUAT_DEBUG_PRINT("kv_set my123 ret %d", ret);
	memset(value, 0x21, 32);
	ret = luat_kv_set("my456", value, 32);
	LUAT_DEBUG_PRINT("kv_set my456 ret %d", ret);

	// luat_kv_get("my123", value, 255);
	// luat_kv_get("my456", value, 255);

	//Similarly, get value, kv_kv_get
	ret = luat_kv_get("my123", value, 255); // Pay attention to leaving a byte to put 0x00
	LUAT_DEBUG_PRINT("kv_get ret %d", ret);
	if (ret > 0) {
		value[ret] = 0x00;
		LUAT_DEBUG_PRINT("kv read value %s", value);
		//The value written in should be equal to the value written out
		if (memcmp("123", value, strlen("123"))) {
			LUAT_DEBUG_PRINT("kv value NOT match");
		}
	}
	else {
		// The previous logic writes the string "123", which will definitely be greater than 0 when obtained, unless there is an error at the bottom
		LUAT_DEBUG_PRINT("kv read failed");
	}

	luat_rtos_task_sleep(1000);
	// Clear the entire kv data, use with caution
	// This is just a demonstration of this API, it does not mean that you need to call the API after use.
	//luat_kv_clear();
	
	// The demonstration ends and the task exits.
	while (1) {
		luat_rtos_task_sleep(1000);
	}
}

static void task_demo_kv(void)
{
	
	luat_rtos_task_handle handle;
	luat_rtos_task_create(&handle, 8*1024, 50, "kv", kv_demo, NULL, 0);
}

static void demo_kv_init(void)
{
	int ret = 0;
	ret = luat_kv_init();
	LUAT_DEBUG_PRINT("luat_kv_init ret %d", ret);
}

//Start hw_demoA_init, start the hardware initial level 1
// INIT_HW_EXPORT(hw_demoA_init, "1");
//Start hw_demoB_init, start the position hardware initial level 2
// INIT_HW_EXPORT(hw_demoB_init, "2");
// Start dr_demoC_init, start position driver level 1
// INIT_DRV_EXPORT(dr_demo_kv_init, "1");
//Start dr_demoD_init, start position driver level 2
INIT_DRV_EXPORT(demo_kv_init, "2");
//Start task_demoE_init, start position task level 1
// INIT_TASK_EXPORT(task_demoE_init, "1");
//Start task_demoF_init, start position task level 2
INIT_TASK_EXPORT(task_demo_kv, "2");

