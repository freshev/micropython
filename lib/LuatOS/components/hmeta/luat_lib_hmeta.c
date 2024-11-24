/*@Moduleshmeta
@summary hardware metadata
@version 1.0
@date 2022.01.11
@demohmeta
@tag LUAT_USE_HMETA
@usage
-- This library is under development
--[[
    The purpose of this library is to demonstrate the capabilities of the current hardware, for example:
1. How many GPIOs are there, what is the default mode of each GPIO, and whether it supports pull-up/pull-down
2. How many I2Cs are there and what rates are supported?
3. How many SPIs are there and what rates and modes are supported?
4. Extended attributes, such as distinguishing Air780E and Air600E

]]*/
#include "luat_base.h"
#include "luat_hmeta.h"

/*Get Modules name
@apihmeta.model()
@return string If it can be recognized, return the Modules type, otherwise it will be nil
@usage
sys.taskInit(function()
    while 1 do
        sys.wait(3000)
        -- hmeta identifies the underlying Modules type
        -- Different Moduless can use the same bsp, but depending on the package, the specific Modules can still be identified based on internal data
        log.info("hmeta", hmeta.model())
        log.info("bsp", rtos.bsp())
    end
end)*/
static int l_hmeta_model(lua_State *L) {
    char buff[40] = {0};
    luat_hmeta_model_name(buff);
    if (strlen(buff)) {
        lua_pushstring(L, buff);
    }
    else {
        lua_pushnil(L);
    }
    return 1;
}

/*Get the hardware version number of the Modules
@apihmeta.hwver()
@return string If it can be recognized, return the Modules type, otherwise it will be nil
@usage
sys.taskInit(function()
    while 1 do
        sys.wait(3000)
        -- hmeta identifies the underlying Modules type
        -- Different Moduless can use the same bsp, but depending on the package, the specific Modules can still be identified based on internal data
        log.info("hmeta", hmeta.model(), hmeta.hwver())
        log.info("bsp", rtos.bsp())
    end
end)*/
static int l_hmeta_hwver(lua_State *L) {
    char buff[40] = {0};
    luat_hmeta_hwversion(buff);
    if (strlen(buff)) {
        lua_pushstring(L, buff);
    }
    else {
        lua_pushnil(L);
    }
    return 1;
}

// static int l_hmeta_gpio(lua_State *L) {
//     return 0;
// }

// static int l_hmeta_uart(lua_State *L) {
//     return 0;
// }

#include "rotable2.h"
static const rotable_Reg_t reg_hmeta[] =
{
    { "model" ,           ROREG_FUNC(l_hmeta_model)},
    { "hwver" ,           ROREG_FUNC(l_hmeta_hwver)},
    // { "gpio" ,            ROREG_FUNC(l_hmeta_gpio)},
    // { "uart" ,            ROREG_FUNC(l_hmeta_uart)},
	{ NULL,               ROREG_INT(0)}
};

LUAMOD_API int luaopen_hmeta( lua_State *L ) {
    luat_newlib2(L, reg_hmeta);
    return 1;
}
