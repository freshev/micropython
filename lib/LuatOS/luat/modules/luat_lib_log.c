
/*@Modules  log
@summary 日志库
@version 1.0
@date    2020.03.30
@tag LUAT_CONF_BSP*/
#include "luat_base.h"
#include "luat_sys.h"
#include "luat_msgbus.h"
#include "luat_zbuff.h"
#include "ldebug.h"
#include "luat_rtos.h"
#define LUAT_LOG_TAG "log"
#include "luat_log.h"
typedef struct luat_log_conf
{
    uint8_t style;

}luat_log_conf_t;

#define LOG_STYLE_NORMAL        0
#define LOG_STYLE_DEBUG_INFO    1
#define LOG_STYLE_FULL          2

static luat_log_conf_t lconf = {
    .style=0
};

static int add_debug_info(lua_State *L, uint8_t pos, const char* LEVEL) {
    lua_Debug ar;
    // int arg;
    // int d = 0;
    // // Find the depth of the current stack
    // while (lua_getstack(L, d, &ar) != 0) {
    //     d++;
    // }
    // // Defend yourself, it is unlikely that even d==0 will fail.
    // if (d == 0)
    //     return 0;
    // Get the real stack location information
    if (!lua_getstack(L, 1, &ar))
        return 0;
    // S contains the source code, l contains the current line number
    if (0 == lua_getinfo(L, "Sl", &ar))
        return 0;
    // Skip without debugging information
    if (ar.source == NULL)
        return 0;
    int line = ar.currentline > 64*1024 ? 0 : ar.currentline;
    // Push the file name and line number. Note: The first character of the source code path is the identifier and needs to be skipped.
    if (LEVEL)
        lua_pushfstring(L, "%s/%s:%d", LEVEL, ar.source + 1, line);
    else
        lua_pushfstring(L, "%s:%d", ar.source + 1, line);
    if (lua_gettop(L) > pos)
        lua_insert(L, pos);
    return 1;
}

/*Set log level
@api log.setLevel(level)
@string level log level, available string or numerical value, the string is (SILENT, DEBUG, INFO, WARN, ERROR, FATAL), the numerical value is (0,1,2,3,4,5)
@return nil no return value
@usage
--Set the log level to INFO
log.setLevel("INFO")*/
static int l_log_set_level(lua_State *L) {
    int LOG_LEVEL = 0;
    if (lua_isinteger(L, 1)) {
        LOG_LEVEL = lua_tointeger(L, 1);
    }
    else if (lua_isstring(L, 1)) {
        const char* lv = lua_tostring(L, 1);
        if (strcmp("SILENT", lv) == 0) {
            LOG_LEVEL = LUAT_LOG_CLOSE;
        }
        else if (strcmp("DEBUG", lv) == 0) {
            LOG_LEVEL = LUAT_LOG_DEBUG;
        }
        else if (strcmp("INFO", lv) == 0) {
            LOG_LEVEL = LUAT_LOG_INFO;
        }
        else if (strcmp("WARN", lv) == 0) {
            LOG_LEVEL = LUAT_LOG_WARN;
        }
        else if (strcmp("ERROR", lv) == 0) {
            LOG_LEVEL = LUAT_LOG_ERROR;
        }
    }
    if (LOG_LEVEL == 0) {
        LOG_LEVEL = LUAT_LOG_CLOSE;
    }
    luat_log_set_level(LOG_LEVEL);
    return 0;
}

/*Set log style
@api log.style(val)
@int Log style, default is 0, if not passed, the current value will be obtained
@return int current log style
@usage
-- Take log.info("ABC", "DEF", 123) as an example, assuming that the code is located in line 12 of main.lua
--Default log 0
--I/user.ABC DEF 123
--Debug style 1, add additional debugging information
-- I/main.lua:12 ABC DEF 123
--Debugging style 2, add additional debugging information, the location is different
-- I/user.ABC main.lua:12 DEF 123

log.style(0) -- default style 0
log.style(1) -- debugging style 1
log.style(2) -- debugging style 2*/
static int l_log_style(lua_State *L) {
    if (lua_isinteger(L, 1))
        lconf.style = luaL_checkinteger(L, 1);
    lua_pushinteger(L, lconf.style);
    return 1;
}

/*Get log level
@api log.getLevel()
@return int log level corresponds to 0,1,2,3,4,5
@usage
-- Get log level
log.getLevel()*/
int l_log_get_level(lua_State *L) {
    lua_pushinteger(L, luat_log_get_level());
    return 1;
}

static int l_log_2_log(lua_State *L, const char* LEVEL) {
    // Doesn't it send anything?
    int argc = lua_gettop(L);
    if (argc < 1) {
        // Pass at least 1 parameter
        return 0;
    }
    if (lconf.style == LOG_STYLE_NORMAL) {
        lua_pushfstring(L, "%s/user.%s", LEVEL, lua_tostring(L, 1));
        lua_remove(L, 1); // remove tag
        lua_insert(L, 1);
    }
    else if (lconf.style == LOG_STYLE_DEBUG_INFO) {
        add_debug_info(L, 1, LEVEL);
    }
    else if (lconf.style == LOG_STYLE_FULL) {
        lua_pushfstring(L, "%s/user.%s", LEVEL, lua_tostring(L, 1));
        lua_remove(L, 1); // remove tag
        lua_insert(L, 1);
        add_debug_info(L, 2, NULL);
    }
    lua_getglobal(L, "print");
    lua_insert(L, 1);
    lua_call(L, lua_gettop(L) - 1, 0);
    return 0;
}

/*Output log, level debug
@api log.debug(tag, val, val2, val3, ...)
@string tag log ID, must be a string
@... Parameters to be printed
@return nil no return value
@usage
-- Log output D/onenet connect ok
log.debug("onenet", "connect ok")*/
static int l_log_debug(lua_State *L) {
    if (luat_log_get_level() > LUAT_LOG_DEBUG) return 0;
    return l_log_2_log(L, "D");
}

/*Output log, level info
@api log.info(tag, val, val2, val3, ...)
@string tag log ID, must be a string
@... Parameters to be printed
@return nil no return value
@usage
-- Log output I/onenet connect ok
log.info("onenet", "connect ok")*/
static int l_log_info(lua_State *L) {
    if (luat_log_get_level() > LUAT_LOG_INFO) return 0;
    return l_log_2_log(L, "I");
}

/*Output log, level warn
@api log.warn(tag, val, val2, val3, ...)
@string tag log ID, must be a string
@... Parameters to be printed
@return nil no return value
@usage
-- Log output W/onenet connect ok
log.warn("onenet", "connect ok")*/
static int l_log_warn(lua_State *L) {
    if (luat_log_get_level() > LUAT_LOG_WARN) return 0;
    return l_log_2_log(L, "W");
}

/*Output log, level error
@api log.error(tag, val, val2, val3, ...)
@string tag log ID, must be a string
@... Parameters to be printed
@return nil no return value
@usage
-- Log output E/onenet connect ok
log.error("onenet", "connect ok")*/
static int l_log_error(lua_State *L) {
    if (luat_log_get_level() > LUAT_LOG_ERROR) return 0;
    return l_log_2_log(L, "E");
}

#include "rotable2.h"
static const rotable_Reg_t reg_log[] =
{
    { "debug" ,     ROREG_FUNC(l_log_debug)},
    { "info" ,      ROREG_FUNC(l_log_info)},
    { "warn" ,      ROREG_FUNC(l_log_warn)},
    { "error" ,     ROREG_FUNC(l_log_error)},
    { "fatal" ,     ROREG_FUNC(l_log_error)}, // treat with error
    { "setLevel" ,  ROREG_FUNC(l_log_set_level)},
    { "getLevel" ,  ROREG_FUNC(l_log_get_level)},
    { "style",      ROREG_FUNC(l_log_style)},
    //{ "_log" ,      ROREG_FUNC(l_log_2_log)},


    //@const LOG_SILENT number No log mode
    { "LOG_SILENT", ROREG_INT(LUAT_LOG_CLOSE)},
    //@const LOG_DEBUG number debug log mode
    { "LOG_DEBUG",  ROREG_INT(LUAT_LOG_DEBUG)},
    //@const LOG_INFO number info log mode
    { "LOG_INFO",   ROREG_INT(LUAT_LOG_INFO)},
    //@const LOG_WARN number warning log mode
    { "LOG_WARN",   ROREG_INT(LUAT_LOG_WARN)},
    //@const LOG_ERROR number error log mode
    { "LOG_ERROR",  ROREG_INT(LUAT_LOG_ERROR)},
	{ NULL,         ROREG_INT(0) }
};

LUAMOD_API int luaopen_log( lua_State *L ) {
    luat_newlib2(L, reg_log);
    return 1;
}

void luat_log_dump(const char* tag, void* ptr, size_t len) {
    if (ptr == NULL) {
        luat_log_log(LUAT_LOG_DEBUG, tag, "ptr is NULL");
        return;
    }
    if (len == 0) {
        luat_log_log(LUAT_LOG_DEBUG, tag, "ptr len is 0");
        return;
    }
    char buff[256] = {0};
    uint8_t* ptr2 = (uint8_t*)ptr;
    for (size_t i = 0; i < len; i++)
    {
        sprintf_(buff + strlen(buff), "%02X ", ptr2[i]);
        if (i % 8 == 7) {
            luat_log_log(LUAT_LOG_DEBUG, tag, "%s", buff);
            buff[0] = 0;
        }
    }
    if (strlen(buff)) {
        luat_log_log(LUAT_LOG_DEBUG, tag, "%s", buff);
    }
}
