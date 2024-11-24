
#include "clock.h"
#include "slpman.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "luat_pm.h"
#include "luat_gpio.h"
#include "luat_rtos.h"
#include "common_api.h"
#include "luat_debug.h"
#include "luat_air153C_wtd.h"
#include "platform_define.h"

/**This example needs to be used with the official Hezhou Modules that supports external chip watchdogs.*/
luat_gpio_cfg_t irq_output_gpio_cfg;
luat_gpio_cfg_t feed_gpio_cfg;
luat_gpio_cfg_t reset_gpio_cfg;
luat_gpio_cfg_t mode_gpio_cfg;

static void* feed_timer;//Feed the watchdog output timer
static void* feed_count_timer;//Feed the watchdog counting timer

static int s_time_set = 0;
static int s_wtd_feed_pin = 28;//The default Modules is to feed the watchdog and discipline it.

static void wtd_feed_count_cb(uint32_t arg)
{
    luat_air153C_wtd_feed_wtd();
}

/*op 1: Enable watchdog feeding GPIO
        0: Turn off the watchdog feeding GPIO*/
static void luat_air153C_wtd_op(int op)
{
    if (op){
        luat_gpio_set_default_cfg(&feed_gpio_cfg);
        feed_gpio_cfg.pin = s_wtd_feed_pin;
        luat_gpio_open(&feed_gpio_cfg);
	    luat_gpio_set(s_wtd_feed_pin, 1);
    }else{
	    luat_gpio_set(s_wtd_feed_pin, 0);
        luat_gpio_close(s_wtd_feed_pin);
    }
}

//Feed the watchdog callback
static void feed_wtd_cb(uint32_t arg)
{
    luat_air153C_wtd_op(0);
    LUAT_DEBUG_PRINT("Feed Over");
    if (s_time_set)
        s_time_set--;
    LUAT_DEBUG_PRINT("[DIO]s_time_set [%d]", s_time_set);
    if (s_time_set)
    {
        luat_start_rtos_timer(feed_count_timer, 100, 0);
    }
}

//Feed the dog
int luat_air153C_wtd_feed_wtd(void)
{
    luat_air153C_wtd_op(1);
    luat_start_rtos_timer(feed_timer, 400, 0);
    return 0;
}

//Close watchdog feeding
int luat_air153C_wtd_close(void)
{
    luat_air153C_wtd_op(1);
    luat_start_rtos_timer(feed_timer, 700, 0);
    return 0;
}


//Set the number of times the watchdog feeds the dog
int luat_air153C_wtd_set_timeout(size_t timeout)
{
    s_time_set = 0;
    if ((timeout % 4 != 0) || !timeout || timeout > 24)
        return 1;

    s_time_set = timeout / 4;
    luat_air153C_wtd_feed_wtd();
    return 0;
}


int luat_air153C_wtd_setup(void)
{
    return luat_air153C_wtd_feed_wtd();
}

/*Default Modules GPIO pin configuration initialization*/
void luat_air153C_wtd_cfg_init(int wtd_feed_pin)
{
    slpManAONIOPowerOn();
    s_wtd_feed_pin = wtd_feed_pin;
    feed_timer = luat_create_rtos_timer(feed_wtd_cb, NULL, NULL);
    feed_count_timer = luat_create_rtos_timer(wtd_feed_count_cb, NULL, NULL);
}
