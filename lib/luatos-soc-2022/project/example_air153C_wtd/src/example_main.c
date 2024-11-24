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
#include "luat_air153C_wtd.h"
#include "luat_pm.h"
#include "luat_uart.h"
#include "luat_gpio.h"
#include "plat_config.h"

/*External watchdog chip implements watchdog*/

#define WTD_NOT_FEED_TEST       0       //Test not to feed the dog
#define WTD_FEED_TEST           1       //Test feeding the dog
#define WTD_CLOSE_FEED          0       //Close watchdog feeding
#define CLOSE_FEED_AND_FEED     0       //Close watchdog feeding and then open it again
#define TEST_MODE_RESET_TEST    0       //Test mode reset

static luat_rtos_task_handle feed_wdt_task_handle;

// void soc_get_unilog_br(uint32_t *baudrate)
// {
// *baudrate = 3000000; //UART0 is used as a log port to output a 12M baud rate, and high-performance USB must be used to convert it to TTL.
// }

static void task_feed_wdt_run(void *param)
{   
    LUAT_DEBUG_PRINT("[DIO] ------------");

    /*The following if code uses UART for LOG debugging*/    
	// if (BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_PORT_SEL) != PLAT_CFG_ULG_PORT_UART)
	// {
	// 	BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_PORT_SEL, PLAT_CFG_ULG_PORT_UART);
	// 	BSP_SavePlatConfigToRawFlash();
	// }

    luat_air153C_wtd_cfg_init(28);//Initialize the watchdog and set the watchdog feeding pin
    luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, "test");
    luat_mobile_set_rrc_auto_release_time(1);
    luat_pm_set_usb_power(0);
    luat_rtos_task_sleep(2000);
    luat_air153C_wtd_feed_wtd();//After feeding the dog, be careful not to feed the watchdog twice within 1 second, otherwise it will enter the test mode and reset the output to high level.
    
    /*Test without feeding the dog*/
    #if WTD_NOT_FEED_TEST
        int count = 0;
        LUAT_DEBUG_PRINT("[DIO] Feed WTD Test Start");
        luat_rtos_task_sleep(3000);
        while (1)
        {
            count++;
            luat_rtos_task_sleep(1000);
            LUAT_DEBUG_PRINT("[DIO]Timer count(1s):[%d]", count);
        }
    #endif

/*Test feeding the dog*/
#if WTD_FEED_TEST
    LUAT_DEBUG_PRINT("[DIO] Feed WTD Test Start");
    luat_rtos_task_sleep(3000);
    while (1)
    {
        luat_rtos_task_sleep(60000);
        luat_air153C_wtd_feed_wtd();
        LUAT_DEBUG_PRINT("[DIO]Eat Dog");
    }
#endif

/*Close watchdog feeding*/
#if WTD_CLOSE_FEED
    int flag = 0;
    LUAT_DEBUG_PRINT("[DIO] Close Feed WTD Test Start");
    luat_rtos_task_sleep(3000);
    while (1)
    {
        if (!flag)
        {
            flag = 1;
            luat_air153C_wtd_close();
        }
        flag++;
        LUAT_DEBUG_PRINT("[DIO]Timer count(1s):[%d]", flag);
        luat_rtos_task_sleep(1000);
    }
#endif


/*Turn off Feed the Dog and then turn it back on*/
#if CLOSE_FEED_AND_FEED
    int flag = 0;
    LUAT_DEBUG_PRINT("[DIO] Close And Open Feed WTD Test Start");
    luat_rtos_task_sleep(3000);
    while (1)
    {
        if (!flag)
        {
            flag = 1;
            LUAT_DEBUG_PRINT("[DIO] Close Feed WTD!");
            luat_air153C_wtd_close();
            luat_rtos_task_sleep(1000);//Convenient to observe the settings for a longer time
        }
        flag++;
        if (flag == 280){
            LUAT_DEBUG_PRINT("[DIO] Open Feed WTD!");
            luat_air153C_wtd_feed_wtd();
        }
        luat_rtos_task_sleep(1000);
        LUAT_DEBUG_PRINT("[DIO]Timer count(1s):[%d]", flag);
    }
#endif

/*Test mode reset
    Test mode: Feeding the watchdog 2 times within 1 second will reset the Modules and restart it.*/
#if TEST_MODE_RESET_TEST
    int count = 0;
    LUAT_DEBUG_PRINT("[DIO] Test Mode Test Start");
    luat_rtos_task_sleep(3000);
    while (1)
    {
        if (count == 15)
        {
            LUAT_DEBUG_PRINT("[DIO] Rrset Module");
            //It actually takes time to set the time. It takes 500ms to feed the watchdog each time, so you have to wait for the corresponding time in turn.
            luat_air153C_wtd_set_timeout(8);
            /*Examples of waiting times include:
                time=8/4*500*/
            luat_rtos_task_sleep(1100);
            count = 0;
        }
        count++;
        luat_rtos_task_sleep(1000);
        LUAT_DEBUG_PRINT("[DIO]Timer count(1s):[%d]", count);
    }
#endif

    luat_rtos_task_delete(feed_wdt_task_handle);
    
}

static void task_feed_wdt_init(void)
{
    luat_rtos_task_create(&feed_wdt_task_handle, 2048, 50, "feed_wdt", task_feed_wdt_run, NULL, 15);
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

INIT_TASK_EXPORT(task_feed_wdt_init,"0");
// This task simulates software exceptions by executing an infinite loop. The watchdog will automatically restart after 20 seconds when the watchdog times out.
// Only used for simulation testing, you can open it yourself if necessary
// INIT_TASK_EXPORT(task_dead_loop_init,"2");



