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
#include "luat_flash.h"
#include "luat_crypto.h"

#include "mem_map.h"

/*This demo is an advanced operation, please be sure to read the tips first

The overall layout of Flash is as follows, corresponding to non-LuatOS applications:

AP flash layout, toatl 4MB
flash raw address: 0x00000000---0x00400000
flash xip address(from ap view): 0x00800000---0x00c00000


0x00000000 |---------------------------------|
                    | header1 8KB |
0x00002000 |---------------------------------|
                    | header2 8KB |
0x00004000 |---------------------------------|
                    | bl part1 32KB |
0x0000c000 |---------------------------------|
                    | bl part2 96KB |
0x00024000 |---------------------------------|
                    | app img 2.5MB + 384k |------OTA write
0x00304000 |---------------------------------|
                    | fota 512KB |-----OTA download write
0x00384000 |---------------------------------|
                    | lfs 288KB |-----FS write
0x003cc000 |---------------------------------|
                    | kv 64KB |-----kv write
0x003dc000 |---------------------------------|
                    | rel_ap(factory) 16KB |-----factory write
0x003e0000 |---------------------------------|
                    | rel_ap 16KB |-----factory write
0x003e4000 |---------------------------------|
                    | hib backup 96KB |-----hib write
0x003fc000 |---------------------------------|
                    | plat config 16KB |-----similar as FS
0x00400000 |---------------------------------|


**Notice**:
1. If you want to read data directly, you can read it directly through the above address +0x00800000, without going through luat_flash.h
2. If you need to write or erase data, you need to call luat_flash.h

As you can see from the picture above, there are only two areas that can be moved:
1. KV database area, if you do not use luat_kv at all, you can use it directly
2. AP space, if you determine that you do not need to use all the space, you can crop it, but you need to change mem_map.h*/

static void flash_example(void *param)
{
	luat_rtos_task_sleep(1500);
	fotaNvmNfsPeInit(1);	//Allow the last 896KB space of app.img to be opened for writing

	luat_rtos_task_sleep(2000);
	
	char buff[256] = {0};
	

	uint32_t addr = 0x00284000; // This is the last 512KB of app.img

	// There are not many skills in flash reading and writing
	luat_flash_read(buff, addr, 256);

	luat_flash_erase(addr, 4096);

	// Generate some random data to simulate business data
	luat_crypto_trng(buff, 256);

	luat_flash_write(buff, addr, 256);

	luat_flash_read(buff, addr, 256);
	fotaNvmNfsPeInit(0);	//After the test is completed, you can close and open it
	while(1)
	{
		luat_rtos_task_sleep(1000);
		LUAT_DEBUG_PRINT("flash demo is done");
	}
}

static void task_demoE_init(void)
{
	luat_rtos_task_handle handle;
	luat_rtos_task_create(&handle, 16*1024, 50, "flash", flash_example, NULL, 0);
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

