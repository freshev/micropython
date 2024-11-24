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

// This file is the entry function
// Detailed demonstrations are in example_fs_osa.c and example_fs_lfs.c

#include "common_api.h"
#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_debug.h"

extern void exmaple_fs_luat_main(void);

static void fs_example(void *param)
{
	luat_rtos_task_sleep(1500);
	exmaple_fs_luat_main();
	while(1)
	{
		luat_rtos_task_sleep(1000);
		LUAT_DEBUG_PRINT("fs demo is done");
	}
}

static void task_demoE_init(void)
{
	luat_rtos_task_handle handle;
	luat_rtos_task_create(&handle, 4*1024, 50, "fs", fs_example, NULL, 0);
}


//Start hw_demoA_init, start the hardware initial level 1
// INIT_HW_EXPORT(hw_demoA_init, "1");
//Start hw_demoB_init, start the position hardware initial level 2
// INIT_HW_EXPORT(hw_demoB_init, "2");
//Start dr_demoC_init, start position driver level 1
// INIT_DRV_EXPORT(dr_demoC_init, "1");
//Start dr_demoD_init, start position driver level 2
// INIT_DRV_EXPORT(dr_demoD_init, "2");
//Start task_demoE_init, start position task level 1
INIT_TASK_EXPORT(task_demoE_init, "1");
//Start task_demoF_init, start position task level 2
// INIT_TASK_EXPORT(task_demoF_init, "2");

