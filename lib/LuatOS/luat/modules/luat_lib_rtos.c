/*@Modules rtos
@summary RTOS underlying operation library
@version 1.0
@date 2020.03.30
@tag LUAT_CONF_BSP*/
#include "luat_base.h"
#include "luat_sys.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_mem.h"

#define LUAT_LOG_TAG "rtos"
#include "luat_log.h"
static uint32_t autogc_high_water = 90;
static uint32_t autogc_mid_water = 80;
static uint16_t autogc_config = 100; // TODO configurable via API
static uint16_t autogc_counter = 0;
#ifndef LUAT_COMPILER_NOWEAK
LUAT_WEAK uint8_t luat_msgbus_is_empty(void)
{
	return 0;
}
#endif
/*Accepts and processes the underlying message queue.
@api rtos.receive(timeout)
@int timeout length, usually -1, wait forever
@return msgid If it is a timer message, the timer message id and additional information will be returned. Other messages are determined by the bottom layer and do not make any guarantees to the Lua layer.
-- This method is called through sys.run() and should not be used by ordinary users.
rtos.receive(-1)*/
static int l_rtos_receive(lua_State *L) {
    rtos_msg_t msg = {0};
    int re = {0};
    size_t total = 0;
    size_t used = 0;
    size_t max_used = 0;
    //The system is idle and the automatic collection function is set
    if (luat_msgbus_is_empty() && autogc_config)
    {
    	//LLOGD("auto collect check %d,%d", luat_msgbus_is_empty(), autogc_config);
    	luat_meminfo_luavm(&total, &used, &max_used);
    	//Collect directly when reaching the mandatory line
        if ( (used * 100) >= (total * autogc_high_water))
        {
            //LLOGD("luavm ram too high! used %d, total %d. Trigger Force-GC", used, total);
            // Need to be executed twice, because userdata will not be recycled until the second time
            lua_gc(L, LUA_GCCOLLECT, 0);
            lua_gc(L, LUA_GCCOLLECT, 0);
        }
        else
        {

            if (autogc_counter >= autogc_config) {
                autogc_counter = 0;
                if ( (used * 100) >= (total * autogc_mid_water))
                {
                    //LLOGD("luavm ram too high! used %d, total %d. Trigger Force-GC", used, total);
                    // Need to be executed twice, because userdata will not be recycled until the second time
                    lua_gc(L, LUA_GCCOLLECT, 0);
                    lua_gc(L, LUA_GCCOLLECT, 0);
                }
            }
            else {
                autogc_counter ++;
            }
        }
    }
    else
    {
    	autogc_counter = 0;	//Maybe it can’t be cleared here
    }


    re = luat_msgbus_get(&msg, luaL_checkinteger(L, 1));
    if (!re) {
        //LLOGD("rtos_msg got, invoke it handler=%08X", msg.handler);
        lua_pushlightuserdata(L, (void*)(&msg));
        return msg.handler(L, msg.ptr);
    }
    else {
        //LLOGD("rtos_msg get timeout");
        lua_pushinteger(L, -1);
        return 1;
    }
}

//------------------------------------------------------------------
static int l_timer_handler(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    luat_timer_t *timer = (luat_timer_t *)ptr;
    int timer_id = msg->arg1;
    if (timer_id > 0) {
        timer = luat_timer_get(timer_id);
    }
    else if (timer != NULL) {
        timer_id = timer->id;
        timer = luat_timer_get(timer_id);
    }
    if (timer == NULL)
        return 0;
    // LLOGD("l_timer_handler id=%ld\n", timer->id);
    lua_pushinteger(L, MSG_TIMER);
    lua_pushinteger(L, timer->id);
    lua_pushinteger(L, timer->repeat);
    //lua_pushinteger(L, timer->timeout);
    if (timer->repeat == 0) {
        // LLOGD("stop timer %d", timer_id);
        luat_timer_stop(timer);
        luat_heap_free(timer);
    }
    else if (timer->repeat > 0) {
        timer->repeat --;
    }
    return 3;
}

/*Start a timer
@api rtos.timer_start(id,timeout,_repeat)
@int timer id
@int timeout duration, unit milliseconds
@int Number of repetitions, default is 0
@return id If it is a timer message, the timer message id and additional information will be returned. Other messages are determined by the bottom layer and do not make any guarantees to the Lua layer.
@usage
-- Please use sys.timerStart for user code
-- Start a 3-second loop timer
rtos.timer_start(10000, 3000, -1)*/
static int l_rtos_timer_start(lua_State *L) {
    lua_gettop(L);
    size_t timeout = 0;
    size_t type = 0;
    size_t id = (size_t)luaL_checkinteger(L, 1) / 1;
#if 0
    if (lua_isnumber(L, 2)) {
    	timeout = lua_tonumber(L, 2) * 1000;
    	type = 1;
    } else
#endif
    	timeout = (size_t)luaL_checkinteger(L, 2);
    int repeat = (size_t)luaL_optinteger(L, 3, 0);
    // LLOGD("start timer id=%ld", id);
    // LLOGD("timer timeout=%ld", timeout);
    // LLOGD("timer repeat=%ld", repeat);
    if (timeout < 1) {
        lua_pushinteger(L, 0);
        return 1;
    }
    luat_timer_t *timer = (luat_timer_t*)luat_heap_malloc(sizeof(luat_timer_t));
    if (timer == NULL){
        LLOGE("timer malloc fail");
        lua_pushinteger(L, 0);
        return 1;
    }
    
    timer->id = id;
    timer->timeout = timeout;
    timer->repeat = repeat;
    timer->func = &l_timer_handler;
    timer->type = type;
    int re = luat_timer_start(timer);
    if (re == 0) {
        lua_pushinteger(L, 1);
    }
    else {
        LLOGD("start timer fail, free timer %p", timer);
        luat_heap_free(timer);
        lua_pushinteger(L, 0);
    }
    return 1;
}

/*Close and release a timer
@api rtos.timer_stop(id)
@int timer id
@return nil no return value
@usage
-- Please use sys.timerStop for user code
rtos.timer_stop(id)*/
static int l_rtos_timer_stop(lua_State *L) {
    int timerid = -1;
    luat_timer_t *timer = NULL;
    if (!lua_isinteger(L, 1)) {
        return 0;
    }
    timerid = lua_tointeger(L, 1);
    timer = luat_timer_get(timerid);
    if (timer != NULL) {
        // LLOGD("timer stop, free timer %d", timerid);
        luat_timer_stop(timer);
        luat_heap_free(timer);
    }
    return 0;
}

/*Device restart
@api rtos.reboot()
@return nil no return value
-- Reboot device now
rtos.reboot()*/
int l_rtos_reboot(lua_State *L) {
    luat_os_reboot(luaL_optinteger(L, 1, 0));
    return 0;
}

//-----------------------------------------------------------------

/*Get firmware compilation date
@api rtos.buildDate()
@return string firmware compilation date
@usage
-- Get the compilation date
local d = rtos.buildDate()*/
static int l_rtos_build_date(lua_State *L) {
    lua_pushstring(L, __DATE__);
    return 1;
}

/*Get hardware bsp model
@api rtos.bsp()
@return string hardware bsp model
@usage
-- Get the hardware bsp model
local bsp = rtos.bsp()*/
static int l_rtos_bsp(lua_State *L) {
    lua_pushstring(L, luat_os_bsp());
    return 1;
}

/*Get firmware version number
@api rtos.version()
@return string firmware version number, such as "V0001"
@usage
-- Read the version number
local luatos_version = rtos.version()*/
static int l_rtos_version(lua_State *L) {
    lua_pushstring(L, luat_version_str());
    return 1;
}

/*Enter standby mode, only some devices are available, this API is obsolete, it is recommended to use the pm library
@api rtos.standy(timeout)
@int Sleep duration, in milliseconds
@return nil no return value
@usage
--Enter standby mode
rtos.standby(5000)*/
static int l_rtos_standy(lua_State *L) {
    int timeout = luaL_checkinteger(L, 1);
    luat_os_standy(timeout);
    return 0;
}

/*Get memory information
@api rtos.meminfo(type)
@type "sys" system memory, "lua" virtual machine memory, "psram" psram memory, the default is lua virtual machine memory
@return int total memory size, unit bytes
@return int The currently used memory size, in bytes
@return int The highest used memory size in history, in bytes
@usage
--Print memory usage
log.info("mem.lua", rtos.meminfo())
log.info("mem.sys", rtos.meminfo("sys"))*/
static int l_rtos_meminfo(lua_State *L) {
    size_t len = 0;
    size_t total = 0;
    size_t used = 0;
    size_t max_used = 0;
    const char * str = luaL_optlstring(L, 1, "lua", &len);
    if (strcmp("sys", str) == 0) {
        luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &used, &max_used);
    }
    else if(strcmp("psram", str) == 0){
        luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &used, &max_used);
    }
    else {
        luat_meminfo_luavm(&total, &used, &max_used);
    }
    
    lua_pushinteger(L, total);
    lua_pushinteger(L, used);
    lua_pushinteger(L, max_used);
    return 3;
}

/*Returns the underlying description information in the format LuatOS_$VERSION_$BSP, which can be used for OTA upgrade to determine the underlying information.
@api rtos.firmware()
@return string underlying description information
@usage
--Print the underlying description information
log.info("firmware", rtos.firmware())*/
static int l_rtos_firmware(lua_State *L) {
    lua_pushfstring(L, "LuatOS-SoC_%s_%s", luat_version_str(), luat_os_bsp());
    return 1;
}

extern char custom_search_paths[4][24];

/*Set a custom Lua script search path with higher priority than the built-in path
@api rtos.setPaths(pathA, pathB, pathC, pathD)
@string Path A, for example "/sdcard/%s.luac", if no value is passed, it will default to "". In addition, the maximum length cannot exceed 23 bytes.
@string path B, for example "/sdcard/%s.lua"
@string path C, for example "/lfs2/%s.luac"
@string path D, for example "/lfs2/%s.lua"
@usage
-- After mounting the sd card or spiflash
rtos.setPaths("/sdcard/user/%s.luac", "/sdcard/user/%s.lua")
require("sd_user_main") -- will search and load /sdcard/user/sd_user_main.luac and /sdcard/user/sd_user_main.lua*/
static int l_rtos_set_paths(lua_State *L) {
    size_t len = 0;
    const char* str = NULL;
    for (size_t i = 0; i < 4; i++)
    {
        if (lua_isstring(L, i +1)) {
            str = luaL_checklstring(L, i+1, &len);
            memcpy(custom_search_paths[i], str, len + 1);
        }
        else {
            custom_search_paths[i][0] = 0x00;
        }
    }
    return 0;
}

/*Empty function, does nothing
@api rtos.nop()
@return nil no return value
@usage
-- This function is simply lua -> c -> lua. Go through it.
-- No parameters, no return value, no logical processing
-- In most cases, you will not encounter a call to this function
-- It usually only appears in performance testing code, because it does nothing.
rtos.nop()*/
static int l_rtos_nop(lua_State *L) {
    (void)L;
    return 0;
}

/*Automatic memory collection configuration is a supplement to Lua's own collection mechanism. It is not necessary and is only triggered when Luavm is idle.
@api rtos.autoCollectMem(period, warning_level, force_level)
@int The period of automatic collection, which is equivalent to the number of receive calls, 0~60000. If it is 0, the automatic collection function is turned off. The default is 100.
@int The memory usage warning water level is the percentage of the total luavm memory, 50~95. When the memory reaches (>=) the warning line, it will start to determine whether to collect. The default is 80
@int The memory usage forced collection water level is the percentage of the total luavm memory, 50~95. When the memory reaches (>=) the forced collection line, it will be forced to collect. The default is 90, which must be larger than the warning water level.
@return nil no return value
@usage
rtos.autoCollectMem(100, 80, 90)*/
static int l_rtos_auto_colloect_mem(lua_State *L) {
    uint32_t period = luaL_optinteger(L, 1, 100);
    uint32_t mid = luaL_optinteger(L, 2, 80);
    uint32_t high = luaL_optinteger(L, 3, 90);
    if (period > 60000) {
    	return 0;
    }
    if (mid > 95 || high > 95) {
    	return 0;
    }
    if (mid < 50 || high < 50) {
    	return 0;
    }
    if (mid >= high) {
    	return 0;
    }
    LLOGD("mem collect param %u,%u,%u -> %u,%u,%u", autogc_config, autogc_mid_water, autogc_high_water, period, mid, high);
    autogc_config = period;
	autogc_mid_water = mid;
	autogc_high_water = high;
    return 0;
}

// TODO Some platforms do not support LUAT_WEAK
LUAT_WEAK int luat_poweron_reason(void) {
    return 0;
}

//uint32_t-high 16 bits main reason for restart, low 16 bits detailed reason
//High 16 bits: 0-power on/reset boot, 1-user active software restart, 2-RTC boot, 3-abnormal restart, 4-wake up boot
//lower 16 bits
//
// static int l_rtos_poweron_reason(lua_State *L) {
//     lua_pushinteger(L,luat_poweron_reason());
//     return 1;
// }

//------------------------------------------------------------------
#include "rotable2.h"
static const rotable_Reg_t reg_rtos[] =
{
    { "timer_start" ,      ROREG_FUNC(l_rtos_timer_start)},
    { "timer_stop",        ROREG_FUNC(l_rtos_timer_stop)},
    { "receive",           ROREG_FUNC(l_rtos_receive)},
    { "reboot",            ROREG_FUNC(l_rtos_reboot)},
    // { "poweron_reason",    ROREG_FUNC(l_rtos_poweron_reason)},
    { "standy",            ROREG_FUNC(l_rtos_standy)},

    { "buildDate",         ROREG_FUNC(l_rtos_build_date)},
    { "bsp",               ROREG_FUNC(l_rtos_bsp)},
    { "version",           ROREG_FUNC(l_rtos_version)},
    { "meminfo",           ROREG_FUNC(l_rtos_meminfo)},
    { "firmware",          ROREG_FUNC(l_rtos_firmware)},
    { "setPaths",          ROREG_FUNC(l_rtos_set_paths)},
    { "nop",               ROREG_FUNC(l_rtos_nop)},
	{ "autoCollectMem",          ROREG_FUNC(l_rtos_auto_colloect_mem)},
    { "INF_TIMEOUT",       ROREG_INT(-1)},

    { "MSG_TIMER",         ROREG_INT(MSG_TIMER)},
    // { "MSG_GPIO",           NULL,              MSG_GPIO},
    // { "MSG_UART_RX",        NULL,              MSG_UART_RX},
    // { "MSG_UART_TXDONE",    NULL,              MSG_UART_TXDONE},
	{ NULL,                ROREG_INT(0) }
};

LUAMOD_API int luaopen_rtos( lua_State *L ) {
    luat_newlib2(L, reg_rtos);
    return 1;
}

LUAT_WEAK const char* luat_version_str(void) {
    #ifdef LUAT_BSP_VERSION
    return LUAT_BSP_VERSION;
    #else
    return LUAT_VERSION;
    #endif
}
