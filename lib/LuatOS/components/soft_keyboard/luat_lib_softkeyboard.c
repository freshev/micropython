
/*@Modules softkb
@summary software keyboard matrix
@version 1.0
@date 2022.03.09
@tag LUAT_USE_SOFTKB*/

#include "luat_base.h"
#include "luat_softkeyboard.h"
#include "luat_msgbus.h"

#define MAX_DEVICE_COUNT 2
static luat_softkeyboard_conf_t softkb_conf[MAX_DEVICE_COUNT];

int l_softkeyboard_handler(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
/*@sys_pub softkeyboard
Software keyboard matrix messages
SOFT_KB_INC
@number port, keyboard id is currently fixed at 0 and can be ignored
@number data, keyboard keys need to be parsed with the init map
@number state, key state 1 is pressed, 0 is released
@usage
sys.subscribe("SOFT_KB_INC", function(port, data, state)
    --port is currently fixed at 0 and can be ignored
    -- data, needs to be parsed with the init map
    -- state, 1 means pressed, 0 means released
    log.info("keyboard", port, data, state)
end)*/
    lua_pushstring(L, "SOFT_KB_INC");
    lua_pushinteger(L, msg->arg1);
    lua_pushinteger(L, msg->arg2);
    lua_pushinteger(L, msg->ptr);
    lua_call(L, 4, 0);
    return 0;
}

/**
Initialize software keyboard matrix
@api softkb.init(port, key_in, key_out)
@int reserved, currently filled with 0
@table matrix input key table
@table matrix output key table
@usage
    key_in = {pin.PD10,pin.PE00,pin.PE01,pin.PE02}
    key_out = {pin.PD12,pin.PD13,pin.PD14,pin.PD15}
    softkb.init(0,key_in,key_out)

sys.subscribe("SOFT_KB_INC", function(port, data, state)
    --port is currently fixed at 0 and can be ignored
    -- data, needs to be parsed with the init map
    -- state, 1 means pressed, 0 means released
    -- TODO detailed introduction
end)*/
int l_softkb_init(lua_State* L) {
    uint8_t softkb_port = luaL_checkinteger(L,1);
    softkb_conf[softkb_port].port = softkb_port;
    if (lua_istable(L, 2)) {
        softkb_conf[softkb_port].inio_num = lua_rawlen(L, 2);
        softkb_conf[softkb_port].inio = (uint8_t*)luat_heap_calloc(softkb_conf[softkb_port].inio_num,sizeof(uint8_t));
        for (size_t i = 0; i < softkb_conf[softkb_port].inio_num; i++){
            lua_geti(L,2,i+1);
            softkb_conf[softkb_port].inio[i] = luaL_checkinteger(L,-1);
            lua_pop(L, 1);
        }
    }
    if (lua_istable(L, 3)) {
        softkb_conf[softkb_port].outio_num = lua_rawlen(L, 3);
        softkb_conf[softkb_port].outio = (uint8_t*)luat_heap_calloc(softkb_conf[softkb_port].outio_num,sizeof(uint8_t));
        for (size_t i = 0; i < softkb_conf[softkb_port].outio_num; i++){
            lua_geti(L,3,i+1);
            softkb_conf[softkb_port].outio[i] = luaL_checkinteger(L,-1);
            lua_pop(L, 1);
        }
    }
    int ret = luat_softkeyboard_init(&softkb_conf[softkb_port]);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/**
Delete software keyboard matrix
@api softkb.deinit(port)
@int reserved, currently filled with 0
@usage
    softkb.deinit(0)*/
int l_softkb_deinit(lua_State* L) {
    luat_softkeyboard_conf_t conf = {0};
    uint8_t softkb_port = luaL_checkinteger(L,1);
    int ret = luat_softkeyboard_deinit(&softkb_conf[softkb_port]);
    luat_heap_free(softkb_conf[softkb_port].inio);
    luat_heap_free(softkb_conf[softkb_port].outio);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}


#include "rotable2.h"
static const rotable_Reg_t reg_softkb[] =
{
    { "init",          ROREG_FUNC(l_softkb_init)},
    { "deinit",        ROREG_FUNC(l_softkb_deinit)},
	{ NULL,            {}}
};

LUAMOD_API int luaopen_softkb( lua_State *L ) {
    luat_newlib2(L, reg_softkb);
    return 1;
}
