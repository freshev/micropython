/*@Modules  httpsrv
@summary http服务端
@version 1.0
@date    2022.010.15
@demo wlan
@tag LUAT_USE_HTTPSRV*/

#include "luat_base.h"
#include "luat_httpsrv.h"

#define LUAT_LOG_TAG "httpsrv"
#include "luat_log.h"

/*Start and listen to an http port
@api httpsrv.start(port, func)
@int port number
@function callback function
@return bool returns true if successful, otherwise returns false
@usage

-- Listen on port 80
httpsrv.start(80, function(client, method, uri, headers, body)
    -- method is a string, such as GET POST PUT DELETE
    -- uri is also a string, for example / /api/abc
    -- headers table type
    -- body string
    log.info("httpsrv", method, uri, json.encode(headers), body)
    if uri == "/led/1" then
        LEDA(1)
        return 200, {}, "ok"
    elseif uri == "/led/0" then
        LEDA(0)
        return 200, {}, "ok"
    end
    --Conventions for return values   code, headers, body
    -- If there is no return value, the default is 404, {}, ""
    return 404, {}, "Not Found" .. uri
end)
-- About static files
-- Case 1: / , mapped to /index.html
-- Case 2: /abc.html, first search for /abc.html, if it does not exist, search for /abc.html.gz
-- If gz exists, it will automatically respond with a compressed file, which is supported by most browsers.
-- Currently, the default search is for files under /luadb/xxx, which is not configurable yet.*/
static int l_httpsrv_start(lua_State *L) {
    int port = luaL_checkinteger(L, 1);
    if (!lua_isfunction(L, 2)) {
        LLOGW("httpsrv need callback function!!!");
        return 0;
    }
    lua_pushvalue(L, 2);
    int lua_ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
    luat_httpsrv_ctx_t ctx = {
        .port = port,
        .lua_ref_id = lua_ref_id
    };
    int ret = luat_httpsrv_start(&ctx);
    if (ret == 0) {
        LLOGI("http listen at 0.0.0.0:%d", ctx.port);
    }
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Stop http service
@api httpsrv.stop(port)
@int port number
@return nil currently has no return value*/
static int l_httpsrv_stop(lua_State *L) {
    int port = luaL_checkinteger(L, 1);
    luat_httpsrv_stop(port);
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_httpsrv[] =
{
    {"start",        ROREG_FUNC(l_httpsrv_start) },
    {"stop",         ROREG_FUNC(l_httpsrv_stop) },
	{ NULL,          ROREG_INT(0) }
};

LUAMOD_API int luaopen_httpsrv( lua_State *L ) {
    luat_newlib2(L, reg_httpsrv);
    return 1;
}
