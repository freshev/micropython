/*@Modulesrtc
@summary real time clock
@version 1.0
@date 2021.08.31
@demortc
@tag LUAT_USE_RTC*/
#include "luat_base.h"
#include "luat_rtc.h"

#define LUAT_LOG_TAG "rtc"
#include "luat_log.h"

int Base_year = 1900;

#ifndef LUAT_COMPILER_NOWEAK
void LUAT_WEAK luat_rtc_set_tamp32(uint32_t tamp) {
    LLOGD("not support yet");
}
#endif

/*set clock
@api rtc.set(tab)
@table or int clock parameter, see example
@return bool Returns true if successful, otherwise returns nil or false
@usage
rtc.set({year=2021,mon=8,day=31,hour=17,min=8,sec=43})
--Currently only the Air101/Air103/Air105/EC618 series support the timestamp method
rtc.set(1652230554)*/
static int l_rtc_set(lua_State *L){
    struct tm tblock = {0};
    int ret;
    if (!lua_istable(L, 1)) {
    	if (lua_isinteger(L, 1))
    	{
    		uint32_t tamp = lua_tointeger(L, 1);
    	    luat_rtc_set_tamp32(tamp);
    	    lua_pushboolean(L, 1);
    	    return 1;
    	}
        LLOGW("rtc time need table");
        return 0;
    }
    lua_settop(L, 1);

    lua_pushstring(L, "year");
    lua_gettable(L, 1);
    if (lua_isnumber(L, -1)) {
        tblock.tm_year = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss year");
        return 0;
    }

    lua_pushstring(L, "mon");
    lua_gettable(L, 1);
    if (lua_isnumber(L, -1)) {
        tblock.tm_mon = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss mon");
        return 0;
    }

    lua_pushstring(L, "day");
    lua_gettable(L, 1);
    if (lua_isnumber(L, -1)) {
        tblock.tm_mday = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss day");
        return 0;
    }

    lua_pushstring(L, "hour");
    lua_gettable(L, 1);
    if (lua_isnumber(L, -1)) {
        tblock.tm_hour = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss hour");
        return 0;
    }

    lua_pushstring(L, "min");
    lua_gettable(L, 1);
    if (lua_isnumber(L, -1)) {
        tblock.tm_min = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss min");
        return 0;
    }

    lua_pushstring(L, "sec");
    lua_gettable(L, 1);
    if (lua_isnumber(L, -1)) {
        tblock.tm_sec = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss sec");
        return 0;
    }

    tblock.tm_year -= Base_year;
    tblock.tm_mon -= 1;

    ret = luat_rtc_set(&tblock);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Get clock
@apirtc.get()
@return table clock parameters, see example
@usage
local t = rtc.get()
-- {year=2021,mon=8,day=31,hour=17,min=8,sec=43}
log.info("rtc", json.encode(t))*/
static int l_rtc_get(lua_State *L){
    struct tm tblock = {0};
    int ret;
    ret = luat_rtc_get(&tblock);
    if (ret) {
        return 0;
    }

    tblock.tm_year += Base_year;
    tblock.tm_mon += 1;


    lua_newtable(L);

    lua_pushstring(L, "year");
    lua_pushinteger(L, tblock.tm_year );
    lua_settable(L, -3);

    lua_pushstring(L, "mon");
    lua_pushinteger(L, tblock.tm_mon );
    lua_settable(L, -3);

    lua_pushstring(L, "day");
    lua_pushinteger(L, tblock.tm_mday);
    lua_settable(L, -3);

    lua_pushstring(L, "hour");
    lua_pushinteger(L, tblock.tm_hour);
    lua_settable(L, -3);

    lua_pushstring(L, "min");
    lua_pushinteger(L, tblock.tm_min);
    lua_settable(L, -3);

    lua_pushstring(L, "sec");
    lua_pushinteger(L, tblock.tm_sec);
    lua_settable(L, -3);

    return 1;
}

/*Set RTC wake-up time
@api rtc.timerStart(id, tab)
@int clock id, usually only supports 0
@table clock parameters, see example
@return bool Returns true if successful, otherwise returns nil or false
@usage
--Currently, this interface is not suitable for the 780E/700E/780EP series of core-moving Moduless. If you need to wake up regularly, you can use pm.dtimerStart()
-- It is recommended to set rtc.set to the correct time before use
rtc.timerStart(0, {year=2021,mon=9,day=1,hour=17,min=8,sec=43})*/
static int l_rtc_timer_start(lua_State *L){
    int id;
    struct tm tblock = {0};
    int ret;

    id = luaL_checkinteger(L, 1);
    if (!lua_istable(L, 2)) {
        LLOGW("rtc time need table");
        return 0;
    }

    lua_settop(L, 2);
    lua_pushstring(L, "year");
    lua_gettable(L, 2);
    if (lua_isnumber(L, -1)) {
        tblock.tm_year = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss year");
        return 0;
    }

    lua_pushstring(L, "mon");
    lua_gettable(L, 2);
    if (lua_isnumber(L, -1)) {
        tblock.tm_mon = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss mon");
        return 0;
    }

    lua_pushstring(L, "day");
    lua_gettable(L, 2);
    if (lua_isnumber(L, -1)) {
        tblock.tm_mday = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss day");
        return 0;
    }

    lua_pushstring(L, "hour");
    lua_gettable(L, 2);
    if (lua_isnumber(L, -1)) {
        tblock.tm_hour = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss hour");
        return 0;
    }

    lua_pushstring(L, "min");
    lua_gettable(L, 2);
    if (lua_isnumber(L, -1)) {
        tblock.tm_min = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss min");
        return 0;
    }

    lua_pushstring(L, "sec");
    lua_gettable(L, 2);
    if (lua_isnumber(L, -1)) {
        tblock.tm_sec = luaL_checkinteger(L, -1);
    }
    else {
        LLOGW("rtc time miss sec");
        return 0;
    }

    tblock.tm_year -= Base_year;
    tblock.tm_mon -= 1;

    ret = luat_rtc_timer_start(id, &tblock);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Cancel RTC wake-up time
@api rtc.timerStop(id)
@int clock id, usually only supports 0
@return bool Returns true if successful, otherwise returns nil or false
@usage
rtc.timerStop(0)*/
static int l_rtc_timer_stop(lua_State *L){
    int id;
    int ret;

    id = luaL_checkinteger(L, 1);
    ret = luat_rtc_timer_stop(id);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Set RTC base year, not recommended
@api rtc.setBaseYear(Base_year)
@int Base year Base_year, usually 1900
@usage
rtc.setBaseYear(1900)*/
static int l_rtc_set_base_year(lua_State *L){
    Base_year = luaL_checkinteger(L, 1);
    return 0;
}

/*Read or set time zone
@api rtc.timezone(tz)
@int Time zone value, please note that the unit is 1/4 time zone. For example, East Eighth District is 32, not 8. It does not need to be passed
@return current/set time zone value
@usage
-- Set to East 8th District
rtc.timezone(32)
-- Set to East 3 District
rtc.timezone(12)
--Set to West 4th District
rtc.timezone(-16)
-- Note: No matter what time zone is set, rtc.get/set is always UTC time
-- The time zone affects the os.date/os.time function
--Only some Moduless support setting time zone, and the default value is generally 32, which is the East Eighth District*/
static int l_rtc_timezone(lua_State *L){
    int timezone = 0;
    if (lua_isinteger(L, 1)) {
        timezone = luaL_checkinteger(L, 1);
        timezone = luat_rtc_timezone(&timezone);
    }
    else {
        timezone = luat_rtc_timezone(NULL);
    }
    lua_pushinteger(L, timezone);
    return 1;
}


#include "rotable2.h"
static const rotable_Reg_t reg_rtc[] =
{
    { "set",        ROREG_FUNC(l_rtc_set)},
    { "get",        ROREG_FUNC(l_rtc_get)},
    { "timerStart", ROREG_FUNC(l_rtc_timer_start)},
    { "timerStop",  ROREG_FUNC(l_rtc_timer_stop)},
    { "setBaseYear", ROREG_FUNC(l_rtc_set_base_year)},
    { "timezone",   ROREG_FUNC(l_rtc_timezone)},
	{ NULL,         ROREG_INT(0) }
};

LUAMOD_API int luaopen_rtc( lua_State *L ) {
    luat_newlib2(L, reg_rtc);
    return 1;
}
