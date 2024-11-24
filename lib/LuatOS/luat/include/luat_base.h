/**************************************************** ***************************
 * Basic operations of LuatOS
 * @author wendal
 * @since 0.0.1
 *************************************************** ****************************/

#ifndef LUAT_BASE_H
#define LUAT_BASE_H
/**LuatOS version number*/
#define LUAT_VERSION "23.11"
#define LUAT_VERSION_BETA 0
//Debug switch, reserved
#define LUAT_DEBUG 0

#if !defined(LUA_USE_C89) && defined(_WIN32)
#define LUAT_WEAK
#elif defined(__ARMCC_VERSION)
#define LUAT_WEAK                     __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define LUAT_WEAK                     __weak
#elif defined(__GNUC__)
#define LUAT_WEAK                     __attribute__((weak))
#elif defined(_MSC_VER)
#define LUAT_WEAK
#else
#define LUAT_WEAK                     __attribute__((weak))
#endif

//-------------------------------
//General header file

#include "stdint.h"
#include "string.h"
#include "luat_types.h"

#ifdef __LUATOS__ 
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lstate.h"
// luaopen_xxx represents various libraries, with a separate header file starting from 2021.09.26
#include "luat_libs.h"

/**
 * The main entry function of LuatOS is controlled by LuatOS from here on.
 * When integrating, this function should be started in an independent thread/task*/
int luat_main (void);

/**
 * Load library functions. The platform implementation should load available standard libraries and extension libraries based on time conditions.
 * Among them, the standard library is defined as Lua native libraries such as _G/table/io/os.
 * The extension library is the following luaopen_XXXX and the library extended by the manufacturer.*/
void luat_openlibs(lua_State *L);

// Customize the initialization entrance of the extension library, you can register the Lua library yourself, or perform other initialization operations.
void luat_custom_init(lua_State *L);

//c wait interface
uint64_t luat_pushcwait(lua_State *L);
//c waits for the interface and returns the wrong object directly to the user
void luat_pushcwait_error(lua_State *L, int arg_num);
//c waits for the interface, performs a callback response to the specified id, and carries return parameters
int luat_cbcwait(lua_State *L, uint64_t id, int arg_num);
//c wait interface, callback without parameters, does not need to be passed into the Lua stack
void luat_cbcwait_noarg(uint64_t id);

#endif

/** sprintf needs to support the printing of longlong values   and provide a platform-independent implementation*/
int l_sprintf(char *buf, size_t size, const char *fmt, ...);

/** Restart the device*/
void luat_os_reboot(int code);
/** The device enters standby mode*/
void luat_os_standy(int timeout);
/** Manufacturer/Modules name, such as Air302, Air640W*/
const char* luat_os_bsp(void);

void luat_os_entry_cri(void);

void luat_os_exit_cri(void);

uint32_t luat_os_interrupt_disable(void);

void luat_os_interrupt_enable(uint32_t level);

void luat_os_irq_disable(uint8_t IRQ_Type);

void luat_os_irq_enable(uint8_t IRQ_Type);

/** Stop starting, currently only rt-thread implementation has this setting*/
void stopboot(void);

void luat_timer_us_delay(size_t us);

const char* luat_version_str(void);

void luat_os_print_heapinfo(const char* tag);

#endif
