/*@Modules wlanraw
@summary WLAN data RAW transmission
@version 1.0
@date 2024.4.11
@tag LUAT_USE_WLAN_RAW
@usage
-- Please check https://github.com/wendal/xt804-spinet*/
#include "luat_base.h"
#include "rotable2.h"
#include "luat_zbuff.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_wlan_raw.h"
#include "luat_zbuff.h"
#include "luat_mem.h"

#define LUAT_LOG_TAG "wlan.raw"
#include "luat_log.h"

// store callback function
static int lua_cb_ref;
#define RAW_BUFF_COUNT 8
#define RAW_BUFF_SIZE  1600

static luat_wlan_raw_data_t datas[RAW_BUFF_COUNT];

/*Initialize the RAW layer of WLAN
@api wlanraw.setup(opts, cb)
@table opts configuration parameters
@function callback function, form function(buff, size)
@return boolean true indicates success, others fail
@usage
-- Currently only supported by XT804 series, such as Air101/Air103/Air601/Air690
wlanraw.setup({
    buffsize = 1600, -- buffer size, default 1600 bytes
    buffcount = 10, -- number of buffers, default 8
}, cb)*/
static int l_wlan_raw_setup(lua_State *L) {
    if (datas[0].buff) {
        return 0;
    }
    size_t len = RAW_BUFF_SIZE;
    size_t count = RAW_BUFF_COUNT;
    lua_newtable(L);
    for (size_t i = 0; i < count; i++)
    {
        luat_zbuff_t *buff = (luat_zbuff_t *)lua_newuserdata(L, sizeof(luat_zbuff_t));
        luaL_setmetatable(L, LUAT_ZBUFF_TYPE);
        lua_pushvalue(L, -1);
        memset(buff, 0, sizeof(luat_zbuff_t));
        buff->type = LUAT_HEAP_PSRAM;
        buff->addr = (uint8_t *)luat_heap_opt_malloc(buff->type, len);
        buff->len = len;
        datas[i].zbuff_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        datas[i].zbuff_used = &buff->used;
        datas[i].buff = buff->addr;
        lua_seti(L, -2, i + 1);
        // LLOGD("zbuff_ref array index%d lua index%d", i, datas[i].zbuff_ref);
    }
    lua_pushvalue(L, 2);
    lua_cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    luat_wlan_raw_conf_t conf = {0};
    luat_wlan_raw_setup(&conf);
    lua_pushboolean(L, 1);
    lua_pushvalue(L, 3);
    return 1;
}

static int l_wlan_raw_write(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    luat_zbuff_t * buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
    int offset = luaL_optinteger(L, 3, 0);
    int len = luaL_optinteger(L, 4, buff->used - offset);
    int ret = luat_wlan_raw_write(id, buff->addr + offset, len);
    lua_pushboolean(L, ret == 0);
    lua_pushinteger(L, ret);
    return 2;
}

static const rotable_Reg_t reg_wlan_raw[] = {
    { "setup",              ROREG_FUNC(l_wlan_raw_setup)},
    { "write",              ROREG_FUNC(l_wlan_raw_write)},
    { NULL,                 ROREG_INT(0)}
};

LUAMOD_API int luaopen_wlan_raw(lua_State *L)
{
    luat_newlib2(L, reg_wlan_raw);
    return 1;
}

static int l_wlan_raw_event_handler(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    if (!lua_cb_ref) {
        return 0; // No callback function is set, exit directly
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, lua_cb_ref);
    if (!lua_isfunction(L, -1)) {
        return 0; // No callback function is set, exit directly
    }
    lua_pushinteger(L, msg->arg1);
    // LLOGD("Remove zbuff lua index %d", msg->arg2);
    lua_rawgeti(L, LUA_REGISTRYINDEX, msg->arg2);
    lua_call(L, 2, 0);
    return 0;
}

int l_wlan_raw_event(int tp, void* buff, size_t len) {
    rtos_msg_t msg = {
        .handler = l_wlan_raw_event_handler,
        .arg1 = tp,
        .arg2 = 0,
        .ptr = NULL,
    };
    if (len > RAW_BUFF_SIZE) {
        LLOGE("Type %d data length %d exceeds buffer size %d", tp, len, RAW_BUFF_SIZE);
        return -2;
    }
    if (!lua_cb_ref) {
        LLOGW("Not initialized, ignore data %d %p %d", tp, buff, len);
        return -3;
    }
    for (size_t i = 0; i < RAW_BUFF_COUNT; i++)
    {
        if ((*datas[i].zbuff_used) == 0) {
            // LLOGD("Use zbuff to transfer array index %d write length %d", i, len);
            memcpy(datas[i].buff, buff, len);
            // datas[i].used = 1;
            *(datas[i].zbuff_used) = len;
            msg.arg2 = datas[i].zbuff_ref;
            luat_msgbus_put(&msg, 0);
            return 0;
        }
    }
    LLOGE("No zbuff available");
    return -1;
}
