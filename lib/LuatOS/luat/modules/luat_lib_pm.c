
/*@Modules pm
@summary Power Management
@version 1.0
@date 2020.07.02
@demopm
@tagLUAT_USE_PM
@usage
--[[
Introduction to sleep mode

-- IDLE normal operating mode
-- LIGHT light sleep mode:
        CPU pause
        RAM remains powered
        Timers/network events/IO interrupts can automatically wake up
        The program continues to run after waking up
        GPIO hold level
-- DEEP deep sleep mode
        CPU pause
        Core RAM loses power, retaining RAM to maintain power supply
        Ordinary GPIO is powered off, peripheral driver is powered off
        AON_GPIO maintains the level before sleep
        dtimer timer can wake up
        wakeup foot can wake up
        After waking up, the program runs from the beginning, and all runtime data before hibernation is lost.
-- HIB sleep mode
        CPU pause
        If the RAM is powered off, the reserved RAM will also be powered off.
        Ordinary GPIO is powered off, peripheral driver is powered off
        AON_GPIO maintains the level before sleep
        dtimer timer can wake up
        wakeup foot can wake up
        After waking up, the program runs from the beginning, and all runtime data before hibernation is lost.

For some Moduless, such as Air780E, DEEP/HIB has no difference in user code.

Except for pm.shutdown(), RTC is always running unless the power is lost.
]]

--Timer wake-up, please use pm.dtimerStart()
-- wakeup wake up
    -- For example, Air101/Air103 has an independent wakeup pin, which does not require configuration and can directly control wakeup.
    -- For example, Air780E series has multiple wakeups available. Configure virtual GPIO through gpio.setup(32) for wakeup configuration.

pm.request(pm.IDLE) -- Request to enter different sleep modes by switching different values
-- Corresponding to the Air780E series, it does not necessarily enter sleep mode immediately after execution. If there is no need for subsequent data transmission, you can enter flight mode first and then quickly sleep.*/
#include "lua.h"
#include "lauxlib.h"
#include "luat_base.h"
#include "luat_pm.h"
#include "luat_msgbus.h"

#define LUAT_LOG_TAG "pm"
#include "luat_log.h"

// static int lua_event_cb = 0;
/*@sys_pub pm
deep sleep timer定时时间到回调
DTIMER_WAKEUP
@usage
sys.subscribe("DTIMER_WAKEUP", function(timer_id)
    log.info("deep sleep timer", timer_id)
end)*/
int luat_dtimer_cb(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "DTIMER_WAKEUP");
        lua_pushinteger(L, msg->arg1);
        lua_call(L, 2, 0);
    }
    return 0;
}

/**
Request to enter the specified sleep mode
@api pm.request(mode)
@int Sleep mode, such as pm.IDLE/LIGHT/DEEP/HIB.
@return boolean processing result, even if the return is successful, it may not be entered, nor will it be entered immediately
@usage
-- Request to enter sleep mode
--[[
IDLE runs normally, that is, no sleep
LIGHT light sleep, CPU stop, RAM retention, peripheral retention, interrupt wake-up possible. Some models support continued operation from hibernation
DEEP deep sleep, CPU stops, RAM is powered off, only special pins maintain the level before sleep, most pins cannot wake up the device.
HIB completely sleeps, the CPU is stopped, the RAM is powered off, and only the reset/special wake-up pin can wake up the device.
]]

pm.request(pm.HIB)*/
static int l_pm_request(lua_State *L) {
    int mode = luaL_checkinteger(L, 1);
    if (luat_pm_request(mode) == 0)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);
    return 1;
}

// static int l_pm_release(lua_State *L) {
//     int mode = luaL_checkinteger(L, 1);
//     if (luat_pm_release(mode) == 0)
//         lua_pushboolean(L, 1);
//     else
//         lua_pushboolean(L, 0);
//     return 1;
// }

/**
Start the underlying timer, which still takes effect in sleep mode. It is only triggered once and is invalid in shutdown state.
@api pm.dtimerStart(id, timeout)
@int timer id, usually 0-5
@int Timing duration, in milliseconds
@return boolean processing result
@usage
--Add underlying timer
pm.dtimerStart(0, 300 * 1000) -- wake up after 5 minutes
-- Moving core CAT1 platform series (Air780E/Air700E/Air780EP, etc.)
-- id = 0 or id = 1 Yes, the maximum sleep time is 2.5 hours
-- id >= 2, the maximum sleep time is 740 hours*/
static int l_pm_dtimer_start(lua_State *L) {
    int dtimer_id = luaL_checkinteger(L, 1);
    int timeout = luaL_checkinteger(L, 2);
    if (luat_pm_dtimer_start(dtimer_id, timeout)) {
        lua_pushboolean(L, 0);
    }
    else {
        lua_pushboolean(L, 1);
    }
    return 1;
}

/**
Turn off the underlying timer
@api pm.dtimerStop(id)
@int timer id
@usage
-- Close the underlying timer
pm.dtimerStop(0) -- Close the underlying timer with id=0*/
static int l_pm_dtimer_stop(lua_State *L) {
    int dtimer_id = luaL_checkinteger(L, 1);
    luat_pm_dtimer_stop(dtimer_id);
    return 0;
}

#ifndef LUAT_COMPILER_NOWEAK
LUAT_WEAK uint32_t luat_pm_dtimer_remain(int id){
	return -1;
}
#endif

/**
Check if the underlying timer is running
@api pm.dtimerCheck(id)
@int timer id
@return boolean processing result, true is still running, false is not running
@return number If running, the remaining running time in milliseconds (requires bsp support)
@usage
-- Check whether the underlying timer is running
pm.dtimerCheck(0) -- Check the underlying timer with id=0*/
static int l_pm_dtimer_check(lua_State *L) {
    int dtimer_id = luaL_checkinteger(L, 1);
    if (luat_pm_dtimer_check(dtimer_id)){
        uint32_t remain = luat_pm_dtimer_remain(dtimer_id);
    	lua_pushboolean(L, 1);
        lua_pushinteger(L, remain);
        return 2;
    }else{
    	lua_pushboolean(L, 0);
        return 1;
    }
}

static int l_pm_dtimer_list(lua_State *L) {
    size_t c = 0;
    size_t dlist[24];

    luat_pm_dtimer_list(&c, dlist);

    lua_createtable(L, c, 0);
    for (size_t i = 0; i < c; i++)
    {
        if (dlist[i] > 0) {
            lua_pushinteger(L, dlist[i]);
            lua_seti(L, -3, i+1);
        }
    }

    return 1;
}

/**
Check which timer the scheduled wake-up is from, if it is not scheduled to wake up, return -1
@api dtimerWkId()
@return int The processing result >=0 is the timer ID of this timed wake-up. Other errors indicate that it is not timed to wake up.
@usage
local timer_id = pm.dtimerWkId()*/
static int l_pm_dtimer_wakeup_id(lua_State *L) {
    int dtimer_id = 0xFF;

    luat_pm_dtimer_wakeup_id(&dtimer_id);

    if (dtimer_id != 0xFF) {
        lua_pushinteger(L, dtimer_id);
    }
    else {
        lua_pushinteger(L, -1);
    }
    return 1;
}

// static int l_pm_on(lua_State *L) {
//     if (lua_isfunction(L, 1)) {
//         if (lua_event_cb != 0) {
//             luaL_unref(L, LUA_REGISTRYINDEX, lua_event_cb);
//         }
//         lua_event_cb = luaL_ref(L, LUA_REGISTRYINDEX);
//     }
//     else if (lua_event_cb != 0) {
//         luaL_unref(L, LUA_REGISTRYINDEX, lua_event_cb);
//     }
//     return 0;
// }

/**
Boot reason, used to determine whether to boot from the hibernation Modules or power/reset
@api pm.lastReson()
@return int 0-Power on/reset boot, 1-RTC boot, 2-WakeupIn/Pad/IO boot, 3-Unknown reason (Wakeup/RTC is possible). Currently, only air101 and air103 will have this return value
@return int 0-Normal boot (power on/reset), 3-Deep sleep boot, 4-Hibernation boot
@return int Detailed reason for reset and boot: 0-powerkey or power on 1-charging or boot after AT command download is completed 2-alarm clock boot 3-software restart 4-unknown reason 5-RESET key 6-abnormal restart 7-tool control restart 8-Internal watchdog restart 9-External restart 10-Charging and booting
@usage
--Which method is used to boot the computer?
log.info("pm", "last power reson", pm.lastReson())*/
static int l_pm_last_reson(lua_State *L) {
    int lastState = 0;
    int rtcOrPad = 0;
    luat_pm_last_state(&lastState, &rtcOrPad);
    lua_pushinteger(L, rtcOrPad);
    lua_pushinteger(L, lastState);
    lua_pushinteger(L, luat_pm_get_poweron_reason());
    return 3;
}

/**
Force into the specified sleep mode, ignoring the impact of certain peripherals, such as USB
@api pm.force(mode)
@int sleep mode
@return boolean processing result. If the return is successful, there is a high probability that it will enter the sleep mode immediately.
@usage
-- Request to enter sleep mode
pm.force(pm.HIB)
-- For Yixin CAT1 platform series (Air780E/Air700E/Air780EP, etc.), this operation will turn off USB communication
-- If you need to turn on USB after waking up, please turn on USB voltage
--pm.power(pm.USB, true)*/
static int l_pm_force(lua_State *L) {
    lua_pushinteger(L, luat_pm_force(luaL_checkinteger(L, 1)));
    return 1;
}

/**
Check hibernation status
@apipm.check()
@return boolean processing result, if it can enter sleep successfully, return true, otherwise return false
@return int The bottom return value, 0 represents the ability to enter the lowest sleep level, other values   represent the lowest sleep level
@usage
--Request to enter sleep mode, and then check whether it can actually sleep
pm.request(pm.HIB)
if pm.check() then
    log.info("pm", "it is ok to hib")
else
    -- For Yixin CAT1 platform series (Air780E/Air700E/Air780EP, etc.), this operation will turn off USB communication
    pm.force(pm.HIB) -- force sleep
    -- If you need to turn on USB after waking up, please turn on USB voltage
    --sys.wait(100)
    --pm.power(pm.USB, true)
end*/
static int l_pm_check(lua_State *L) {
    int ret = luat_pm_check();
    lua_pushboolean(L, luat_pm_check() == 0 ? 1 : 0);
    lua_pushinteger(L, ret);
    return 2;
}


#ifndef LUAT_COMPILER_NOWEAK
LUAT_WEAK int luat_pm_poweroff(void) {
    LLOGW("powerOff is not supported");
    return -1;
}
#else
extern int luat_pm_poweroff(void);
#endif

/**
Shut down
@apipm.shutdown()
@return nil no return value
@usage
-- Currently supports the core CAT1 platform series (Air780E/Air700E/Air780EP, etc.)
-- Requires firmware compiled after 2022-12-22
pm.shutdown()*/
static int l_pm_power_off(lua_State *L) {
    (void)L;
    luat_pm_poweroff();
    return 0;
}

/**
Restart
@apipm.reboot()
@return nil no return value
-- Restart the device immediately. The behavior of this function is exactly the same as rtos.reboot(), but it is aliased in the pm library.
pm.reboot()*/
int l_rtos_reboot(lua_State *L);
int l_rtos_standby(lua_State *L);

/**
Turn on the internal power control. Note that not all platforms support it. Some platforms may support some options, depending on the hardware.
@api pm.power(id, onoff)
@int power control id, pm.USB pm.GPS etc.
@boolean or int switch true/1 is on, false/0 is off, the default is off, some options support numerical values
@return boolean The processing result is true if it is successful, if it is false it fails.
@usage
-- Turn off the USB power, otherwise turn it on and it will send true
pm.power(pm.USB, false)

-- Air780EG, power on the built-in GPS chip. Note that the GPS and GPS_ANT of Air780EG are controlled together, so they are merged.
pm.power(pm.GPS, true)

-- Enable pwrkey anti-shake at boot for Yixin CAT1 platform series (Air780E/Air700E/Air780EP, etc.)
-- Note: After turning it on, the reset button will turn off!!! You need to press and hold pwrkey for 2 seconds to turn it on.
-- pm.power(pm.PWK_MODE, true)

-- Move core CAT1 platform series (Air780E/Air700E/Air780EP, etc.) PSM+ low power consumption setting
--Yixin CAT1 platform series (Air780E/Air700E/Air780EP, etc.) energy-saving mode, 0~3, 0 completely off, 1 performance priority, 2 balance, 3 extreme power consumption
-- For details visit: https://airpsm.cn
-- pm.power(pm.WORK_MODE, 1)*/
static int l_pm_power_ctrl(lua_State *L) {
	uint8_t onoff = 0;
    int id = luaL_checkinteger(L, 1);
    if (lua_isboolean(L, 2)) {
    	onoff = lua_toboolean(L, 2);
    }
    else
    {
    	onoff = lua_tointeger(L, 2);
    }
    lua_pushboolean(L, !luat_pm_power_ctrl(id, onoff));
    return 1;
}

/**
IO high level voltage control
@api pm.ioVol(id, val)
@int level id, currently only pm.IOVOL_ALL_GPIO
@int level value, unit millivolt
@return boolean The processing result is true if it is successful, if it is false it fails.
@usage
-- Set IO level for Yixin CAT1 platform series (Air780E/Air700E/Air780EP, etc.), range 1650 ~ 2000, 2650 ~ 3400, unit millivolt, step 50mv
-- Note that the setting priority here will be higher than the configuration of the hardware IOSEL pin.
-- But when booting, the hardware configuration is still used until this API is called for configuration, so the io level will change.
-- pm.ioVol(pm.IOVOL_ALL_GPIO, 3300) -- All GPIO high level outputs 3.3V
-- pm.ioVol(pm.IOVOL_ALL_GPIO, 1800) -- All GPIO high level outputs 1.8V*/
static int l_pm_iovolt_ctrl(lua_State *L) {
int val = 3300;
 int id = luaL_optinteger(L, 1, LUAT_PM_ALL_GPIO);
 if (lua_isboolean(L, 2)) {
	val = lua_toboolean(L, 2);
 }
 else if (lua_isinteger(L, 2)) {
	 val = luaL_checkinteger(L, 2);
 }
 lua_pushboolean(L, !luat_pm_iovolt_ctrl(id, val));
 return 1;
}

#ifndef LUAT_COMPILER_NOWEAK
LUAT_WEAK int luat_pm_iovolt_ctrl(int id, int val) {
 return -1;
}
#endif


/**
Configure wake-up pin (currently only available for esp series)
@api pm.wakeupPin(pin,level)
@int/table gpio pins
@int wake-up voltage optional, default low level wake-up
@return boolean processing result
@usage
pm.wakeupPin(8,0)*/
static int l_pm_wakeup_pin(lua_State *L) {
    int level = luaL_optinteger(L, 2,0);
    if (lua_istable(L, 1)) {
        size_t count = lua_rawlen(L, 1);
		for (size_t i = 1; i <= count; i++){
			lua_geti(L, 1, i);
            luat_pm_wakeup_pin(luaL_checkinteger(L, -1), level);
			lua_pop(L, 1);
		}
    }else if(lua_isnumber(L, 1)){
        luat_pm_wakeup_pin(luaL_checkinteger(L, 1), level);
    }
    lua_pushboolean(L, 1);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_pm[] =
{
    { "request" ,       ROREG_FUNC(l_pm_request )},
    // { "release" ,    ROREG_FUNC(   l_pm_release)},
    { "dtimerStart",    ROREG_FUNC(l_pm_dtimer_start)},
    { "dtimerStop" ,    ROREG_FUNC(l_pm_dtimer_stop)},
	{ "dtimerCheck" ,   ROREG_FUNC(l_pm_dtimer_check)},
    { "dtimerList",     ROREG_FUNC(l_pm_dtimer_list)},
    { "dtimerWkId",     ROREG_FUNC(l_pm_dtimer_wakeup_id)},
    //{ "on",           ROREG_FUNC(l_pm_on)},
    { "force",          ROREG_FUNC(l_pm_force)},
    { "check",          ROREG_FUNC(l_pm_check)},
    { "lastReson",      ROREG_FUNC(l_pm_last_reson)},
    { "shutdown",       ROREG_FUNC(l_pm_power_off)},
    { "reboot",         ROREG_FUNC(l_rtos_reboot)},
	{ "power",          ROREG_FUNC(l_pm_power_ctrl)},
    { "ioVol",         ROREG_FUNC(l_pm_iovolt_ctrl)},
    { "wakeupPin",         ROREG_FUNC(l_pm_wakeup_pin)},


    //@const NONE number No sleep mode
    { "NONE",           ROREG_INT(LUAT_PM_SLEEP_MODE_NONE)},
    //@const IDLE number IDLE mode
    { "IDLE",           ROREG_INT(LUAT_PM_SLEEP_MODE_IDLE)},
    //@const LIGHT number LIGHT模式
    { "LIGHT",          ROREG_INT(LUAT_PM_SLEEP_MODE_LIGHT)},
    //@const DEEP number DEEP mode
    { "DEEP",           ROREG_INT(LUAT_PM_SLEEP_MODE_DEEP)},
    //@const HIB number HIB mode
    { "HIB",            ROREG_INT(LUAT_PM_SLEEP_MODE_STANDBY)},
    //@const USB number USB power supply
    { "USB",            ROREG_INT(LUAT_PM_POWER_USB)},
    //@const GPS number GPS power
    { "GPS",            ROREG_INT(LUAT_PM_POWER_GPS)},
    //@const GPS_ANT number GPS antenna power supply, only required for active antennas
    { "GPS_ANT",        ROREG_INT(LUAT_PM_POWER_GPS_ANT)},
    //@const CAMERA number camera power supply, CAM_VCC output
    { "CAMERA",         ROREG_INT(LUAT_PM_POWER_CAMERA)},
    //@const DAC_EN number Air780E and Air600E, Air780EP's DAC_EN (LDO_CTL in the new version of the hardware manual, the same PIN, naming change), note that the default configuration of audio will automatically use this pin to control the enablement of CODEC
    { "DAC_EN",         ROREG_INT(LUAT_PM_POWER_DAC_EN_PIN)},
    //@const LDO_CTL number Air780E and Air600E, LDO_CTL of Air780EP (DAC_EN of the old version of the hardware manual, the same PIN, naming change), LDO_CTL of Air780EP, note that the default configuration of audio will automatically use this pin to control the enablement of CODEC
    { "LDO_CTL",         ROREG_INT(LUAT_PM_POWER_LDO_CTL_PIN)},
    //@const PWK_MODE number Whether to turn on the powerkey filter mode of the core CAT1 platform series (Air780E/Air700E/Air780EP, etc.), true to turn on, please note that reset in filter mode turns into direct shutdown
    { "PWK_MODE",       ROREG_INT(LUAT_PM_POWER_POWERKEY_MODE)},
    //@const WORK_MODE number The energy-saving mode of Yixin CAT1 platform series (Air780E/Air700E/Air780EP, etc.), 0~3, 0 completely off, 1 performance priority, 2 balance, 3 extreme power consumption
    { "WORK_MODE",    ROREG_INT(LUAT_PM_POWER_WORK_MODE)},
	//@const IOVL number All GPIO high-level voltage controls, currently only available for the Yixin CAT1 platform series (Air780E/Air700E/Air780EP, etc.)
    { "IOVOL_ALL_GPIO",    ROREG_INT(LUAT_PM_ALL_GPIO)},

	{ NULL,             ROREG_INT(0) }
};

LUAMOD_API int luaopen_pm( lua_State *L ) {
    luat_newlib2(L, reg_pm);
    return 1;
}
