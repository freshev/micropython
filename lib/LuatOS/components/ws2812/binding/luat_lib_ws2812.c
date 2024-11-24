
/*@Modules ws2812
@summary Fantasy light with RGB controller (WS2812 series)
@version 1.0
@date 2023.11.14
@author wendal
@tag LUAT_USE_WS2812
@usage
-- This library is still under development*/

#include "luat_base.h"
#include "luat_mem.h"
#include "luat_ymodem.h"
#include "luat_zbuff.h"
#define LUAT_LOG_TAG "ws2812"
#include "luat_log.h"

#include "luat_ws2812.h"

/*Create context
@api ws2812.create(mode, count, id)
@int transmission mode, such as ws2812.GPIO, ws2812.PWM, ws2812.SPI
@int Total number of RGB lights
@int main parameter id, has different values   for different modes
@return userdata Returns the context if the creation is successful, otherwise returns nil
@usage
--GPIO mode, 64 lights, using GPIO9
local leds = ws2812.create(ws2812.GPIO, 64, 9)
-- SPI mode, 32 lights, using SPI1
local leds = ws2812.create(ws2812.SPI, 32, 1)
--PWM mode, 16 lights, using PWM4
local leds = ws2812.create(ws2812.PWM, 16, 4)
--HW mode, 64 lights, using dedicated hardware implementation, the specific ID needs to be checked in the manual
local leds = ws2812.create(ws2812.RMT, 64, 2)

-- Note: Not all Moduless support all the above modes
-- Moreover, the firmware needs to enable the corresponding GPIO/SPI/PWM function to use the corresponding mode.*/
static int l_ws2812_create(lua_State* L) {
    int mode = luaL_checkinteger(L, 1);
    int count = luaL_checkinteger(L, 2);
    int id = luaL_checkinteger(L, 3);

    if (mode < LUAT_WS2812_MODE_GPIO || mode > LUAT_WS2812_MODE_HW) {
        LLOGE("Unknown mode %d", mode);
        return 0;
    }
    if (count <= 0 || count >= 4 * 1024) {
        LLOGE("The number of lights is illegal %d", count);
        return 0;
    }
    size_t len = sizeof(luat_ws2812_t) + sizeof(luat_ws2812_color_t) * count;
    luat_ws2812_t* ctx = lua_newuserdata(L, len);
    if (ctx == NULL) {
        LLOGE("out of memory when malloc ws2812 ctx");
        return 0;
    }
    memset(ctx, 0, len);
    ctx->id = id;
    ctx->count = count;
    ctx->mode = mode;
    return 1;
}

/*Set the color of the light
@api ws2812.set(leds,index, R, G, B)
@userdata context obtained through ws2812.create
@int The number of the lamp, starting from 0
@int R value in RGB value
@int G value in RGB value
@int B value in RGB value
@return boolean Returns true if the setting is successful, otherwise returns nil
@usage
-- RGB color-by-color transfer
ws2812.set(leds, 5, 0xFF, 0xAA, 0x11)
-- It also supports passing one parameter, which is equivalent to the previous one.
ws2812.set(leds, 5, 0xFFAA11)*/
static int l_ws2812_set(lua_State* L) {
    luat_ws2812_t* ctx = lua_touserdata(L, 1);
    int offset = luaL_checkinteger(L, 2);
    if (offset < 0 || offset >= ctx->count) {
        LLOGE("The lamp serial number is out of bounds!!! %d %d", offset, ctx->count);
        return 0;
    }
    if (lua_isinteger(L, 3) && lua_isinteger(L, 4) && lua_isinteger(L, 5)) {
        ctx->colors[offset].R = luaL_checkinteger(L, 3);
        ctx->colors[offset].G = luaL_checkinteger(L, 4);
        ctx->colors[offset].B = luaL_checkinteger(L, 5);
    }
    else if (lua_isinteger(L, 3)) {
        lua_Integer val = luaL_checkinteger(L, 3);
        ctx->colors[offset].R = (val >> 16) & 0xFF;
        ctx->colors[offset].G = (val >> 8) & 0xFF;
        ctx->colors[offset].B = (val >> 0) & 0xFF;
    }
    else {
        LLOGE("Invalid RGB parameter");
        return 0;
    }
    lua_pushboolean(L, 1);
    return 0;
}

/*Send data to device
@api ws2812.send(leds)
@userdata context obtained through ws2812.create
@return boolean Returns true if the setting is successful, otherwise returns nil
@usage
-- No more parameters, just send it and it’s done
ws2812.send(leds)*/
static int l_ws2812_send(lua_State* L) {
    luat_ws2812_t* ctx = lua_touserdata(L, 1);
    int ret = luat_ws2812_send(ctx);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 0;
}

/*Configure additional parameters
@api ws2812.args(leds, arg0, arg1, arg2, arg3, arg4)
@userdata context obtained through ws2812.create
@int extra parameter 0
@int extra parameter 1
@int extra parameter 2
@int extra parameter 3
@int extra parameters 4
@return boolean Returns true if the setting is successful, otherwise returns nil
@usage
-- This function is related to the specific mode

--GPIO mode can adjust the specific delays of T0H T0L, T1H T1L
ws2812.send(leds, t0h, t0l, t1h, t1l)*/
static int l_ws2812_args(lua_State* L) {
    luat_ws2812_t* ctx = lua_touserdata(L, 1);
    int c = lua_gettop(L);
    if (c > 1) {
        for (size_t i = 1; i < c && i < 8; i++)
        {
            ctx->args[i - 1] = lua_tointeger(L, i + 1);
        }
    }
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_ws2812[] =
{
    { "create",           	ROREG_FUNC(l_ws2812_create)},
    { "set",           	    ROREG_FUNC(l_ws2812_set)},
    { "args",               ROREG_FUNC(l_ws2812_args)},
    { "send",           	ROREG_FUNC(l_ws2812_send)},

    { "GPIO",               ROREG_INT(LUAT_WS2812_MODE_GPIO)},
    { "SPI",                ROREG_INT(LUAT_WS2812_MODE_SPI)},
    { "PWM",                ROREG_INT(LUAT_WS2812_MODE_PWM)},
    { "RMT",                ROREG_INT(LUAT_WS2812_MODE_RMT)},
    { "HW",                 ROREG_INT(LUAT_WS2812_MODE_HW)},
	{ NULL,                 ROREG_INT(0)}
};

LUAMOD_API int luaopen_ws2812( lua_State *L ) {
    luat_newlib2(L, reg_ws2812);
    return 1;
}
