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

#ifndef LUAT_PM_H
#define LUAT_PM_H

#include "luat_base.h"
/**
 * @defgroup luatos_pm power management interface
 * @{*/

/* ------------------------------------------------ sleep begin----------------------------------------------- */
#define LUAT_PM_SLEEP_MODE_NONE 0	 ///< The system is in an active state and has not luat any steps to reduce power consumption.
#define LUAT_PM_SLEEP_MODE_IDLE 1	 ///< Idle mode, this mode stops the CPU and some clocks when the system is idle, and can be woken up by any event or interrupt
#define LUAT_PM_SLEEP_MODE_LIGHT 2	 ///<Light sleep mode, CPU is stopped, most clocks and peripherals are stopped
#define LUAT_PM_SLEEP_MODE_DEEP 3	 ///< Deep sleep mode, the CPU stops, only a few low-power peripherals work, and can be woken up by special interrupts
#define LUAT_PM_SLEEP_MODE_STANDBY 4 ///< Standby mode, CPU stopped, device context lost (can be saved to special peripherals), usually reset after waking up
//#define LUAT_PM_SLEEP_MODE_SHUTDOWN 5 ///<Shutdown mode, lower power consumption than Standby mode, context is usually unrecoverable, reset after wake-up
#define LUAT_PM_POWER_MODE_NORMAL (0)	///< Remove all measures to reduce power consumption
#define LUAT_PM_POWER_MODE_HIGH_PERFORMANCE (1)	///< It is possible to maintain performance and wake up remotely from sleep state, taking into account low power consumption, use LUAT_PM_SLEEP_MODE_LIGHT
#define LUAT_PM_POWER_MODE_BALANCED (2) ///< Performance and power consumption balance, use LUAT_PM_SLEEP_MODE_LIGHT
#define LUAT_PM_POWER_MODE_POWER_SAVER (3) ///< PSM+ ultra-low power consumption, use LUAT_PM_SLEEP_MODE_STANDBY to enter PSM+ mode

/**
 * @brief Set the deepest sleep mode flag
 *
 * @param mode deepest sleep mode
 * @param vote_tag dormant tag
 * @return int =0 success, others failure*/
int luat_pm_set_sleep_mode(int mode, const char *vote_tag);

/**
 * @brief Get the sleep mode corresponding to the sleep mark
 *
 * @param vote_tag dormant tag
 * @return int = -1 fails, others succeed*/
int luat_pm_get_sleep_mode(const char *vote_tag);

typedef void (*luat_pm_sleep_callback_t)(int mode);
/**
 * @brief Register the callback function before sleeping
 *
 * @param callback_fun user callback before sleeping
 * @return int =0 success, others failure*/
int luat_pm_sleep_register_pre_handler(luat_pm_sleep_callback_t callback_fun);

/**
 * @brief Unregister the callback function before sleep
 *
 * @return int =0 success, others failure*/
int luat_pm_sleep_deregister_pre_handler(void);

/**
 * @brief Register the callback function after waking up
 *
 * @param callback_fun user callback after waking up
 * @return int =0 success, others failure*/
int luat_pm_sleep_register_post_handler(luat_pm_sleep_callback_t callback_fun);


/**
 * @brief Unregister the callback function after waking up
 *
 * @param callback_fun user callback after waking up
 * @return int =0 success, others failure*/
int luat_pm_sleep_deregister_post_handler(void);
/*------------------------------------------------ sleep   end----------------------------------------------- */


/* ----------------------------------------------- wkaeup begin---------------------------------------------- */
/**
 * @brief wakeupPad
 */
typedef enum LUAT_PM_WAKEUP_PAD
{
	LUAT_PM_WAKEUP_PAD_0 = 0, 	/**<wakeupid_0*/
	LUAT_PM_WAKEUP_PAD_1, 		/**<wakeupid_1*/
	LUAT_PM_WAKEUP_PAD_2,		/**<wakeupid_2*/
	LUAT_PM_WAKEUP_PAD_3,		/**<wakeupid_3*/
	LUAT_PM_WAKEUP_PAD_4,		/**<wakeupid_4*/
	LUAT_PM_WAKEUP_PAD_5,		/**<wakeupid_5*/
	LUAT_PM_WAKEUP_LPUART,		/**<LPUART wake up*/
	LUAT_PM_WAKEUP_LPUSB,		/**<LPUSB wake up*/
	LUAT_PM_WAKEUP_PWRKEY,		/**<PWRKEY wake up*/
	LUAT_PM_WAKEUP_CHARGE,		/**<CHARGE wake up*/
	LUAT_PM_WAKEUP_PAD_MAX
}LUAT_PM_WAKEUP_PAD_E;
/**
 * @brief wakeupPad configuration parameters*/
typedef struct luat_pm_wakeup_pad_cfg
{
	uint8_t pos_edge_enable;	/**Enable rising edge interrupt*/
    uint8_t neg_edge_enable;	/**Enable falling edge interrupt*/
	uint8_t pull_up_enable;		/**Configuration pull-up*/
    uint8_t pull_down_enable;	/**Configuration drop-down*/
}luat_pm_wakeup_pad_cfg_t;
/**
 * @brief defines wakeupPad interrupt callback function type*/
typedef void (*luat_pm_wakeup_pad_isr_callback_t)(LUAT_PM_WAKEUP_PAD_E num);

/**
 * @brief Set wakeupPad interrupt callback function
 *
 * @param callback_fun wakeupPad interrupt callback function
 * @return int =0 success, others failure*/
int luat_pm_wakeup_pad_set_callback(luat_pm_wakeup_pad_isr_callback_t callback_fun);

/**
 * @brief Configuring the wakeupPad interrupt parameters conflicts with the GPIO20-22 input configuration and cannot be used simultaneously with the GPIO API.
 *
 * @param enable interrupt enable
 * @param source_id wakeupPad
 * @param cfg wakeupPad configuration parameters
 * @return int =0 success, others failure*/
int luat_pm_wakeup_pad_set(uint8_t enable, LUAT_PM_WAKEUP_PAD_E source_id, luat_pm_wakeup_pad_cfg_t *cfg);

/**
 * @brief Get the wakeupPad pin level
 *
 * @param source_id wakeupPad
 * @return int =-1 failed, 0 is low level, 1 is high level*/
int luat_pm_wakeup_pad_get_value(LUAT_PM_WAKEUP_PAD_E source_id);

/**
 * @brief powerkey setting mode*/
typedef enum LUAT_PM_POWERKEY_MODE
{
	LUAT_PM_PWRKEY_PWRON_MODE = 0,				/**default*/
    LUAT_PM_PWRKEY_WAKEUP_LOWACTIVE_MODE,		/**Low level press*/
    LUAT_PM_PWRKEY_WAKEUP_HIGHACTIVE_MODE,		/**High level press*/
    LUAT_PM_PWRKEY_UNKNOW_MODE,
}LUAT_PM_POWERKEY_MODE_E;

/**
 * @brief powerkey status*/
typedef enum LUAT_PM_POWERKEY_STATE
{
    LUAT_PM_PWRKEY_RELEASE = 0,					/**release*/
    LUAT_PM_PWRKEY_PRESS,						/**Press*/
    LUAT_PM_PWRKEY_LONGPRESS,					/**Long press*/
    LUAT_PM_PWRKEY_REPEAT,						/**Repeat activation*/
}LUAT_PM_POWERKEY_STATE_E;

/**
 * @brief powerkey configuration parameters*/
typedef struct
{
    int16_t long_press_timeout;					/**Long press timeout*/
    int16_t repeat_timeout;						/**Repeat timeout*/
    int16_t pwroff_timeout;						/**Shutdown time, this value is meaningless*/
}luat_pm_pwrkey_cfg_t;

typedef void(* luat_pm_pwrkey_callback_t)(LUAT_PM_POWERKEY_STATE_E status);

/**
 * @brief configure powerkey button
 *
 * @param mode interrupt enable
 * @param pullUpEn wakeupPad
 * @param cfg powerkey configuration parameters
 * @param callback powerkey callback function
 * @return int =0 success, others failure*/
int luat_pm_set_pwrkey(LUAT_PM_POWERKEY_MODE_E mode, bool pullUpEn, luat_pm_pwrkey_cfg_t *cfg, luat_pm_pwrkey_callback_t callback);
/* ------------------------------------------------ wakeup end----------------------------------------------- */


/* ---------------------------------------- power on/off/reboot begin---------------------------------------- */
/**
 * @brief boot reason*/
typedef enum LUAT_PM_POWERON_REASON
{
	LUAT_PM_POWERON_REASON_NORMAL = 0,			/**<powerkey or power on*/
	LUAT_PM_POWERON_REASON_FOTA,				/**<Power on after charging or AT command download is completed*/
	LUAT_PM_POWERON_REASON_ALARM,				/**<Alarm clock on*/
	LUAT_PM_POWERON_REASON_SWRESET,				/**<Software restart*/
	LUAT_PM_POWERON_REASON_UNKNOWN,				/**<Unknown reason*/
	LUAT_PM_POWERON_REASON_HWRESET,				/**<RESET key to restart*/
	LUAT_PM_POWERON_REASON_EXCEPTION,			/**<Abnormal restart*/
	LUAT_PM_POWERON_REASON_TOOL,				/**<Tool controlled restart*/
	LUAT_PM_POWERON_REASON_WDT,					/**<Internal watchdog restart*/
	LUAT_PM_POWERON_REASON_EXTERNAL,			/**<External restart*/
	LUAT_PM_POWERON_REASON_CHARGING,			/**<Charging and turning on*/
} LUAT_PM_POWERON_REASON_E;
/**
 * @brief Get the boot reason
 * @param NULL
 * @return @see LUAT_PM_POWERON_REASON_E*/
int luat_pm_get_poweron_reason(void);
/**
 * @brief Set device shutdown*/
int luat_pm_poweroff(void);
/**
 * @brief device restart*/
int luat_pm_reboot(void);
/* ----------------------------------------- power on/off/reboot end----------------------------------------- */


/* --------------------------------------------- vbat/vbus begin--------------------------------------------- */
/**
 * @brief Get VBUS insertion status
 * @param status VBUS insertion status, inserted is 1, not inserted is 0*/
int luat_pm_get_vbus_status(uint8_t *status);



/* ---------------------------------------------- vbat/vbus end---------------------------------------------- */


/* ------------------------------------------------ timer begin----------------------------------------------- */\
/**
 * @brief Software timer ID of deep sleep mode*/
typedef enum LUAT_PM_DEEPSLEEP_TIMERID
{
	LUAT_PM_DEEPSLEEP_TIMER_ID0 = 0,		/**0 and 1, the maximum timing time is 2.5 hours, the accuracy is 10ms, there is no need to store information in flash,*/
	LUAT_PM_DEEPSLEEP_TIMER_ID1,
	LUAT_PM_DEEPSLEEP_TIMER_ID2,          /**2 to 6, the maximum timing time is 740 hours, the accuracy is 10ms, and information needs to be stored in flash. This type of timer should try to avoid repeated starting and stopping to prevent the flash life from being reduced. If the timing duration does not exceed 2.5 hours, it is recommended to use 0 and 1*/
	LUAT_PM_DEEPSLEEP_TIMER_ID3,
	LUAT_PM_DEEPSLEEP_TIMER_ID4,
	LUAT_PM_DEEPSLEEP_TIMER_ID5,
	LUAT_PM_DEEPSLEEP_TIMER_ID6,
}LUAT_PM_DEEPSLEEP_TIMERID_E;


/**
 * @brief Reasons for waking up from deep sleep mode*/
typedef enum LUAT_PM_WAKEUP_REASON
{
    LUAT_PM_WAKEUP_FROM_POR = 0,
    LUAT_PM_WAKEUP_FROM_RTC,
    LUAT_PM_WAKEUP_FROM_PAD,
    LUAT_PM_WAKEUP_FROM_LPUART,
    LUAT_PM_WAKEUP_FROM_LPUSB,
    LUAT_PM_WAKEUP_FROM_PWRKEY,
    LUAT_PM_WAKEUP_FROM_CHARG,
}LUAT_PM_WAKEUP_REASON_E;

/**
 * @brief defines the callback function type after the scheduled time expires*/
typedef LUAT_RT_RET_TYPE (*luat_pm_deep_sleep_mode_timer_callback_t)(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id);

/**
 * @brief Register the software timer timeout callback function in deep sleep mode
 *
 * @param timer_id timer ID
 * @param callback callback function
 * @return int =0 success, others failure*/
int luat_pm_deep_sleep_mode_register_timer_cb(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id, luat_pm_deep_sleep_mode_timer_callback_t callback);

/**
 * @brief Start the software timer in deep sleep mode
 *
 * @param timer_id timer ID
 * @param timeout timeout time, unit ms
 * @return int =0 success, others failure*/
int luat_pm_deep_sleep_mode_timer_start(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id, uint32_t timeout);

/**
 * @brief Stop software timer in deep sleep mode
 *
 * @param timer_id timer ID
 * @return int =0 success, others failure*/
int luat_pm_deep_sleep_mode_timer_stop(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id);

/**
 * @brief Check if the software timer in deep sleep mode is running
 *
 * @param timer_id timer ID
 * @return int =0 is not running, int =1 is running*/
int luat_pm_deep_sleep_mode_timer_is_running(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id);

/**
 * @brief Get the remaining timing time of the software timer in deep sleep mode
 *
 * @param timer_id timer ID
 * @return uint32_t =0xffffffff The timer is not running or the timer id does not exist, other remaining running time, unit ms*/
uint32_t luat_pm_get_deep_sleep_mode_timer_remain_time(LUAT_PM_DEEPSLEEP_TIMERID_E timer_id);

/**
 * @brief Get the wake-up reason
 *
 * @return LUAT_PM_WAKEUP_REASON*/
int luat_pm_get_wakeup_reason(void);
/*------------------------------------------------ timer   end----------------------------------------------- */

/**
 * @brief Manually control USB power
 * @attention After shutting down, plugging in USB will reactivate, causing the USB charger status to remain 1
 * @param onoff 0 off, others on
 * @return int =0 success, others failure*/
int luat_pm_set_usb_power(uint8_t onoff);
/**
 * @brief Manually control GPS power (only applicable to 780EG)
 * @param onoff 0 off, 1 on
 * @return int =0 success, others failure*/
int luat_pm_set_gnss_power(uint8_t onoff);
/**
 * @brief Set networking low power mode, equivalent to AT+POWERMODE
 * @param low power main mode see LUAT_PM_POWER_MODE_XXX
 * @param reserved, low-power secondary mode, when the main mode is set to LUAT_PM_POWER_MODE_BALANCED, the power consumption mode can be fine-tuned, currently unavailable
 * @return int =0 success, others failure
 * @note conflicts with luat_pm_set_sleep_mode and luat_pm_set_usb_power and cannot be used at the same time*/
int luat_pm_set_power_mode(uint8_t mode, uint8_t sub_mode);
/**
 * @brief io voltage adjustment
 * @param id reserved, currently not needed
 * @param val, voltage value, unit mV
 * @return int =0 success, others failure*/
int luat_pm_iovolt_ctrl(int id, int val);
/**
 * @brief Set powerkey anti-shake switch
 * @param onoff 1 means turning on powerkey anti-shake, 0 means turning off anti-shake
 * @return int =0 success, others failure
 * @note When anti-shake is turned on, long press the PowerKey to turn on the phone, and the reset button will turn off*/
int luat_pm_set_powerkey_mode(uint8_t onoff);
/**@}*/


#endif
