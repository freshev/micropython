
/*@Modulesos
@summary os operation
@version 1.0
@date 2020.07.03
@tag LUAT_CONF_BSP
@demo os_date_time
@usage
-- The os Modules is a native Modules of Lua. This document is to facilitate the explanation of common problems in actual use.

-- For native documentation, please refer to https://wiki.luatos.com/_static/lua53doc/manual.html#6.9*/

#define loslib_c
#define LUA_LIB

#include "lprefix.h"


#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include "luat_fs.h"


/*
** {==================================================================
** List of valid conversion specifiers for the 'strftime' function;
** options are grouped by length; group of length 2 start with '||'.
** ===================================================================
*/
#if !defined(LUA_STRFTIMEOPTIONS)	/* { */

/* options for ANSI C 89 (only 1-char options) */
#define L_STRFTIMEC89		"aAbBcdHIjmMpSUwWxXyYZ%"

/* options for ISO C 99 and POSIX */
#define L_STRFTIMEC99 "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%" \
    "||" "EcECExEXEyEY" "OdOeOHOIOmOMOSOuOUOVOwOWOy"  /* two-char options */

/* options for Windows */
#define L_STRFTIMEWIN "aAbBcdHIjmMpSUwWxXyYzZ%" \
    "||" "#c#x#d#H#I#j#m#M#S#U#w#W#y#Y"  /* two-char options */

// #if defined(LUA_USE_WINDOWS)
// #define LUA_STRFTIMEOPTIONS	L_STRFTIMEWIN
// #elif defined(LUA_USE_C89)
#define LUA_STRFTIMEOPTIONS	L_STRFTIMEC89
// #else  /* C99 specification */
// #define LUA_STRFTIMEOPTIONS	L_STRFTIMEC99
// #endif

#endif					/* } */
/* }================================================================== */


/*
** {==================================================================
** Configuration for time-related stuff
** ===================================================================
*/

#if !defined(l_time_t)		/* { */
/*
** type to represent time_t in Lua
*/
#define l_timet			lua_Integer
#define l_pushtime(L,t)		lua_pushinteger(L,(lua_Integer)(t))

static time_t l_checktime (lua_State *L, int arg) {
  lua_Integer t = luaL_checkinteger(L, arg);
  luaL_argcheck(L, (time_t)t == t, arg, "time out-of-bounds");
  return (time_t)t;
}

#endif				/* } */


#if !defined(l_gmtime)		/* { */
/*
** By default, Lua uses gmtime/localtime, except when POSIX is available,
** where it uses gmtime_r/localtime_r
*/

#if defined(LUA_USE_POSIX)	/* { */

#define l_gmtime(t,r)		gmtime_r(t,r)
#define l_localtime(t,r)	localtime_r(t,r)

#else				/* }{ */

/* ISO C definitions */
#define l_gmtime(t,r)		((void)(r)->tm_sec, gmtime(t))
#define l_localtime(t,r)  	((void)(r)->tm_sec, localtime(t))

#endif				/* } */

#endif				/* } */

/* }================================================================== */


/*
** {==================================================================
** Configuration for 'tmpnam':
** By default, Lua uses tmpnam except when POSIX is available, where
** it uses mkstemp.
** ===================================================================
*/
#if !defined(lua_tmpnam)	/* { */

#if defined(LUA_USE_POSIX)	/* { */

#include <unistd.h>

#define LUA_TMPNAMBUFSIZE	32

#if !defined(LUA_TMPNAMTEMPLATE)
#define LUA_TMPNAMTEMPLATE	"/tmp/lua_XXXXXX"
#endif

#define lua_tmpnam(b,e) { \
        strcpy(b, LUA_TMPNAMTEMPLATE); \
        e = mkstemp(b); \
        if (e != -1) close(e); \
        e = (e == -1); }

#else				/* }{ */

/* ISO C definitions */
#define LUA_TMPNAMBUFSIZE	L_tmpnam
#define lua_tmpnam(b,e)		{ e = (tmpnam(b) == NULL); }

#endif				/* } */

#endif				/* } */
/* }================================================================== */



#if defined(LUA_USE_LINUX) || defined(LUA_USE_WINDOWS) || defined(LUA_USE_MACOSX)
static int os_execute (lua_State *L) {
  const char *cmd = luaL_optstring(L, 1, NULL);
  int stat = system(cmd);
  if (cmd != NULL)
    return luaL_execresult(L, stat);
  else {
    lua_pushboolean(L, stat);  /* true if there is a shell */
    return 1;
  }
}
#endif

/*Remove files
@api os.remove(path)
@string full path of the file to be removed
@return bool returns true if successful, nil otherwise.
@return string Returns the reason string when failure occurs
@usage
-- Delete a file in the root directory
os.remove("/1.txt")
-- Note that files during online flashing are usually in the /luadb directory. Files in this directory are read-only.
-- That is, os.remove("/luadb/xxx.bin") cannot be executed*/
static int os_remove (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  return luaL_fileresult(L, luat_fs_remove(filename) == 0, filename);
}

/*File rename
@api os.rename(old_path, new_path)
@string full path of source file
@string full path of target file
@return bool returns true if successful, nil otherwise.
@return string Returns the reason string when failure occurs
@usage
-- Note that only files in the same file system can be renamed
-- For example:
os.rename("/1.txt", "/2.txt")
-- Different file systems, or the source file system is read-only, cannot be executed.
--os.rename("/luadb/1.txt", "/luadb/2.txt")
--os.rename("/luadb/1.txt", "/2.txt")*/
static int os_rename (lua_State *L) {
  const char *fromname = luaL_checkstring(L, 1);
  const char *toname = luaL_checkstring(L, 2);
  return luaL_fileresult(L, luat_fs_rename(fromname, toname) == 0, NULL);
}


#if defined(LUA_USE_LINUX) || defined(LUA_USE_WINDOWS) || defined(LUA_USE_MACOSX)
static int os_tmpname (lua_State *L) {
  char buff[LUA_TMPNAMBUFSIZE];
  int err;
  lua_tmpnam(buff, err);
  if (err)
    return luaL_error(L, "unable to generate a unique filename");
  lua_pushstring(L, buff);
  return 1;
}

static int os_getenv (lua_State *L) {
  lua_pushstring(L, getenv(luaL_checkstring(L, 1)));  /* if NULL push nil */
  return 1;
}
#endif

/*Returns an approximate value in seconds of CPU time used by the program
@api os.clock()
@return timestamp
@usage
-- It is not recommended to use this API
-- If you need to get the timestamp, please use os.time()
-- To get the system running time, please use mcu.ticks()*/
static int os_clock (lua_State *L) {
  lua_pushnumber(L, ((lua_Number)clock())/(lua_Number)CLOCKS_PER_SEC);
  return 1;
}


/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield (lua_State *L, const char *key, int value) {
  lua_pushinteger(L, value);
  lua_setfield(L, -2, key);
}

static void setboolfield (lua_State *L, const char *key, int value) {
  if (value < 0)  /* undefined? */
    return;  /* does not set field */
  lua_pushboolean(L, value);
  lua_setfield(L, -2, key);
}


/*
** Set all fields from structure 'tm' in the table on top of the stack
*/
static void setallfields (lua_State *L, struct tm *stm) {
  setfield(L, "sec", stm->tm_sec);
  setfield(L, "min", stm->tm_min);
  setfield(L, "hour", stm->tm_hour);
  setfield(L, "day", stm->tm_mday);
  setfield(L, "month", stm->tm_mon + 1);
  setfield(L, "year", stm->tm_year + 1900);
  setfield(L, "wday", stm->tm_wday + 1);
  setfield(L, "yday", stm->tm_yday + 1);
  setboolfield(L, "isdst", stm->tm_isdst);
}


static int getboolfield (lua_State *L, const char *key) {
  int res;
  res = (lua_getfield(L, -1, key) == LUA_TNIL) ? -1 : lua_toboolean(L, -1);
  lua_pop(L, 1);
  return res;
}


/* maximum value for date fields (to avoid arithmetic overflows with 'int') */
#if !defined(L_MAXDATEFIELD)
#define L_MAXDATEFIELD	(INT_MAX / 2)
#endif

static int getfield (lua_State *L, const char *key, int d, int delta) {
  int isnum;
  int t = lua_getfield(L, -1, key);  /* get field and its type */
  lua_Integer res = lua_tointegerx(L, -1, &isnum);
  if (!isnum) {  /* field is not an integer? */
    if (t != LUA_TNIL)  /* some other value? */
      return luaL_error(L, "field '%s' is not an integer", key);
    else if (d < 0)  /* absent field; no default? */
      return luaL_error(L, "field '%s' missing in date table", key);
    res = d;
  }
  else {
    if (!(-L_MAXDATEFIELD <= res && res <= L_MAXDATEFIELD))
      return luaL_error(L, "field '%s' is out-of-bound", key);
    res -= delta;
  }
  lua_pop(L, 1);
  return (int)res;
}


static const char *checkoption (lua_State *L, const char *conv,
                                ptrdiff_t convlen, char *buff) {
  const char *option = LUA_STRFTIMEOPTIONS;
  int oplen = 1;  /* length of options being checked */
  for (; *option != '\0' && oplen <= convlen; option += oplen) {
    if (*option == '|')  /* next block? */
      oplen++;  /* will check options with next length (+1) */
    else if (memcmp(conv, option, oplen) == 0) {  /* match? */
      memcpy(buff, conv, oplen);  /* copy valid option to buffer */
      buff[oplen] = '\0';
      return conv + oplen;  /* return next item */
    }
  }
  luaL_argerror(L, 1,
    lua_pushfstring(L, "invalid conversion specifier '%%%s'", conv));
  return conv;  /* to avoid warnings */
}


/* maximum size for an individual 'strftime' item */
#define SIZETIMEFMT	250

#ifdef LUAT_USE_RTC
#include "luat_rtc.h"
#endif

/*date function
@api os.date(fmt, time)
@string format string, can be nil
@table date and time table
@return table/string The return value is different depending on the fmt
@usage

-- A few points worth noting:
-- 1. If UTC time is required, write "!" as the first character of fmt.
-- 2. The formatting of fmt follows the C function strftime, which can be found at https://developer.aliyun.com/article/320480

-- Get the local time string
log.info("local time string", os.date())
-- Get UTC time string
log.info("UTC time string", os.date("!%c"))
-- Format local time string
log.info("local time string", os.date("%Y-%m-%d %H:%M:%S"))
-- Format UTC time string
log.info("UTC time string", os.date("!%Y-%m-%d %H:%M:%S"))
-- Format time string
log.info("Custom time string", os.date("!%Y-%m-%d %H:%M:%S", os.time({year=2000, mon=1, day=1 , hour=0, min=0, sec=0})))

-- Get the table of local time
log.info("local time string", json.encode(os.date("*t")))
-- Get the table of UTC time
log.info("UTC time string", json.encode(os.date("!*t")))*/
static int os_date (lua_State *L) {
  size_t slen;
  const char *s = luaL_optlstring(L, 1, "%c", &slen);
  time_t t = luaL_opt(L, l_checktime, 2, time(NULL));
  const char *se = s + slen;  /* 's' end */
  struct tm tmr, *stm;
  if (*s == '!') {  /* UTC? */
    stm = l_gmtime(&t, &tmr);
    s++;  /* skip '!' */
  }
  else{
    #ifdef LUAT_USE_RTC
    t += (luat_rtc_timezone(NULL) / 4) * 3600;
    stm = l_gmtime(&t, &tmr);
    #else
    stm = l_localtime(&t, &tmr);
    #endif
  }
  if (stm == NULL)  /* invalid date? */
    return luaL_error(L,
                 "time result cannot be represented in this installation");
  if (strcmp(s, "*t") == 0) {
    lua_createtable(L, 0, 9);  /* 9 = number of fields */
    setallfields(L, stm);
  }
  else {
    char cc[4];  /* buffer for individual conversion specifiers */
    luaL_Buffer b;
    cc[0] = '%';
    luaL_buffinit(L, &b);
    while (s < se) {
      if (*s != '%')  /* not a conversion specifier? */
        luaL_addchar(&b, *s++);
      else {
        size_t reslen;
        char *buff = luaL_prepbuffsize(&b, SIZETIMEFMT);
        s++;  /* skip '%' */
        s = checkoption(L, s, se - s, cc + 1);  /* copy specifier to 'cc' */
        reslen = strftime(buff, SIZETIMEFMT, cc, stm);
        luaL_addsize(&b, reslen);
      }
    }
    luaL_pushresult(&b);
  }
  return 1;
}

/*timestamp function
@api os.time(mytime)
@table date and time table
@return timestamp
@usage
-- Note that this function returns a UTC timestamp.
-- Timestamp, but the precision under Lua can only be seconds
log.info("UTC timestamp", os.time())
log.info("Custom timestamp", os.time({year=2000, mon=1, day=1, hour=0, min=0, sec=0}))*/
static int os_time (lua_State *L) {
  time_t t;
  if (lua_isnoneornil(L, 1))  /* called without args? */
  {
    t = time(NULL);  /* get current time */
  }
  else {
    struct tm ts;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);  /* make sure table is at the top */
    ts.tm_sec = getfield(L, "sec", 0, 0);
    ts.tm_min = getfield(L, "min", 0, 0);
    ts.tm_hour = getfield(L, "hour", 12, 0);
    ts.tm_mday = getfield(L, "day", -1, 0);
    ts.tm_year = getfield(L, "year", -1, 1900);
    ts.tm_isdst = getboolfield(L, "isdst");
    // Compatible with the mon attribute of the RTC library, why didn’t I think of it at the beginning?
    // https://gitee.com/openLuat/LuatOS/issues/I7MYRS
    if (lua_getfield(L, 1, "mon") != LUA_TNIL) {
      ts.tm_mon = lua_tointeger(L, -1) -1;
      lua_pop(L, 1);
    }
    else {
      lua_settop(L, 1);
      ts.tm_mon = getfield(L, "month", -1, 1);
    }
    t = mktime(&ts);
    setallfields(L, &ts);  /* update fields with normalized values */
  }
  if (t != (time_t)(l_timet)t || t == (time_t)(-1))
    return luaL_error(L,
                  "time result cannot be represented in this installation");
  l_pushtime(L, t);
  return 1;
}

/*time difference
@api os.difftime(timeA, timeB)
@int time A, numerical type
@int time B, numerical type
@return int time difference*/
static int os_difftime (lua_State *L) {
  time_t t1 = l_checktime(L, 1);
  time_t t2 = l_checktime(L, 2);
  lua_pushnumber(L, (lua_Number)difftime(t1, t2));
  return 1;
}

/* }====================================================== */

#if defined(LUA_USE_LINUX) || defined(LUA_USE_WINDOWS) || defined(LUA_USE_MACOSX)
static int os_setlocale (lua_State *L) {
  static const int cat[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
                      LC_NUMERIC, LC_TIME};
  static const char *const catnames[] = {"all", "collate", "ctype", "monetary",
     "numeric", "time", NULL};
  const char *l = luaL_optstring(L, 1, NULL);
  int op = luaL_checkoption(L, 2, "all", catnames);
  lua_pushstring(L, setlocale(cat[op], l));
  return 1;
}

static int os_exit (lua_State *L) {
  int status;
  if (lua_isboolean(L, 1))
    status = (lua_toboolean(L, 1) ? EXIT_SUCCESS : EXIT_FAILURE);
  else
    status = (int)luaL_optinteger(L, 1, EXIT_SUCCESS);
  if (lua_toboolean(L, 2))
    lua_close(L);
  if (L) exit(status);  /* 'if' to avoid warnings for unreachable 'return' */
  return 0;
}
#endif

#include "rotable2.h"
static const rotable_Reg_t syslib[] = {
  {"clock",     ROREG_FUNC(os_clock)},
  {"date",      ROREG_FUNC(os_date)},
  {"difftime",  ROREG_FUNC(os_difftime)},
#if defined(LUA_USE_LINUX) || defined(LUA_USE_WINDOWS) || defined(LUA_USE_MACOSX)
 {"execute",   ROREG_FUNC(os_execute)},
 {"exit",      ROREG_FUNC(os_exit)},
 {"getenv",    ROREG_FUNC(os_getenv)},
 {"setlocale", ROREG_FUNC(os_setlocale)},
 {"tmpname",   ROREG_FUNC(os_tmpname)},
#endif
  {"remove",    ROREG_FUNC(os_remove)},
  {"rename",    ROREG_FUNC(os_rename)},
  {"time",      ROREG_FUNC(os_time)},
  {NULL, ROREG_INT(0) }
};

/* }====================================================== */



LUAMOD_API int luaopen_os (lua_State *L) {
  luat_newlib2(L, syslib);
  return 1;
}

