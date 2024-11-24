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
#include "luat_mem.h"
#include "luat_debug.h"
#include "mem_map.h"
luat_rtos_task_handle task1_handle;

static void check(uint32_t time, uint32_t address)
{
	if (time >= 300)
	{
		LUAT_DEBUG_PRINT("Address %x erase time is too long %ums, service life is exhausted", address, (uint32_t)(time));
	}
	else if (time >= 200)
	{
		LUAT_DEBUG_PRINT("Address %x erase time is too long %ums, service life is over half", address, (uint32_t)(time));
	}
	else if (time >= 100)
	{
		LUAT_DEBUG_PRINT("Address %x erase time is too long %ums, used frequently", address, (uint32_t)(time));
	}
	else if (time >= 50)
	{
		LUAT_DEBUG_PRINT("Address %x erase time is too long %ums, pay attention to the number of uses", address, (uint32_t)(time));
	}
	else
	{
		LUAT_DEBUG_PRINT("Address %x was erased successfully", address);
	}
}

static void task(void *param)
{
	uint64_t start_ms, end_ms;
	uint32_t address;
	luat_mobile_set_flymode(0, 1);
	luat_rtos_task_sleep(2000);
	LUAT_DEBUG_PRINT("Detect FOTA area flash");
	for(address = FLASH_FOTA_REGION_START; address < FLASH_FOTA_REGION_END; address += 4096)
	{
		start_ms = luat_mcu_tick64_ms();
		if (luat_flash_erase(address, 4096))
		{
			LUAT_DEBUG_PRINT("Address %x erase failed", address);
		}
		end_ms = luat_mcu_tick64_ms();
		check((uint32_t)(end_ms - start_ms), address);
	}
	LUAT_DEBUG_PRINT("Detect FS area flash");
	for(address = FLASH_FS_REGION_START; address < FLASH_FS_REGION_END; address += 4096)
	{
		start_ms = luat_mcu_tick64_ms();
		if (luat_flash_erase(address, 4096))
		{
			LUAT_DEBUG_PRINT("Address %x erase failed", address);
		}
		end_ms = luat_mcu_tick64_ms();
		check((uint32_t)(end_ms - start_ms), address);
	}
	LUAT_DEBUG_PRINT("Detect kv area flash");
	for(address = FLASH_FDB_REGION_START; address < FLASH_FDB_REGION_END; address += 4096)
	{
		start_ms = luat_mcu_tick64_ms();
		if (luat_flash_erase(address, 4096))
		{
			LUAT_DEBUG_PRINT("Address %x erase failed", address);
		}
		end_ms = luat_mcu_tick64_ms();
		check((uint32_t)(end_ms - start_ms), address);
	}

	while(1)
	{
		luat_rtos_task_sleep(1000);
		LUAT_DEBUG_PRINT("test done");
	}
}

static void task_demo_init(void)
{
	luat_rtos_task_create(&task1_handle, 8*1024, 50, "test", task, NULL, 0);
}

//Start task_demoE_init, start position task level 1
INIT_TASK_EXPORT(task_demo_init, "1");

