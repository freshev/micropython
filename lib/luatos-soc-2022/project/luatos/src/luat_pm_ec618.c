#include "luat_base.h"
#include "luat_pm.h"
#include "luat_msgbus.h"
#include "luat_wdt.h"

#include "bsp.h"
#include "bsp_custom.h"
#include "osasys.h"
#include "ostask.h"
#include "slpman.h"
#include "reset.h"
//#include "psproxytask.h"
#include "driver_gpio.h"
#include "common_api.h"
#include "plat_config.h"
#include "ps_event_callback.h"
#include "cmips.h"
#include "ps_lib_api.h"

#define LUAT_LOG_TAG "pm"
#include "luat_log.h"

static uint8_t lastRequestMode = SLP_IDLE_STATE; //Set when APP starts
static uint8_t wakeupSrc = 0;
static uint8_t firstSlpstate;
static uint8_t wakeup_deeptimer_id = 0xFF;

static const char slpStateText[5][5]={{"Actv"},{"Idle"},{"Slp1"},{"Slp2"},{"Hibn"}};
static const char wakeupSrcStr[3][4] = {{"POR"}, {"RTC"}, {"IO"}};
extern void soc_set_usb_sleep(uint8_t onoff);
uint32_t inParam = 0xAABBCCDD;
extern int luat_dtimer_cb(lua_State *L, void* ptr);

static void appTimerExpFunc(uint8_t id) {
    wakeup_deeptimer_id = id;
    LLOGI("DeepTimer Wakeup by id=%d", id);
    if (luat_msgbus_is_ready()) {
        rtos_msg_t msg = {0};
        msg.handler = luat_dtimer_cb;
        msg.arg1 = wakeup_deeptimer_id;
        luat_msgbus_put(&msg, 0);
    }
}

static slpManSlpState_t luat_user_slp_state(void)
{
	return lastRequestMode;
}

int luat_pm_request(int mode) {
    if (mode < 0 || mode > LUAT_PM_SLEEP_MODE_STANDBY) {
        LLOGW("bad mode=%ld", mode);
        return -2;
    }
    if (lastRequestMode < 0 || lastRequestMode > LUAT_PM_SLEEP_MODE_STANDBY)
        lastRequestMode = 0;
    LLOGI("request mode=%s, prev=%s", slpStateText[mode], slpStateText[lastRequestMode]);
    lastRequestMode = mode;
    soc_set_usb_sleep(0);
    return 0;
}

int luat_pm_release(int mode) {
	soc_set_usb_sleep(0);
	lastRequestMode = LUAT_PM_SLEEP_MODE_IDLE;
    return 0;
}

int luat_pm_dtimer_start(int id, size_t timeout) {
    if (id < 0 || id > DEEPSLP_TIMER_ID6) {
        return -1;
    }
    slpManDeepSlpTimerStart(id, timeout);
    return 0;
}

int luat_pm_dtimer_stop(int id) {
    if (id < 0 || id > DEEPSLP_TIMER_ID6) {
        return -1;
    }
    slpManDeepSlpTimerDel(id);
    return 0;
}

int luat_pm_dtimer_check(int id) {
    if (id < 0 || id > DEEPSLP_TIMER_ID6) {
        return false;
    }
    LLOGD("dtimer check id %d, remain %d ms", id, slpManDeepSlpTimerRemainMs(id));
    if (slpManDeepSlpTimerRemainMs(id) <= 500)
    {
    	slpManDeepSlpTimerDel(id);
    }
    return slpManDeepSlpTimerIsRunning(id);
}

uint32_t luat_pm_dtimer_remain(int id){
    if (id < 0 || id > DEEPSLP_TIMER_ID6) {
        return -1;
    }
	return slpManDeepSlpTimerRemainMs(id);
}

int luat_pm_last_state(int *lastState, int *rtcOrPad) {
    *lastState = firstSlpstate;
    *rtcOrPad = wakeupSrc;
    return 0;
}

int luat_pm_force(int mode) {
    if (mode < 0 || mode > LUAT_PM_SLEEP_MODE_STANDBY) {
        LLOGW("bad mode=%ld", mode);
        return -2;
    }
    LLOGI("force request mode=%ld, prev mode=%ld", mode, lastRequestMode);
	lastRequestMode = mode;
	soc_set_usb_sleep(1);
    return 0;
}

int luat_pm_check(void) {
    
    return lastRequestMode;
}

int luat_pm_dtimer_list(size_t* c, size_t* dlist) {
    for (uint8_t i = 0; i<= DEEPSLP_TIMER_ID6; i++) {
        if (slpManDeepSlpTimerIsRunning(i)) {
            uint32_t retime = slpManDeepSlpTimerRemainMs(i);
            if (retime != 0xffffffff) {
                *(dlist+i) = retime;
            }
        }
    }
    return 0;
}

int luat_pm_dtimer_wakeup_id(int* id) {
    if (wakeup_deeptimer_id != 0xFF) {
        *id = wakeup_deeptimer_id;
        return 0;
    }
    return -1;
}

//---------------------------------------------------------------
void luat_pm_preinit(void)
{
	for(uint8_t i = 0; i <= DEEPSLP_TIMER_ID6; i++)
	{
	    slpManDeepSlpTimerRegisterExpCb(i, appTimerExpFunc);
	}
}

void luat_pm_init(void) {
	//LLOGI("pm mode %d", apmuGetDeepestSleepMode());
    if (BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_PWRKEY_MODE) != 0) {
        LLOGD("PowerKey-Debounce is enabled");
        // The power button anti-shake is on. If you want to disable it, you can call pm.power(pm.PWR_MODE, false)
    }
    apmuSetDeepestSleepMode(AP_STATE_HIBERNATE);
    soc_set_usb_sleep(0);
    slpManSlpState_t slpstate = slpManGetLastSlpState();
    slpManWakeSrc_e src = slpManGetWakeupSrc();
    wakeupSrc = (uint8_t)src;
    if (src > WAKEUP_FROM_PAD)
    {
    	src = WAKEUP_FROM_PAD;
    }
    if (slpstate == SLP_SLP2_STATE) {
        LLOGI("poweron: Wakup Sleep2 by %s %d", wakeupSrcStr[src], wakeup_deeptimer_id);
        firstSlpstate = LUAT_PM_SLEEP_MODE_DEEP;
    }
    else if (slpstate == SLP_HIB_STATE) {
        LLOGI("poweron: Wakup Hib by %s %d", wakeupSrcStr[src], wakeup_deeptimer_id);
        firstSlpstate = LUAT_PM_SLEEP_MODE_STANDBY;
    }
    else {
        firstSlpstate = LUAT_PM_SLEEP_MODE_NONE;
        LLOGI("poweron: Power/Reset");
    }
    slpManRegisterUsrSlpDepthCb(luat_user_slp_state);
    if (wakeup_deeptimer_id != 0xff)
    {
    	luat_msgbus_init();
        rtos_msg_t msg = {0};
        msg.handler = luat_dtimer_cb;
        msg.arg1 = wakeup_deeptimer_id;
        luat_msgbus_put(&msg, 0);
    }
}

int luat_pm_get_poweron_reason(void)
{
    LastResetState_e apRstState,cpRstState;
	ResetStateGet(&apRstState, &cpRstState);
	DBG("ap %d,cp %d", apRstState, cpRstState);
	int id = 0;

	switch(cpRstState)
	{
	case LAST_RESET_HARDFAULT:
	case LAST_RESET_ASSERT:
		return LUAT_PM_POWERON_REASON_EXCEPTION;
		break;
	case LAST_RESET_WDTSW:
	case LAST_RESET_WDTHW:
	case LAST_RESET_LOCKUP:
	case LAST_RESET_AONWDT:
		return LUAT_PM_POWERON_REASON_WDT;
		break;
	}

	switch(apRstState)
	{
	case LAST_RESET_POR:
	case LAST_RESET_NORMAL:
		id = LUAT_PM_POWERON_REASON_NORMAL;
		break;
	case LAST_RESET_SWRESET:
		id = LUAT_PM_POWERON_REASON_SWRESET;
		break;
	case LAST_RESET_HARDFAULT:
	case LAST_RESET_ASSERT:
		id = LUAT_PM_POWERON_REASON_EXCEPTION;
		break;
	case LAST_RESET_WDTSW:
	case LAST_RESET_WDTHW:
	case LAST_RESET_LOCKUP:
	case LAST_RESET_AONWDT:
		id = LUAT_PM_POWERON_REASON_WDT;
		break;
	case LAST_RESET_BATLOW:
	case LAST_RESET_TEMPHI:
		id = LUAT_PM_POWERON_REASON_EXTERNAL;
		break;
	case LAST_RESET_FOTA:
		id = LUAT_PM_POWERON_REASON_FOTA;
		break;
	case LAST_RESET_CPRESET:
		id = 100 + cpRstState;
		break;
	default:
		id = 200 + cpRstState;
		break;
	}
	return id;
}
///---------------------------------------

extern void pwrKeyStartPowerOff(void);
int luat_pm_poweroff(void)
{
	luat_wdt_close();
	slpManStartPowerOff();
    return 0;
}

int luat_pm_power_ctrl(int id, uint8_t onoff)
{
	switch(id)
	{
	case LUAT_PM_POWER_USB:
		soc_set_usb_sleep(!onoff);
		soc_usb_onoff(onoff);
		break;
	case LUAT_PM_POWER_GPS:
	case LUAT_PM_POWER_GPS_ANT:
	case LUAT_PM_POWER_CAMERA:
		GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_13, 4), 4, 0, 0);
		GPIO_Config(HAL_GPIO_13, 0, onoff);
		break;
	case LUAT_PM_POWER_DAC_EN_PIN:
	case LUAT_PM_POWER_LDO_CTL_PIN:
		GPIO_IomuxEC618(GPIO_ToPadEC618(HAL_GPIO_12, 4), 4, 0, 0);
		GPIO_Config(HAL_GPIO_12, 0, onoff);
		break;
	case LUAT_PM_POWER_POWERKEY_MODE:
		if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_PWRKEY_MODE) != onoff)
		{
			LLOGD("powerkey mode %d to %d", BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_PWRKEY_MODE), onoff);
			BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_PWRKEY_MODE, onoff);
			BSP_SavePlatConfigToRawFlash();
		}
		break;
	case LUAT_PM_POWER_WORK_MODE:
		switch(onoff)
		{
		case LUAT_PM_POWER_MODE_NORMAL:
			lastRequestMode = LUAT_PM_SLEEP_MODE_IDLE;
			break;
		case LUAT_PM_POWER_MODE_POWER_SAVER:
			lastRequestMode = LUAT_PM_SLEEP_MODE_STANDBY;
			break;
		default:
			lastRequestMode = LUAT_PM_SLEEP_MODE_LIGHT;
			break;
		}
		return luat_pm_set_power_mode(onoff, 0);
		break;
	default:
		return -1;
	}
	return 0;
}

int luat_pm_iovolt_ctrl(int id, int val) {
	IOVoltageSel_t set;
	if (val > 3400)
	{
		set = IOVOLT_3_40V;
	}
	else if (val >= 2650)
	{
		set = (val - 2650)/50 + IOVOLT_2_65V;
	}
	else if (val > 2000)
	{
		LLOGW("iovolt: out of range %d %d", id, val);
		return -1;
	}
	else if (val >= 1650)
	{
		set = (val - 1650)/50;
	}
	else
	{
		set = IOVOLT_1_65V;
	}
	slpManNormalIOVoltSet(set);
	slpManAONIOVoltSet(set);
	return 0;

}

int luat_pm_wakeup_pin(int pin, int val){
    LLOGW("not support yet");
    return -1;
}

int luat_pm_set_power_mode(uint8_t mode, uint8_t sub_mode)
{
	return soc_power_mode(mode, sub_mode);
}
