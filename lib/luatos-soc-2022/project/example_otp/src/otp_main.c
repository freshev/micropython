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
#include "luat_otp.h"
#include "luat_crypto.h"

static void otp_example(void *param)
{
	luat_rtos_task_sleep(1500);
	// EC618 has a total of 3 otp partitions, numbered 1, 2, and 3 respectively.
	//The size of each partition is 256 bytes
	char buff[256] = {0};

	// There are not many skills in OTP reading and writing
	LUAT_DEBUG_PRINT("read result %d", luat_otp_read(1, buff, 0, 256));

	LUAT_DEBUG_PRINT("erase result %d", luat_otp_erase(1, 0, 256));

	// Generate some random data to simulate business data
	luat_crypto_trng(buff, 256);
	LUAT_DEBUG_PRINT("write result %d", luat_otp_write(1, buff, 0, 256));

	// The only thing worth noting is the lock function
	// Once locked, erase and write are no longer possible, test carefully
	// LUAT_DEBUG_PRINT("lock result %d", luat_otp_lock(1));

	while(1)
	{
		luat_rtos_task_sleep(1000);
		LUAT_DEBUG_PRINT("otp demo is done");
	}
}

static void task_demoE_init(void)
{
	luat_rtos_task_handle handle;
	luat_rtos_task_create(&handle, 4*1024, 50, "otp", otp_example, NULL, 0);
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

