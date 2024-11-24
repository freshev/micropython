/*@Modules spislave
@summary SPI slave
@version 1.0
@date 2024.3.27
@demo spislave
@tag LUAT_USE_SPI_SLAVE
@usage
-- Please check the demo
-- Currently only supported by XT804 series, such as Air101/Air103/Air601*/
#include "luat_base.h"
#include "rotable2.h"
#include "luat_zbuff.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_spi_slave.h"

#define LUAT_LOG_TAG "spislave"
#include "luat_log.h"

// store callback function
static int lua_cb_ref;

/*Initialize SPI slave
@api spislave.setup(id, opts)
@int The SPI number of the slave. Please note the difference with the number of the SPI master. This is related to the specific device.
@table opts extended configuration parameters, currently no parameters
@return boolean true indicates success, others fail
@usage
-- Currently only supported by XT804 series, such as Air101/Air103/Air601/Air690
-- Air101 as an example, initialize SPI slave, numbered 2, SPI mode
spislave.setup(2)
-- Air101 as an example, initialize SPI slave, numbered 3, SDIO mode
spislavve.setup(3)*/
static int l_spi_slave_setup(lua_State *L) {
    luat_spi_slave_conf_t conf = {
        .id = luaL_checkinteger(L, 1)
    };
    int ret = luat_spi_slave_open(&conf);
    if (ret) {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, ret);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/*Is it writable?
@api spislave.ready(id)
@int slave SPI number
@return boolean true means writable, others are not writable*/
static int l_spi_slave_ready(lua_State *L) {
    luat_spi_slave_conf_t conf = {
        .id = luaL_checkinteger(L, 1)
    };
    int ret = luat_spi_slave_writable(&conf);
    if (ret) {
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}

/*Register event callback function
@api spislave.on(id, cb)
@int slave SPI number
@function callback function*/
static int l_spi_slave_on(lua_State *L) {
    if (lua_cb_ref) {
        luaL_unref(L, LUA_REGISTRYINDEX, lua_cb_ref);
        lua_cb_ref = 0;
    }
    if (lua_isfunction(L, 2)) {
        lua_pushvalue(L, 2);
        lua_cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return 0;
}

/*Read data
@api spislave.read(id, ptr, buff, len)
@int slave SPI number
@userdata user data pointer, obtained from the callback function
@int zbuff buffer object
@int reads the length, obtained from the callback function
@return int The number of bytes read, usually the same as the length expected to be read
@return int error code, returned only when an error occurs*/
static int l_spi_slave_read(lua_State *L) {
    luat_spi_slave_conf_t conf = {
        .id = luaL_checkinteger(L, 1)
    };
    void* ptr = lua_touserdata(L, 2);
    luat_zbuff_t* zbuf = (luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE);
    int len = luaL_checkinteger(L, 4);
    int ret = luat_spi_slave_read(&conf, ptr, zbuf->addr, len);
    if (ret >= 0 ){
        lua_pushinteger(L, ret);
        return 1;
    }
    lua_pushinteger(L, 0);
    lua_pushinteger(L, ret);
    return 2;
}
/*Write data
@api spislave.write(id, ptr, buff, len)
@int slave SPI number
@userdata user data pointer, currently passed nil
@int zbuff buffer object
@int write length, be careful not to exceed the hardware limit, usually 1500 bytes
@return boolean true indicates success, others fail
@return int error code, returned only when an error occurs*/
static int l_spi_slave_write(lua_State *L) {
    luat_spi_slave_conf_t conf = {
        .id = luaL_checkinteger(L, 1)
    };
    luat_zbuff_t* zbuf = (luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE);
    int len = luaL_checkinteger(L, 4);
    int ret = luat_spi_slave_write(&conf, zbuf->addr, len);
    if (ret == 0 ){
        lua_pushboolean(L, 1);
        return 1;
    }
    lua_pushboolean(L, 0);
    lua_pushinteger(L, ret);
    return 2;
}

static const rotable_Reg_t reg_spi_slave[] = {
    { "setup",              ROREG_FUNC(l_spi_slave_setup)},
    { "ready",              ROREG_FUNC(l_spi_slave_ready)},
    { "read",               ROREG_FUNC(l_spi_slave_read)},
    { "write",              ROREG_FUNC(l_spi_slave_write)},
    { "on",                 ROREG_FUNC(l_spi_slave_on)},
    { NULL,                 ROREG_INT(0)}
};

LUAMOD_API int luaopen_spislave(lua_State *L)
{
    luat_newlib2(L, reg_spi_slave);
    return 1;
}

static int l_spi_slave_event_handler(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    if (!lua_cb_ref) {
        return 0; // No callback function is set, exit directly
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, lua_cb_ref);
    if (!lua_isfunction(L, -1)) {
        return 0; // No callback function is set, exit directly
    }
    lua_pushinteger(L, msg->arg1);
    lua_pushlightuserdata(L, msg->ptr);
    lua_pushinteger(L, msg->arg2);
    lua_call(L, 3, 0);
    return 0;
}

int l_spi_slave_event(int id, int event, void* buff, size_t max_size) {
    rtos_msg_t msg = {
        .handler = l_spi_slave_event_handler,
        .arg1 = id | event,
        .arg2 = max_size,
        .ptr = buff,
    };
    luat_msgbus_put(&msg, 0);
    return 0;
}
