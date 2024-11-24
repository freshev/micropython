
/*@Modules sqlite3
@summary sqlite3 database operations
@version 1.0
@date 2023.11.13
@demo sqlite3
@tag LUAT_USE_SQLITE3
@usage
-- Note that this library is still in the development stage, and most BSPs do not yet support this library.
-- This transplant is based on sqlite3 3.44.0
sys.taskInit(function()
    sys.wait(1000)
    local db = sqlite3.open("/ram/test.db")
    log.info("sqlite3", db)
    if db then
        sqlite3.exec(db, "CREATE TABLE devs(ID INT PRIMARY KEY NOT NULL, name CHAR(50));")
        sqlite3.exec(db, "insert into devs values(1, \"ABC\");")
        sqlite3.exec(db, "insert into devs values(2, \"DEF\");")
        sqlite3.exec(db, "insert into devs values(3, \"HIJ\");")
        local ret, data = sqlite3.exec(db, "select * from devs;")
        log.info("Query results", ret, data)
        if ret then
            for k, v in pairs(data) do
                log.info("data", json.encode(v))
            end
        end
        sqlite3.close(db)
    end
end)*/

#include "luat_base.h"
#include "sqlite3.h"
#include "luat_msgbus.h"

#define LUAT_LOG_TAG "sqlite3"
#include "luat_log.h"

/*Open database
@api sqlite3.open(path)
@string database file path, must be filled in, it will be created automatically if it does not exist
@return userdata database pointer, whether to return nil
@usage
local db = sqlite3.open("/test.db")
if db then
   -- Database operation xxxx

    -- Must be turned off after use
    sqlite3.close(db)
end*/
static int l_sqlite3_open(lua_State *L) {
    sqlite3 *db;
    int rc;
    const char* path = luaL_checkstring(L, 1);
    rc = sqlite3_open(path, &db);
    if (rc == SQLITE_OK) {
        lua_pushlightuserdata(L, db);
        return 1;
    }
    LLOGW("Failed to open database %d %s", rc, sqlite3_errstr(rc));
    return 0;
}

static int s_cb(void* args, int nc, char* azResults[], char* azColumns[]) {
    lua_State *L = (lua_State*)args;
    lua_createtable(L, 0, nc);
    size_t count = nc > 0 ? nc : 0;
    for (size_t i = 0; i < count; i++)
    {
        lua_pushstring(L, azResults[i]);
        lua_setfield(L, -2, azColumns[i]);
    }
    lua_seti(L, -2, lua_rawlen(L, -2) + 1);
    return 0;
}

/*Execute SQL statement
@api sqlite3.exec(db, sql)
@userdata The database pointer obtained through sqlite3.open
@string SQL string, must be filled in
@return boolean returns true if successful, otherwise returns nil
@return table successfully returns the query result (if any), otherwise returns the error string*/
static int l_sqlite3_exec(lua_State *L) {
    sqlite3 *db;
    int rc;
    char* errmsg;
    db = lua_touserdata(L, 1);
    if (db == NULL) {
        return 0;
    }
    const char* sql = luaL_checkstring(L, 2);
    lua_newtable(L);
    rc = sqlite3_exec(db, sql, s_cb, L, &errmsg);
    if (rc == SQLITE_OK) {
        lua_pushboolean(L, 1);
        lua_pushvalue(L, -2);
        return 2;
    }
    lua_pushnil(L);
    lua_pushstring(L, errmsg);
    //LLOGW("Failed to execute SQL %s %d %s", sql, rc, errmsg);
    return 2;
}

/*Close database
@api sqlite3.close(db)
@userdata The database pointer obtained through sqlite3.open
@return boolean returns true if successful, otherwise returns nil*/
static int l_sqlite3_close(lua_State *L) {
    sqlite3 *db;
    int rc;
    db = lua_touserdata(L, 1);
    if (db == NULL) {
        return 0;
    }
    rc = sqlite3_close(db);
    if (rc == SQLITE_OK) {
        lua_pushboolean(L, 1);
        return 0;
    }
    LLOGW("Failed to close database %d", rc);
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_sqlite3[] =
{
    { "open" ,            ROREG_FUNC(l_sqlite3_open)},
    { "exec" ,            ROREG_FUNC(l_sqlite3_exec)},
    { "close" ,           ROREG_FUNC(l_sqlite3_close)},
	{ NULL,               ROREG_INT(0)}
};

extern int luat_sqlite3_init(void);

LUAMOD_API int luaopen_sqlite3( lua_State *L ) {
    luat_newlib2(L, reg_sqlite3);
    luat_sqlite3_init();
    return 1;
}


/*Instructions for modifying the sqlite3 source code
No changes were made to the main body, only the following macro definition was added to the header

#define SQLITE_OMIT_WAL 1
#define SQLITE_THREADSAFE 0
#define SQLITE_DEFAULT_MEMSTATUS 0
#define SQLITE_OMIT_LOAD_EXTENSION 1
#define SQLITE_OMIT_LOCALTIME 1
#define SQLITE_OMIT_MEMORYDB 1
#define SQLITE_OMIT_SHARED_CACHE
#define SQLITE_OS_OTHER 1
#define SQLITE_OMIT_SEH

The biggest limitation on sqlite3 is memory usage. It is said that the stack memory requires more than 12k, and the heap memory requires 100~200k. There is no actual verification.*/
