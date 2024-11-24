/*@Modules touchkey
@summary touch button
@version 1.0
@date 2022.01.15
@tag LUAT_USE_TOUCHKEY*/
#include "luat_base.h"
#include "luat_touchkey.h"
#include "luat_msgbus.h"

/*Configure touch buttons
@api touchkey.setup(id, scan_period, window, threshold)
@int sensor id, please check the hardware documentation. For example, air101/air103 supports 1~15, for example, PA7 corresponds to touch id=1
@int Scan interval, range 1 ~ 0x3F, unit 16ms, optional
@int Scan window, range 2-7, optional
@int threshold, range 0-127, optional
@return bool returns true on success, false on failure
@usage
touchkey.setup(1)
sys.subscribe("TOUCHKEY_INC", function(id, count)
    -- sensor id
    -- Counter, touch count statistics
    log.info("touchkey", id, count)
end)*/
static int l_touchkey_setup(lua_State *L) {
    luat_touchkey_conf_t conf;

    conf.id = (uint8_t)luaL_checkinteger(L, 1);
    conf.scan_period = (uint8_t)luaL_optinteger(L, 2, 0);
    conf.window = (uint8_t)luaL_optinteger(L, 2, 0);
    conf.threshold = (uint8_t)luaL_optinteger(L, 2, 0);

    int ret = luat_touchkey_setup(&conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Turn off initial touch keys
@api touchkey.close(id)
@int sensor id, please consult the hardware documentation
@return nil no return value
@usage
-- Looks like it's unlikely to need to be turned off
touchkey.close(1)*/
static int l_touchkey_close(lua_State *L) {
    uint8_t pin = (uint8_t)luaL_checkinteger(L, 1);
    luat_touchkey_close(pin);
    return 0;
}

int l_touchkey_handler(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    if (lua_isnil(L, -1)) {
        lua_pushinteger(L, 0);
        return 1;
    }
/*@sys_pub touchkey
Touch button message
TOUCHKEY_INC
@number port, sensor id
@number state, counter, touch count statistics
@usage
sys.subscribe("TOUCHKEY_INC", function(id, count)
    -- sensor id
    -- Counter, touch count statistics
    log.info("touchkey", id, count)
end)*/
    lua_pushliteral(L, "TOUCHKEY_INC");
    lua_pushinteger(L, msg->arg1);
    lua_pushinteger(L, msg->arg2);
    lua_call(L, 3, 0);
    return 0;
}


#include "rotable2.h"
static const rotable_Reg_t reg_touchkey[] =
{
    { "setup",  ROREG_FUNC(l_touchkey_setup)},
    { "close",  ROREG_FUNC(l_touchkey_close)},
    { NULL,     ROREG_INT(0) }
};

LUAMOD_API int luaopen_touchkey(lua_State *L)
{
    luat_newlib2(L, reg_touchkey);
    return 1;
}
