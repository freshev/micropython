/*@Modules timer
@summary Operating the underlying timer
@version 1.0
@date 2020.03.30
@tag LUAT_USE_TIMER*/
#include "luat_base.h"
#include "luat_log.h"
#include "luat_timer.h"
#include "luat_mem.h"

/*Hard blocking specified duration
@api timer.mdelay(timeout)
@int Blocking duration, unit ms, maximum 1024ms, actual use is strongly recommended not to exceed 200ms
@return nil no return value
@usage
-- No luat code will be executed during this period, including the underlying message processing mechanism
-- This method is generally not used unless you know exactly what will happen.
timer.mdelay(10)*/
static int l_timer_mdelay(lua_State *L) {
    if (lua_isinteger(L, 1)) {
        lua_Integer ms = luaL_checkinteger(L, 1);
        if (ms > 0 && ms < 1024)
            luat_timer_mdelay(ms);
    }
    return 0;
}

/*Hard blocking specifies the duration but at the us level, it will not be very accurate.
@api timer.udelay(timeout)
@int Blocking duration, unit us, maximum 3000us
@return nil no return value
@usage
-- This method is generally not used unless you know exactly what will happen.
-- This API was added on 2023.05.18
timer.udelay(10)
--The actual blocking duration fluctuates*/
static int l_timer_udelay(lua_State *L) {
    if (lua_isinteger(L, 1)) {
        lua_Integer us = luaL_checkinteger(L, 1);
        if (us > 0 && us <= 3000)
            luat_timer_us_delay(us);
    }
    return 0;
}

//TODO supports hwtimer

#include "rotable2.h"
static const rotable_Reg_t reg_timer[] =
{
    { "mdelay", ROREG_FUNC(l_timer_mdelay)},
    { "udelay", ROREG_FUNC(l_timer_udelay)},
	{ NULL,     ROREG_INT(0) }
};

LUAMOD_API int luaopen_timer( lua_State *L ) {
    luat_newlib2(L, reg_timer);
    return 1;
}
