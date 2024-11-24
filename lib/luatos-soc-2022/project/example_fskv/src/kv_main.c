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


//The fskv space can use the free space in the program area or the fdb space (not compatible with the old kv)

#include "common_api.h"

#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_fskv.h"
#include "mem_map.h"

static void kv_demo(void *param)
{
	int ret = 0;
	char value[128] = {0};
	char value2[128] = {0};
	/*After an exception occurs, the default is to crash and restart.
The demo here is set to LUAT_DEBUG_FAULT_HANG_RESET. After an exception occurs, try to upload the crash information to the PC tool. The upload is successful or restarts after timeout.
If you want to facilitate debugging, you can set it to LUAT_DEBUG_FAULT_HANG. If an exception occurs, it will crash without restarting.
However, mass production shipments must be set to restart in case of an exception! ! ! ! ! ! ! ! !*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET); 
	ret = luat_fskv_init(FLASH_FDB_REGION_START + AP_FLASH_XIP_ADDR, FLASH_FDB_REGION_START, 64*1024);


	LUAT_DEBUG_PRINT("luat_kv demo");

	//Set a key-value
	//The maximum length of key is 64
	//The maximum length of value is 4096

	// Setting method, data ending with \0, come directly
	ret = luat_fskv_set("my123", "123", strlen("123"));
	LUAT_DEBUG_PRINT("kv_set my123 ret %d", ret);
	memset(value, 0x21, 32);
	ret = luat_fskv_set("my456", value, 32);
	LUAT_DEBUG_PRINT("kv_set my456 ret %d", ret);

	ret = luat_fskv_get("my123", value, 32);
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

	for(ret = 0; ret < 128; ret++)
	{
		value2[ret] = ret;
	}
	ret = luat_fskv_set("hextest", value2, 128);
	ret = luat_fskv_get("hextest", value, 128);

	if (memcmp(value, value2, 128))
	{
		LUAT_DEBUG_PRINT("test failed");
	}

	luat_rtos_task_sleep(1000);
	// Clear the entire kv data, use with caution
	// This is just a demonstration of this API, it does not mean that you need to call the API after use.
//	luat_fskv_clear();
	
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

INIT_TASK_EXPORT(task_demo_kv, "1");

