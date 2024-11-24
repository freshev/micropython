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

static void usb_serial_input_dummy_cb(uint8_t channel, uint8_t *input, uint32_t len)
{
	DBG("usb serial get %dbyte, test mode, send back", len);
	usb_serial_output(channel, input, len);
}


static void hw_demoA_init(void)
{
	DBG("this hw demo1");
	set_usb_serial_input_callback(usb_serial_input_dummy_cb);
}

static void hw_demoB_init(void)
{
	DBG("this hw demo2");
}

static void dr_demoC_init(void)
{
	DBG("this dr demo1");
}

static void dr_demoD_init(void)
{
	DBG("this dr demo2");
}

static void task1(void *param)
{
	char sn[64];
	memset(sn, 0, sizeof(sn));
	soc_get_sn(sn, sizeof(sn));
	DBG("%s", sn);
	while(1)
	{
		vTaskDelay(1000);
		DBG("utc %u", soc_get_utc());
	}
	vTaskDelete(NULL);
}

static void task2(void *param)
{
	while(1)
	{
		vTaskDelay(1000);
		DBG("utc ms %llu", soc_get_utc_ms());
	}
	vTaskDelete(NULL);
}

static void task_demoE_init(void)
{
	xTaskCreate(task1, "", 256, NULL, 20, NULL);
}

static void task_demoF_init(void)
{
	xTaskCreate(task2, "", 256, NULL, 20, NULL);
}

//Start hw_demoA_init, start the hardware initial level 1
INIT_HW_EXPORT(hw_demoA_init, "1");
//Start hw_demoB_init, start the position hardware initial level 2
INIT_HW_EXPORT(hw_demoB_init, "2");
//Start dr_demoC_init, start position driver level 1
INIT_DRV_EXPORT(dr_demoC_init, "1");
//Start dr_demoD_init, start position driver level 2
INIT_DRV_EXPORT(dr_demoD_init, "2");
//Start task_demoE_init, start position task level 1
INIT_TASK_EXPORT(task_demoE_init, "1");
//Start task_demoF_init, start position task level 2
INIT_TASK_EXPORT(task_demoF_init, "2");

