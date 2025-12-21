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
#include "luat_wdt.h"

/*The following precautions must be luat when using the watchdog on the Yixin 618 platform:
1. For description of watchdog characteristics, please refer to: https://doc.openluat.com/wiki/37?wiki_page_id=4586
2. The interface in luat_wdt.h should not be called in the processing functions in INIT_HW_EXPORT, INIT_DRV_EXPORT, and INIT_TASK_EXPORT because:
   After calling here, the SDK system will initialize the watchdog later, which will overwrite the user's configuration;
   For the usage location, please refer to the call to the luat_wdt_set_timeout interface in task_dead_loop_run in this file.*/

static luat_rtos_task_handle feed_wdt_task_handle;

static void task_feed_wdt_run(void *param)
{                     
    while (1)
    {
        //The watchdog is turned on by default, the timeout is 20s, and there is automatic watchdog feeding logic. The user does not need to start the watchdog again, he only needs to feed the watchdog on time according to his own logic.
        luat_wdt_feed(); 
        LUAT_DEBUG_PRINT("feed wdt");
        luat_rtos_task_sleep(1000);                            
    }
    luat_rtos_task_delete(feed_wdt_task_handle);
    
}

static void task_feed_wdt_init(void)
{
    luat_rtos_task_create(&feed_wdt_task_handle, 2048, 20, "feed_wdt", task_feed_wdt_run, NULL, NULL);
}


static luat_rtos_task_handle dead_loop_task_handle;

static void task_dead_loop_run(void *param)
{
    while (1)
    {
        LUAT_DEBUG_PRINT("dead loop");
    }
    luat_rtos_task_delete(dead_loop_task_handle);
    
}

static void task_dead_loop_init(void)
{   
    luat_rtos_task_create(&dead_loop_task_handle, 2048, 50, "dead_loop", task_dead_loop_run, NULL, NULL);
}

INIT_TASK_EXPORT(task_feed_wdt_init,"1");
// This task simulates software exceptions by executing an infinite loop. The watchdog will automatically restart after 20 seconds when the watchdog times out.
// Only used for simulation testing, you can open it yourself if necessary
// INIT_TASK_EXPORT(task_dead_loop_init,"2");



