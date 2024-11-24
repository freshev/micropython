/*@Modules wdt
@summary watchdog operation library
@version 1.0
@date 2021.08.06
@demo wdt
@tagLUAT_USE_WDT*/
#include "luat_base.h"
#include "luat_wdt.h"

/*Initialize the watchdog and enable it immediately. For most devices, the watchdog cannot be turned off once it is enabled.
@api wdt.init(timeout)
@int timeout length, unit is milliseconds
@return bool Returns true if successful, otherwise returns false (for example, the underlying layer does not support it)
@usage
wdt.init(9000)
sys.timerLoopStart(wdt.feed, 3000)*/
static int l_wdt_init(lua_State *L) {
    int timeout = luaL_optinteger(L, 1, 10);
    int ret = luat_wdt_init(timeout);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Some devices support resetting the watchdog timeout period.
@api wdt.setTimeout(timeout)
@int timeout length, unit is milliseconds
@return bool Returns true if successful, otherwise returns false (for example, the underlying layer does not support it)
@usage
wdt.init(10000)
sys.timerLoopStart(wdt.feed, 3000)
sys.wait(5000)
sys.setTimeout(5000)*/
static int l_wdt_set_timeout(lua_State *L) {
    int timeout = luaL_optinteger(L, 1, 10);
    int ret = luat_wdt_set_timeout(timeout);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Feed the watchdog to reset the timeout and restart the timer
@api wdt.feed()
@return bool Returns true if successful, otherwise returns false (for example, the underlying layer does not support it)
@usage
wdt.init(10000)
-- Feed the watchdog regularly, or feed the watchdog as needed according to business needs
sys.timerLoopStart(wdt.feed, 3000)*/
static int l_wdt_feed(lua_State *L) {
    int ret = luat_wdt_feed();
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Turn off watchdog, usually not supported
@api wdt.close()
@return bool Returns true if successful, otherwise returns false (for example, the underlying layer does not support it)
@usage
wdt.init(10000)
sys.wait(9000)
wdt.close()*/
static int l_wdt_close(lua_State *L) {
    int ret = luat_wdt_feed();
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_wdt[] =
{
    { "init",       ROREG_FUNC(l_wdt_init)},
    { "setTimeout", ROREG_FUNC(l_wdt_set_timeout)},
    { "feed",       ROREG_FUNC(l_wdt_feed)},
    { "close",      ROREG_FUNC(l_wdt_close)},
	{ NULL,         ROREG_INT(0) }
};

LUAMOD_API int luaopen_wdt( lua_State *L ) {
    luat_newlib2(L, reg_wdt);
    return 1;
}
