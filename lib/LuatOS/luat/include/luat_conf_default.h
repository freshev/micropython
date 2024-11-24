
#ifndef LUAT_CONF_DEFAULT

// First, luat_conf.h must be Introductiond first
#ifndef LUAT_CONF_BSP
#error "include luat_conf_bsp.h first!!!"
#endif

//------------------------------
// Lua virtual machine related features
//------------------------------


// Whether to use a 64bit virtual machine, closed by default
#ifndef LUAT_CONF_VM_64bit
    #define LUA_32BITS
#endif

// Whether to use the platform-customized sprintf method
// By default, l_sprintf provided by printf.h is used
#ifndef LUAT_CONF_CUSTOM_SPRINTF
    #include "printf.h"
    #define l_sprintf snprintf_
#endif

// Whether to use static LuaState state
#ifndef LUAT_CONF_LUASTATE_NOT_STATIC
    #define FEATURE_STATIC_LUASTATE 1
#endif

// LUA lauxlib buff system size
#ifndef LUAT_CONF_LAUX_BUFFSIZE
    #define LUAL_BUFFERSIZE 256
#else
    #define LUAL_BUFFERSIZE LUAT_CONF_LAUX_BUFFSIZE
#endif

//------------------------------
// LuatOS features
//-----------------------------

// Whether to use rotable to save fixed memory of built-in libraries
#ifndef LUAT_CONF_DISABLE_ROTABLE
    #define LUAT_USING_ROTABLE
#endif

// Customize the hook when the VM exits (exit with exception or restart after upgrade)
#ifdef LUAT_CONF_CUSTOM_VM_EXIT_HOOK
    void luat_os_vm_exit_hook(int code, int delayMs);
#endif

// OTA hook
#ifdef LUAT_CONF_CUSTOM_OTA_HOOK
    void luat_os_ota_hook(void);
#endif

// Whether to support SSL/TLS/DLTS
#ifdef LUAT_CONF_SUPPORT_SSL
    
#endif

//------------------------------

#endif
