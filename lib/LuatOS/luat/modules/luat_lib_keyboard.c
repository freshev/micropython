
/*@Modules keyboard
@summary keyboard matrix
@version 1.0
@date 2021.11.24
@demo keyboard
@tag LUAT_USE_KEYBOARD*/

#include "luat_base.h"
#include "luat_keyboard.h"
#include "luat_msgbus.h"

//----------------------


static int l_keyboard_handler(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
/*@sys_pub keyboard
keyboard matrix messages
KB_INC
@number port, keyboard id is currently fixed at 0 and can be ignored
@number data, keyboard keys need to be parsed with the init map
@number state, key state 1 is pressed, 0 is released
@usage
sys.subscribe("KB_INC", function(port, data, state)
    --port is currently fixed at 0 and can be ignored
    -- data, needs to be parsed with the init map
    -- state, 1 means pressed, 0 means released
    log.info("keyboard", port, data, state)
end)*/
    lua_pushstring(L, "KB_INC");
    lua_pushinteger(L, msg->arg1);
    lua_pushinteger(L, msg->arg2);
    lua_pushinteger(L, (uint32_t)msg->ptr);
    lua_call(L, 4, 0);
    return 0;
}

static void l_keyboard_irq_cb(luat_keyboard_ctx_t* ctx) {
    rtos_msg_t msg = {0};
    msg.handler = l_keyboard_handler;
    msg.arg1 = ctx->port;
    msg.arg2 = ctx->pin_data;
    msg.ptr = (void*)ctx->state;
    luat_msgbus_put(&msg, 0);
}

//----------------------

/**
Initialize keyboard matrix
@api keyboard.init(port, conf, map, debounce)
@int reserved, currently filled with 0
@int enabled keyboard pin mask, for example, if keyboard0~9 is used, the mask is 0x1FF, if 0~3 is used, the mask is 0xF
@int keyboard pin direction mapping, where input is 0 and output is 1, set bit by bit. For example, keyboard0~3 is used as input and keyboard4~7 is input, then 0xF0
@int Anti-bounce configuration, reserved, can be left blank
@usage
-- Make a 4*4 keyboard matrix, using keyboard0~7, where 0~3 are used for input and 4~7 are used for output.
-- Use keyboard0~7, corresponding conf is 0xFF
-- Among them, 0~3 are used as input, 4~7 are used as output, and the corresponding map is 0xF0
keyboard.init(0, 0xFF, 0xF0)

-- Make a 2*3 keyboard matrix, use keyboard0~4, where 0~1 is used as input and 2~4 is used as output
-- Use keyboard0~4, the binary value is 11111, and the hexadecimal expression corresponding to conf is 0x1F
-- Among them, 0~1 is used as input, 2~4 is used as output, the binary value is 11100, and the corresponding map is 0x14
-- keyboard.init(0, 0xFF, 0x14)

sys.subscribe("KB_INC", function(port, data, state)
    --port is currently fixed at 0 and can be ignored
    -- data, needs to be parsed with the init map
    -- state, 1 means pressed, 0 means released
    -- TODO detailed introduction
end)*/
static int l_keyboard_init(lua_State *L) {
    luat_keyboard_conf_t conf = {0};
    conf.port = luaL_checkinteger(L, 1);
    conf.pin_conf = luaL_checkinteger(L, 2);
    conf.pin_map = luaL_checkinteger(L, 3);
    conf.debounce = luaL_optinteger(L, 4, 1);
    conf.cb = l_keyboard_irq_cb;
    int ret = luat_keyboard_init(&conf);

    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

static int l_keyboard_deinit(lua_State *L) {
    luat_keyboard_conf_t conf = {0};
    conf.port = luaL_checkinteger(L, 1);
    conf.pin_conf = luaL_checkinteger(L, 2);
    conf.pin_map = luaL_checkinteger(L, 3);
    conf.debounce = luaL_optinteger(L, 4, 1);

    int ret = luat_keyboard_deinit(&conf);

    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_keyboard[] =
{
    { "init" ,         ROREG_FUNC(l_keyboard_init)},
    { "deinit" ,       ROREG_FUNC(l_keyboard_deinit)},
	{ NULL,            ROREG_INT(0) }
};

LUAMOD_API int luaopen_keyboard( lua_State *L ) {
    luat_newlib2(L, reg_keyboard);
    return 1;
}
