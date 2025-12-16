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

#include "luat_adc.h"

luat_rtos_task_handle adc_task_handle;

static void task_test_adc(void *param)
{
    int val, val2;

    //The description of adc0 and adc1 is as follows:
    // 1. The maximum input voltage allowed by the adc pin is 3.4V
    // 2. If internal voltage division is used and no external voltage division is used, the software can set the measurement range by calling the luat_adc_ctrl interface, which supports 1.2V, 1.4V, 1.6, 1.9, 2.4, 2.7, 3.2, and 3.8V ranges;
    // 3. If external voltage dividing is used and internal voltage is not divided, the software can set the range to 1.2V by calling the luat_adc_ctrl interface;
    // 4. It is not recommended to activate internal pressure partial pressure and external pressure partial pressure at the same time, as this error will become larger.

    //External input voltage, if it exceeds 3.4v:
    // Only external voltage division can be used, and no internal voltage division is used. The software sets the range to 1.2V by calling the luat_adc_ctrl interface;

    //External input voltage, if it does not exceed 3.4V:
    // 1. You can use external voltage division and no internal voltage division. The software can set the range to 1.2V by calling the luat_adc_ctrl interface;
    // 2. You can also use internal voltage division and no external voltage division. The software calls the luat_adc_ctrl interface to set the range. If the set range is greater than or equal to the actual input maximum voltage, just select the closest range;
    // Considering the hardware cost, the second option is recommended;
    // From the perspective of measurement accuracy, in the second method, 12bit represents the maximum range of the setting, and the maximum range/4096 is the measurement accuracy; in the first method, calculate and compare by yourself based on the external hardware circuit;

    // Just choose one of the internal and external partial pressures; when external partial pressure is not possible, select internal partial pressure;
    // If an external voltage divider is used, it is recommended to do it in one step. After the external voltage divider, the input voltage is in the range of 0.1-1.2V (the pass-through mode is used internally, and no voltage division is required). The closer to 1.2V, the better, and the higher the accuracy.
    // If internal voltage division is used, the closer the configured range is to 1.2V, the better, and the higher the accuracy, provided that the measurement requirements are met.

    // adc has the internal voltage divider turned on by default, and the configured range is 3.8V;
    // The following three lines of commented-out code set the range of adc0 to 1.2V and turn off the internal voltage divider; it is recommended that the user use an external voltage divider;
    // If the external voltage dividing method is used, the user can open the following three lines of code according to his own needs;
    // luat_adc_ctrl_param_t ctrl_param;
	// ctrl_param.range = LUAT_ADC_AIO_RANGE_1_2;
	// luat_adc_ctrl(0, LUAT_ADC_SET_GLOBAL_RANGE, ctrl_param);


    //The descriptions of LUAT_ADC_CH_CPU and LUAT_ADC_CH_VBAT are as follows:
    // LUAT_ADC_CH_CPU is used internally by the chip to detect temperature. It only supports open, read and close operations on the software. Users cannot use it for other purposes;
    // LUAT_ADC_CH_VBAT is used internally by the chip to detect the VBAT voltage. It only supports open, read and close operations in the software. Users cannot use it for other purposes;

    luat_adc_open(0 , NULL);
    luat_adc_open(1 , NULL);
    luat_adc_open(LUAT_ADC_CH_CPU, NULL);
    luat_adc_open(LUAT_ADC_CH_VBAT, NULL);
    while (1)
    {
        luat_rtos_task_sleep(1000);
        luat_adc_read(0 , &val, &val2);
        LUAT_DEBUG_PRINT("adc0: adc original value %d, voltage %d microvolts",val, val2);
        luat_adc_read(1 , &val, &val2);
        LUAT_DEBUG_PRINT("adc1: adc original value %d, voltage %d microvolts",val, val2);
        luat_adc_read(LUAT_ADC_CH_CPU, &val, &val2);
        LUAT_DEBUG_PRINT("temp: adc original value %d, %d degrees Celsius",val, val2);
        luat_adc_read(LUAT_ADC_CH_VBAT, &val, &val2);
        LUAT_DEBUG_PRINT("vbat: adc original value %d, voltage %d millivolts",val, val2);
    }
    
}

static void task_demo_adc(void)
{
    luat_rtos_task_create(&adc_task_handle, 1024, 20, "adc", task_test_adc, NULL, NULL);
}

INIT_TASK_EXPORT(task_demo_adc,"1");



