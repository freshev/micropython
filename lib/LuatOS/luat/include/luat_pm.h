
#ifndef LUAT_PM_H
#define LUAT_PM_H
#include "luat_base.h"
/**
 * @defgroup luatos_device_pm power management class (low power consumption)
 * @{*/
#define LUAT_PM_SLEEP_MODE_NONE     0	//The system is in an active state and has not luat any steps to reduce power consumption.
#define LUAT_PM_SLEEP_MODE_IDLE     1	//Idle mode, this mode stops the CPU and some clocks when the system is idle, and can be woken up by any event or interrupt
#define LUAT_PM_SLEEP_MODE_LIGHT    2	//Light sleep mode, CPU is stopped, most clocks and peripherals are stopped
#define LUAT_PM_SLEEP_MODE_DEEP     3	//Deep sleep mode, the CPU stops, only a few low-power peripherals work, and can be woken up by special interrupts
#define LUAT_PM_SLEEP_MODE_STANDBY	4	//Standby mode, CPU stops, device context is lost (can be saved to special peripherals), usually reset after waking up
//#define LUAT_PM_SLEEP_MODE_SHUTDOWN 5 //Shutdown mode, lower power consumption than Standby mode, context is usually unrecoverable, reset after wake-up
#define LUAT_PM_POWER_MODE_NORMAL (0)	///< Remove all measures to reduce power consumption
#define LUAT_PM_POWER_MODE_HIGH_PERFORMANCE (1)	///< To maintain performance as much as possible and take into account low power consumption, use LUAT_PM_SLEEP_MODE_LIGHT
#define LUAT_PM_POWER_MODE_BALANCED (2) ///< Performance and power consumption balance, use LUAT_PM_SLEEP_MODE_LIGHT
#define LUAT_PM_POWER_MODE_POWER_SAVER (3) ///< Ultra-low power consumption, use LUAT_PM_SLEEP_MODE_STANDBY to enter PSM mode


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

// switch class
enum
{
	LUAT_PM_POWER_USB,
	LUAT_PM_POWER_GPS,
	LUAT_PM_POWER_GPS_ANT,
	LUAT_PM_POWER_CAMERA,
	LUAT_PM_POWER_DAC_EN_PIN,
	LUAT_PM_POWER_POWERKEY_MODE,
	LUAT_PM_POWER_WORK_MODE,
	LUAT_PM_POWER_LDO_CTL_PIN,
};

// Level class
enum
{
	LUAT_PM_ALL_GPIO,
	LUAT_PM_LDO_TYPE_VMMC,
	LUAT_PM_LDO_TYPE_VLCD,
	LUAT_PM_LDO_TYPE_CAMA,
	LUAT_PM_LDO_TYPE_CAMD,
	LUAT_PM_LDO_TYPE_VLP33,//8910 no
	LUAT_PM_LDO_TYPE_VLP18,//8910 no
	LUAT_PM_LDO_TYPE_VIO18,
	LUAT_PM_LDO_TYPE_VIBR,	//8850 no
	LUAT_PM_LDO_TYPE_KEYLED,
	LUAT_PM_LDO_TYPE_VSIM1,	//It may not work, try not to use it
	LUAT_PM_LDO_TYPE_QTY,
};

/**
 * @brief Request to enter the specified sleep mode
 * @param mode sleep mode, see LUAT_PM_SLEEP_MODE_XXX
 * @return int =0 success, others failure*/
int luat_pm_request(int mode);
/**
 * @brief Exit sleep mode
 * @param mode sleep mode, see LUAT_PM_SLEEP_MODE_XXX
 * @return int =0 success, others failure*/
int luat_pm_release(int mode);

/**
 * @brief starts the underlying timer, which still takes effect in sleep mode. It is only triggered once
 * @param id timer id, usually 0-3
 * @param timeout timing duration, in milliseconds
 * @return int =0 success, others failure*/
int luat_pm_dtimer_start(int id, size_t timeout);

/**
 * @brief Stop the underlying timer
 * @param id timer id, usually 0-3
 * @return int =0 success, others failure*/
int luat_pm_dtimer_stop(int id);

/**
 * @brief Check the underlying timer running status
 * @param id timer id, usually 0-3
 * @return int =1 is running, 0 is not running*/
int luat_pm_dtimer_check(int id);

// void luat_pm_cb(int event, int arg, void* args);

/**
 * @brief The reason for wake-up is used to determine whether the boot is from sleep state.
 * @param lastState 0-Normal boot (power on/reset), 3-Deep sleep boot, 4-Hibernation boot
 * @param rtcOrPad 0-Power on/reset boot, 1-RTC boot, 2-WakeupIn/Pad/IO boot, 3-Wakeup/RTC boot
 * @return int =0 success, others failure*/
int luat_pm_last_state(int *lastState, int *rtcOrPad);

/**
 * @brief Force into the specified sleep mode, ignoring the influence of certain peripherals, such as USB
 * @param mode sleep mode, see LUAT_PM_SLEEP_MODE_XXX
 * @return int =0 success, others failure*/
int luat_pm_force(int mode);

/**
 * @brief Check hibernation status
 * @return int, see LUAT_PM_SLEEP_MODE_XXX*/
int luat_pm_check(void);
/**
 * @brief Get the remaining time of all deep sleep timers, in ms
 * @param count [OUT]The number of timers
 * @param list [OUT]Remaining time list
 * @return int =0 success, others failure*/
int luat_pm_dtimer_list(size_t* count, size_t* list);

/**
 * @brief Get wake-up timer id
 * @param id wake-up timing id
 * @return int =0 success, others failure*/
int luat_pm_dtimer_wakeup_id(int* id);

/**
 * @brief shutdown
 **/
int luat_pm_poweroff(void);

/**
 * @brief restart
 **/
int luat_pm_reset(void);

/**
 * @brief Turn on internal power control. Note that not all platforms support it. Some platforms may support some options, depending on the hardware.
 * @param id power control id, see LUAT_PM_POWER_XXX
 * @param val switch true/1 is on, false/0 is off, the default is off, some options support numerical values
 * @return int =0 success, others failure*/
int luat_pm_power_ctrl(int id, uint8_t val);

/**
 * @brief boot reason
 * @return int, see LUAT_PM_POWERON_REASON*/
int luat_pm_get_poweron_reason(void);

/**
 * @brief Set the level of IO voltage domain
 * @param id voltage domain ID, ignored by the mobile core platform
 * @param val expected level value, unit mv
 * @return int Returns 0 if successful, otherwise failed*/
int luat_pm_iovolt_ctrl(int id, int val);

/**
 * @brief Configure wake-up pin, only for esp series
 * @param pin pin
 * @param val level
 * @return*/
int luat_pm_wakeup_pin(int pin, int val);
/**
 * @brief Set networking low power mode, equivalent to AT+POWERMODE
 * @param low power main mode see LUAT_PM_POWER_MODE_XXX
 * @param reserved, low-power secondary mode, when the main mode is set to LUAT_PM_POWER_MODE_BALANCED, the power consumption mode can be fine-tuned, currently unavailable
 * @return int =0 success, others failure
 * @note conflicts with luat_pm_set_sleep_mode and luat_pm_set_usb_power and cannot be used at the same time*/
int luat_pm_set_power_mode(uint8_t mode, uint8_t sub_mode);
/**
 * @brief The remaining time of the deep sleep timer, unit ms
 * @param id timer ID
 * @return uint32_t 0xffffffff failed, others are the remaining time*/
uint32_t luat_pm_dtimer_remain(int id);
/** @}*/
#endif
