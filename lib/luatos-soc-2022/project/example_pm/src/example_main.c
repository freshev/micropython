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
#include "luat_pm.h"

luat_rtos_task_handle pm_task_handle;

//Callback function before sleeping
luat_pm_sleep_callback_t sleep_pre_callback(int mode)
{
    LUAT_DEBUG_PRINT("pm demo sleep before %d", mode);
}

//Callback function after waking up
luat_pm_sleep_callback_t sleep_post_callback(int mode)
{
    LUAT_DEBUG_PRINT("pm demo sleep post %d", mode);
}

//wakepad callback function
luat_pm_wakeup_pad_isr_callback_t pad_wakeup_callback(LUAT_PM_WAKEUP_PAD_E num)
{
    LUAT_DEBUG_PRINT("pm demo wakeup pad num %d", num);
}



static void task_test_pm(void *param)
{
    luat_rtos_task_sleep(20000);
    luat_pm_wakeup_pad_set_callback(pad_wakeup_callback);
    luat_pm_wakeup_pad_cfg_t cfg = {0};
    cfg.neg_edge_enable = 1;
    cfg.pos_edge_enable = 0;
    cfg.pull_down_enable = 0;
    cfg.pull_up_enable = 1;
    luat_pm_wakeup_pad_set(1, LUAT_PM_WAKEUP_PAD_3, &cfg);
    int count=0;
    while (1)
    {
         LUAT_DEBUG_PRINT("pm count %d", count);
         count++;
        //Get vbus insertion status
        uint8_t status;
        LUAT_DEBUG_PRINT("pm demo get vbus status %d %d", luat_pm_get_vbus_status(&status), status);

        //Register the callback function before sleeping
        luat_pm_sleep_register_pre_handler(sleep_pre_callback);

        //Register the callback function after waking up
        luat_pm_sleep_register_post_handler(sleep_post_callback);

        //Set current mark 1 to none mode
        luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_NONE, "1");                             
        LUAT_DEBUG_PRINT("pm demo get sleep mode %d", luat_pm_get_sleep_mode("1"));
        luat_rtos_task_sleep(5000);

        //Set current mark 1 to idle mode
        luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_IDLE, "1");
        LUAT_DEBUG_PRINT("pm demo get sleep mode %d", luat_pm_get_sleep_mode("1"));
        luat_rtos_task_sleep(5000);

        //Set current mark 2 to idle mode
        luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_IDLE, "2");
        LUAT_DEBUG_PRINT("pm demo get sleep mode %d", luat_pm_get_sleep_mode("2"));
        luat_rtos_task_sleep(5000);
        
        //Set the current mark 1 to light mode. At this time, mark 2 is idle mode, so it cannot enter light mode.
        luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, "1");
        LUAT_DEBUG_PRINT("pm demo get sleep mode %d", luat_pm_get_sleep_mode("1"));
        luat_rtos_task_sleep(5000);


        //Set the current mark 2 to light mode. At this time, mark 1 and mark 2 are both in light mode, and the Modules enters light mode.
        luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, "2");
        LUAT_DEBUG_PRINT("pm demo get sleep mode %d", luat_pm_get_sleep_mode("2"));
        luat_rtos_task_sleep(5000);
         LUAT_DEBUG_PRINT("pm count %d", count);
        //Unregister the callback function before sleeping
        luat_pm_sleep_deregister_pre_handler();

        //Unregister the callback function after waking up
        luat_pm_sleep_deregister_post_handler();
        luat_rtos_task_sleep(5000);

        //Open this line to comment and test shutdown
        // luat_pm_poweroff();


        // Open this line to comment and test restart
        // luat_pm_reboot();
    }
}

static void pm(void)
{
    luat_rtos_task_create(&pm_task_handle, 1024, 20, "pm", task_test_pm, NULL, NULL);
}

INIT_TASK_EXPORT(pm, "1");
