#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include "luat_timer.h"
#include <stdlib.h>
#include <stdlib.h>

static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base}, // _G
  {LUA_LOADLIBNAME, luaopen_package}, // require
  {LUA_COLIBNAME, luaopen_coroutine}, // coroutine coroutine library
  {LUA_TABLIBNAME, luaopen_table},    // table library, operates table type data structures
  {LUA_IOLIBNAME, luaopen_io},        // io library, operating files
  {LUA_OSLIBNAME, luaopen_os},        // os library, streamlined
  {LUA_STRLIBNAME, luaopen_string},   // string library, string operations
  {LUA_MATHLIBNAME, luaopen_math},    // math numerical calculation
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},     // debug library, streamlined
#if defined(LUA_COMPAT_BITLIB)
  {LUA_BITLIBNAME, luaopen_bit32},    // unlikely to be enabled
#endif
  {"rtos", luaopen_rtos},             // rtos underlying library, the core function is queue and timer
  {"log", luaopen_log},               //Log library
  {"timer", luaopen_timer},           //delay library
  {"pack", luaopen_pack},             // pack.pack/pack.unpack
  {"json", luaopen_cjson},             // json
  {"zbuff", luaopen_zbuff},            // 
  {"crypto", luaopen_crypto},
  {NULL, NULL}
};

// Load different library functions according to different rtconfig
void luat_openlibs(lua_State *L) {
    //Initialize queue service
    luat_msgbus_init();
    //print_list_mem("done>luat_msgbus_init");
    //Load system library
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
        //extern void print_list_mem(const char* name);
        //print_list_mem(lib->name);
    }
}

void luat_os_reboot(int code) {
    exit(code);
}

const char* luat_os_bsp(void) {
    return "mini";
}


/** The device enters standby mode*/
void luat_os_standy(int timeout) {
  (void)timeout;
  return; // nop
}

void luat_ota_reboot(int timeout_ms) {
  if (timeout_ms > 0)
    luat_timer_mdelay(timeout_ms);
  exit(0);
}

int luat_timer_mdelay(size_t ms) {
  (void)ms;
  return 0;
}


// msgbus is all empty implementation

//Define interface methods
void luat_msgbus_init(void) {}
//void* luat_msgbus_data();
uint32_t luat_msgbus_put(rtos_msg_t* msg, size_t timeout){
  (void)msg;
  (void)timeout;
  return 1;
}
uint32_t luat_msgbus_get(rtos_msg_t* msg, size_t timeout){
  (void)msg;
  (void)timeout;
  return 1;
}
uint32_t luat_msgbus_freesize(void){
  return 1;
}
uint8_t luat_msgbus_is_empty(void){
  return 0;
}

int luat_timer_start(luat_timer_t* timer) {
  (void)timer;
  return -1;
}
int luat_timer_stop(luat_timer_t* timer) {
  (void)timer;
  return -1;
}
luat_timer_t* luat_timer_get(size_t timer_id) {
  (void)timer_id;
  return NULL;
}

void luat_timer_us_delay(size_t us) {
  (void)us;
}
