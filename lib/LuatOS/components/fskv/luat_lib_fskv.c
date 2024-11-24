
/*@Modules fskv
@summary kv database, no data loss after power outage
@version 1.0
@date 2022.12.29
@demo fskv
@tag LUAT_USE_FSKV
@usage
-- The goal of this library is to replace the fdb library
-- 1. Functions compatible with fdb
-- 2. Use the flash space of fdb, which will also replace the fdb library when enabled.
-- 3. Functionally similar to EEPROM
fskv.init()
fskv.set("wendal", 1234)
log.info("fskv", "wendal", fskv.get("wendal"))

--[[
Differences caused by the implementation mechanisms of fskv and fdb

                    fskv fdb
1. value length 4096 255
2. Key length 63 64
3. Space utilization (comparison) lower higher
4. Reading speed is constant. Dirty data affects the speed and is not constant.
5. Writing data is constant. Dirty data affects the speed and is not constant.
6. Balanced Erase Automatic Automatic
]]*/

#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_mem.h"

#include "luat_fskv.h"
#include "luat_sfd.h"

#ifndef LUAT_LOG_TAG
#define LUAT_LOG_TAG "fskv"
#include "luat_log.h"
#endif

#define LUAT_FSKV_MAX_SIZE (4096)

#ifndef LUAT_CONF_FSKV_CUSTOM
extern sfd_drv_t* sfd_onchip;
extern luat_sfd_lfs_t* sfd_lfs;
#endif
static int fskv_inited;

// static char fskv_read_buff[LUAT_FSKV_MAX_SIZE];

/**
Initialize kv database
@api fskv.init()
@return boolean returns true if successful, otherwise returns false
@usage
if fskv.init() then
    log.info("fdb", "kv database initialization successful")
end

-- About clearing the fdb library
-- The download tool does not provide a direct way to clear fdb data, but there is a way to solve it.
--Write a main.lua, execute fskv.kvdb_init and then execute fskv.clear() to clear all fdb data.*/
static int l_fskvdb_init(lua_State *L) {
    if (fskv_inited == 0) {
#ifndef LUAT_CONF_FSKV_CUSTOM
        if (sfd_onchip == NULL) {
            luat_sfd_onchip_init();
        }
        if (sfd_onchip == NULL) {
            LLOGE("sfd-onchip init failed");
            return 0;
        }
        if (sfd_lfs == NULL) {
            luat_sfd_lfs_init(sfd_onchip);
        }
        if (sfd_lfs == NULL) {
            LLOGE("sfd-onchip lfs int failed");
            return 0;
        }
        fskv_inited = 1;
#else
        fskv_inited = luat_fskv_init() == 0;
#endif
    }
    lua_pushboolean(L, fskv_inited);
    return 1;
}

/**
Set a pair of kv data
@api fskv.set(key, value)
@string The name of the key, required, cannot be an empty string
@string user data, required, cannot be nil, supports string/numeric/table/boolean values, the maximum data length is 4095 bytes
@return boolean returns true if successful, otherwise returns false
@usage
--Set data, string, numerical value, table, Boolean value, any
-- But it cannot be nil, function, userdata, task
log.info("fdb", fskv.set("wendal", "goodgoodstudy"))
log.info("fdb", fskv.set("upgrade", true))
log.info("fdb", fskv.set("timer", 1))
log.info("fdb", fskv.set("bigd", {name="wendal",age=123}))*/
static int l_fskv_set(lua_State *L) {
    if (fskv_inited == 0) {
        LLOGE("call fskv.init() first!!!");
        return 0;
    }
    size_t len;
    luaL_Buffer buff;
    luaL_buffinit(L, &buff);
    const char* key = luaL_checkstring(L, 1);
    //luaL_addchar(&buff, 0xA5);
    int type = lua_type(L, 2);
    switch (type)
    {
    case LUA_TBOOLEAN:
        luaL_addchar(&buff, LUA_TBOOLEAN);
        bool val = lua_toboolean(L, 2);
        luaL_addlstring(&buff, (const char*)&val, sizeof(val));
        break;
    case LUA_TNUMBER:
        if (lua_isinteger(L, 2)) {
            luaL_addchar(&buff, LUA_TINTEGER); // Custom type
            lua_Integer val = luaL_checkinteger(L, 2);
            luaL_addlstring(&buff, (const char*)&val, sizeof(val));
        }
        else {
            luaL_addchar(&buff, LUA_TNUMBER);
            lua_getglobal(L, "pack");
            if (lua_isnil(L, -1)) {
                LLOGW("float number need pack lib");
                lua_pushboolean(L, 0);
                return 1;
            }
            lua_getfield(L, -1, "pack");
            lua_pushstring(L, ">f");
            lua_pushvalue(L, 2);
            lua_call(L, 2, 1);
            if (lua_isstring(L, -1)) {
                const char* val = luaL_checklstring(L, -1, &len);
                luaL_addlstring(&buff, val, len);
            }
            else {
                LLOGW("kdb store number fail!!");
                lua_pushboolean(L, 0);
                return 1;
            }
        }
        break;
    case LUA_TSTRING:
    {
        luaL_addchar(&buff, LUA_TSTRING);
        const char* val = luaL_checklstring(L, 2, &len);
        luaL_addlstring(&buff, val, len);
        break;
    }
    case LUA_TTABLE:
    {
        lua_settop(L, 2);
        lua_getglobal(L, "json");
        if (lua_isnil(L, -1)) {
            LLOGW("miss json lib, not support table value");
            lua_pushboolean(L, 0);
            return 1;
        }
        lua_getfield(L, -1, "encode");
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, 2);
            lua_call(L, 1, 1);
            if (lua_isstring(L, -1)) {
                luaL_addchar(&buff, LUA_TTABLE);
                const char* val = luaL_checklstring(L, -1, &len);
                luaL_addlstring(&buff, val, len);
            }
            else {
                LLOGW("json.encode(val) report error");
                lua_pushboolean(L, 0);
                return 1;
            }
        }
        else {
            LLOGW("miss json.encode, not support table value");
            lua_pushboolean(L, 0);
            return 1;
        }
        break;
    }
    default:
    {
        LLOGW("function/userdata/nil/thread isn't allow");
        lua_pushboolean(L, 0);
        return 1;
    }
    }
    if (buff.n > LUAT_FSKV_MAX_SIZE) {
        LLOGE("value too big %d max %d", buff.n, LUAT_FSKV_MAX_SIZE);
        lua_pushboolean(L, 0);
        return 1;
    }
    int ret = luat_fskv_set(key, buff.b, buff.n);
    lua_pushboolean(L, ret == buff.n ? 1 : 0);
    // lua_pushinteger(L, ret);
    return 1;
}

/**
Set the key-value pair data in the table
@api fskv.sett(key, skey, value)
@string The name of the key, required, cannot be an empty string
@string table key name, required, cannot be an empty string
@string user data, required, supports string/numeric/table/boolean values, the maximum data length is 4095 bytes
@return boolean returns true if successful, otherwise returns false/nil
@usage
-- This API was added on 2023.7.26, please note the difference from the set function
--Set data, string, numerical value, table, Boolean value, any
-- But it cannot be function, userdata, task
log.info("fdb", fskv.sett("mytable", "wendal", "goodgoodstudy"))
log.info("fdb", fskv.sett("mytable", "upgrade", true))
log.info("fdb", fskv.sett("mytable", "timer", 1))
log.info("fdb", fskv.sett("mytable", "bigd", {name="wendal",age=123}))

-- The following statement will print out a table of 4 elements
log.info("fdb", fskv.get("mytable"), json.encode(fskv.get("mytable")))
-- Note: If the key does not exist, or the original value is not of table type, it will be completely overwritten.
-- For example, if you write the following method, you will get the table instead of the string in the first line.
log.info("fdb", fskv.set("mykv", "123"))
log.info("fdb", fskv.sett("mykv", "age", "123")) -- what will be saved will be {age:"123"}


-- If the set data is filled with nil, it means deleting the corresponding key.
log.info("fdb", fskv.sett("mykv", "name", "wendal"))
log.info("fdb", fskv.sett("mykv", "name")) -- equivalent to deletion
--*/
static int l_fskv_sett(lua_State *L) {
    if (fskv_inited == 0) {
        LLOGE("call fskv.init() first!!!");
        return 0;
    }
    const char* key = luaL_checkstring(L, 1);
    const char* skey = luaL_checkstring(L, 2);
    if (lua_gettop(L) < 3) {
        LLOGD("require key skey value");
        return 0;
    }
    char tmp[256] = {0};
    char *buff = NULL;
    char *rbuff = NULL;
    int size = luat_fskv_size(key, tmp);
    if (size >= 256) {
        rbuff = luat_heap_malloc(size);
        if (rbuff == NULL) {
            LLOGW("out of memory when malloc key-value buff");
            return 0;
        }
        size_t read_len = luat_fskv_get(key, rbuff, size);
        if (read_len != size) {
            luat_heap_free(rbuff);
            LLOGW("read key-value fail, ignore as not exist");
            return 0;
        }
        buff = rbuff;
    }
    else {
        buff = tmp;
    }
    if (buff[0] == LUA_TTABLE) {
        lua_getglobal(L, "json");
        lua_getfield(L, -1, "decode");
        lua_pushlstring(L, (const char*)(buff + 1), size - 1);
        lua_call(L, 1, 1);
        if (lua_type(L, -1) != LUA_TTABLE) {
            lua_pop(L, 1);
            lua_newtable(L);
        }
    }
    else {
        lua_newtable(L);
    }
    if (rbuff) {
        luat_heap_free(rbuff);
        rbuff = NULL;
    }
    lua_pushvalue(L, 3);
    lua_setfield(L, -2, skey);
    lua_pushcfunction(L, l_fskv_set);
    lua_pushvalue(L, 1);
    lua_pushvalue(L, -3);
    lua_call(L, 2, 1);
    return 1;
}

/**
Get the corresponding data according to the key
@api fskv.get(key, skey)
@string The name of the key, required, cannot be an empty string
@string Optional secondary key, only valid when the original value is table, equivalent to fskv.get(key)[skey]
@return any data is returned if it exists, otherwise nil is returned
@usage
if fskv.init() then
    log.info("fdb", fskv.get("wendal"))
end

-- If you need "default value", which corresponds to a non-bool Boolean value, you can write it like this
local v = fskv.get("wendal") or "123"*/
static int l_fskv_get(lua_State *L) {
    if (fskv_inited == 0) {
        LLOGE("call fskv.init() first!!!");
        return 0;
    }
    // luaL_Buffer buff;
    const char* key = luaL_checkstring(L, 1);
    const char* skey = luaL_optstring(L, 2, "");
    // luaL_buffinitsize(L, &buff, 8192);
    char tmp[256] = {0};
    char *buff = NULL;
    char *rbuff = NULL;
    int size = luat_fskv_size(key, tmp);
    if (size < 2) {
        return 0; //The corresponding KEY does not exist
    }
    if (size >= 256) {
        rbuff = luat_heap_malloc(size);
        if (rbuff == NULL) {
            LLOGW("out of memory when malloc key-value buff");
            return 0;
        }
        size_t read_len = luat_fskv_get(key, rbuff, size);
        if (read_len != size) {
            luat_heap_free(rbuff);
            LLOGW("read key-value fail, ignore as not exist");
            return 0;
        }
        buff = rbuff;
    }
    else {
        buff = tmp;
    }

    lua_Integer intVal;
    // lua_Number *numVal;
    // LLOGD("KV value T=%02X", buff.b[0]);
    switch(buff[0]) {
    case LUA_TBOOLEAN:
        lua_pushboolean(L, buff[1]);
        break;
    case LUA_TNUMBER:
        lua_getglobal(L, "pack");
        lua_getfield(L, -1, "unpack");
        lua_pushlstring(L, (char*)(buff + 1), size - 1);
        lua_pushstring(L, ">f");
        lua_call(L, 2, 2);
        // _, val = pack.unpack(data, ">f")
        break;
    case LUA_TINTEGER:
    	//Cannot assign value directly, the pointer address on the right is inconsistent with the bit width on the left
    	memcpy(&intVal, &buff[1], sizeof(lua_Integer));
//        intVal = (lua_Integer*)(&buff[1]);
//        lua_pushinteger(L, *intVal);
        lua_pushinteger(L, intVal);
        break;
    case LUA_TSTRING:
        lua_pushlstring(L, (const char*)(buff + 1), size - 1);
        break;
    case LUA_TTABLE:
        lua_getglobal(L, "json");
        lua_getfield(L, -1, "decode");
        lua_pushlstring(L, (const char*)(buff + 1), size - 1);
        lua_call(L, 1, 1);
        if (strlen(skey) > 0 && lua_istable(L, -1)) {
            lua_getfield(L, -1, skey);
        }
        break;
    default :
        LLOGW("bad value prefix %02X", buff[0]);
        lua_pushnil(L);
        break;
    }
    if (rbuff)
        luat_heap_free(rbuff);
    return 1;
}

/**
Delete data based on key
@api fskv.del(key)
@string The name of the key, required, cannot be an empty string
@return bool returns true if successful, otherwise returns false
@usage
log.info("fdb", fskv.del("wendal"))*/
static int l_fskv_del(lua_State *L) {
    if (fskv_inited == 0) {
        LLOGE("call fskv.init() first!!!");
        return 0;
    }
    const char* key = luaL_checkstring(L, 1);
    if (key == NULL) {
        lua_pushboolean(L, 0);
        return 1;
    }
    int ret = luat_fskv_del(key);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/**
Clear the entire kv database
@api fskv.clear()
@return bool returns true if successful, otherwise returns false
@usage
-- Clear
fskv.clear()*/
static int l_fskv_clr(lua_State *L) {
    if (fskv_inited == 0) {
        LLOGE("call fskv.init() first!!!");
        return 0;
    }
    int ret = luat_fskv_clear();
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}


/**
kv database iterator
@api fskv.iter()
@return userdata returns the iterator pointer successfully, otherwise returns nil
@usage
-- Clear
local iter = fskv.iter()
if iter then
    while 1 do
        local k = fskv.next(iter)
        if not k then
            break
        end
        log.info("fdb", k, "value", fskv.kv_get(k))
    end
end*/
static int l_fskv_iter(lua_State *L) {
    if (fskv_inited == 0) {
        LLOGE("call fskv.init() first!!!");
        return 0;
    }
    size_t *offset = lua_newuserdata(L, sizeof(size_t));
    memset(offset, 0, sizeof(size_t));
    return 1;
}

/**
kv iterator gets the next key
@api fskv.next(iter)
@userdata pointer returned by fskv.iter()
@return string Returns the string key value successfully, otherwise returns nil
@usage
-- Clear
local iter = fskv.iter()
if iter then
    while 1 do
        local k = fskv.next(iter)
        if not k then
            break
        end
        log.info("fskv", k, "value", fskv.get(k))
    end
end*/
static int l_fskv_next(lua_State *L) {
    size_t *offset = lua_touserdata(L, 1);
    char buff[256] = {0};
    int ret = luat_fskv_next(buff, *offset);
    // LLOGD("fskv.next %d %d", *offset, ret);
    if (ret == 0) {
        lua_pushstring(L, buff);
        *offset = *offset + 1;
        return 1;
    }
    return 0;
}

/*Get kv database status
@api fskv.status()
@return int used space, unit bytes
@return int total available space, in bytes
@return int Total number of kv key-value pairs, unit
@usage
local used, total,kv_count = fskv.status()
log.info("fdb", "kv", used,total,kv_count)*/
static int l_fskv_stat(lua_State *L) {
    size_t using_sz = 0;
    size_t max_sz = 0;
    size_t kv_count = 0;
    if (fskv_inited == 0) {
        LLOGE("call fskv.init() first!!!");
        return 0;
    }
    luat_fskv_stat(&using_sz, &max_sz, &kv_count);
    lua_pushinteger(L, using_sz);
    lua_pushinteger(L, max_sz);
    lua_pushinteger(L, kv_count);
    return 3;
}

#include "rotable2.h"
static const rotable_Reg_t reg_fskv[] =
{
    { "init" ,              ROREG_FUNC(l_fskvdb_init)},
    { "set",                ROREG_FUNC(l_fskv_set)},
    { "get",                ROREG_FUNC(l_fskv_get)},
    { "del",                ROREG_FUNC(l_fskv_del)},
    { "clr",                ROREG_FUNC(l_fskv_clr)},
    { "clear",              ROREG_FUNC(l_fskv_clr)},
    { "stat",               ROREG_FUNC(l_fskv_stat)},
    { "status",             ROREG_FUNC(l_fskv_stat)},
    { "iter",               ROREG_FUNC(l_fskv_iter)},
    { "next",               ROREG_FUNC(l_fskv_next)},
    { "sett",               ROREG_FUNC(l_fskv_sett)},

    // -- Provide an API compatible with fdb
    { "kvdb_init" ,         ROREG_FUNC(l_fskvdb_init)},
    { "kv_set",             ROREG_FUNC(l_fskv_set)},
    { "kv_get",             ROREG_FUNC(l_fskv_get)},
    { "kv_del",             ROREG_FUNC(l_fskv_del)},
    { "kv_clr",             ROREG_FUNC(l_fskv_clr)},
    { "kv_stat",            ROREG_FUNC(l_fskv_stat)},
    { "kv_iter",            ROREG_FUNC(l_fskv_iter)},
    { "kv_next",            ROREG_FUNC(l_fskv_next)},
    { NULL,                 ROREG_INT(0)}
};

LUAMOD_API int luaopen_fskv( lua_State *L ) {
    luat_newlib2(L, reg_fskv);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "fdb");
    return 1;
}
