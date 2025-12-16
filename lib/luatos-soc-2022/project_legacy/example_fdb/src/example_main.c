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
#include "FreeRTOS.h"
#include "task.h"

#include "am_kv.h"

static void fdb_demo(void *param)
{
	int ret = 0;
	char value[256];

	vTaskDelay(3000);

	DBG("am_kv demo");
	

	//Set a key-value
	//The maximum length of key is 255, it is recommended not to exceed 64
	//The maximum length of value is 4096 - 1

	// Setting mode 1, data ending in \0, come directly
	ret = am_kv_set("my123", "123", strlen("123"));
	DBG("kv_set my123 ret %d", ret);
	memset(value, 0x21, 32);
	ret = am_kv_set("my456", value, 32);
	DBG("kv_set my456 ret %d", ret);

	// am_kv_get("my123", value, 255);
	// am_kv_get("my456", value, 255);
	
	//Similarly, there are two ways to get value, fdb_kv_get and fdb_kv_get_blob
	ret = am_kv_get("my123", value, 255); // Pay attention to leaving one byte for 0x00
	DBG("kv_get ret %d", ret);
	if (ret > 0) {
		value[ret] = 0x00;
		DBG("fdb read value %s", value);
		//The value written in should be equal to the value written out
		if (memcmp("123", value, strlen("123"))) {
			DBG("fdb value NOT match");
		}
	}
	else {
		// The previous logic writes the string "123", which will definitely be greater than 0 when obtained, unless there is an error at the bottom
		DBG("fdb read failed");
	}

	vTaskDelay(1000);
	// Clear the entire fdb data, use with caution
	// This is just a demonstration of this API, it does not mean that you need to call the API after use.
	//am_kv_clear();
	
	// The demonstration ends and the task exits.
	vTaskDelete(NULL);
}

static void task_demo_fdb_init(void)
{
	xTaskCreate(fdb_demo, "fdb", 4*1024, NULL, 20, NULL);
	// xTaskCreate(fdb_demo2, "fdb", 8*1024, NULL, 20, NULL);
}

static void dr_fdb_init(void)
{
	int ret = 0;
	ret = am_kv_init();
	DBG("am_kv_init ret %d", ret);
}

//Start hw_demoA_init, start the hardware initial level 1
// INIT_HW_EXPORT(hw_demoA_init, "1");
//Start hw_demoB_init, start the position hardware initial level 2
// INIT_HW_EXPORT(hw_demoB_init, "2");
// Start dr_demoC_init, start position driver level 1
// INIT_DRV_EXPORT(dr_demo_fdb_init, "1");
//Start dr_demoD_init, start position driver level 2
INIT_DRV_EXPORT(dr_fdb_init, "2");
//Start task_demoE_init, start position task level 1
// INIT_TASK_EXPORT(task_demoE_init, "1");
//Start task_demoF_init, start position task level 2
INIT_TASK_EXPORT(task_demo_fdb_init, "2");

