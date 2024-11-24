
/*@Modules repl
@summary "read-evaluate-output" loop
@date 2023.06.16
@tag LUAT_USE_REPL
@usage
--[[
Modules and corresponding ports supported by this function
Module/chip port baud rate and other parameters
Air101/Air103 UART0 921600 8 None 1
Air105 UART0 1500000 8 None 1
ESP32C3 UART0 921600 8 None 1 -- Note, the simple version (without CH343) is not supported
ESP32C2 UART0 921600 8 None 1
ESP32S2 UART0 921600 8 None 1
Air780E virtual serial port any -- calling from physical UART is not supported yet

How to use:
1. For non-Air780E series, you can use any serial port tool, open the corresponding serial port, and remember to check "Carriage return and line feed"
2. Please use Air780E with LuaTools. There is "Simple serial port tool" in the menu to send. Remember to check "Carriage return and line feed"
2. Send the Lua statement and end it with carriage return and line feed.

Statement support:
1. Single-line Lua statement, end with carriage return and line feed
2. Multi-line statements, wrap them in the following format and send them, for example

<<EOF
for k,v in pairs(_G) do
  print(k,v)
end
EOF

Things to note:
1. REPL can be disabled through the repl.enable(false) statement
2. After using uart.setup/uart.close to specify the UART port, REPL automatically becomes invalid.
3. Single-line statements generally support up to 510 bytes. For longer statements, please use the "multi-line statement" method.
4. If you need to define global variables, please use the form _G.xxx = yyy

If you have any questions, please post feedback at chat.openluat.com
]]*/
#include "luat_base.h"
#include "luat_shell.h"
#include "luat_repl.h"
#define LUAT_LOG_TAG "repl"
#include "luat_log.h"

/*Proxy printing, not yet implemented*/
static int l_repl_print(lua_State* L) {
    return 0;
}

/*Enable or disable REPL functionality
@api repl.enable(re)
@bool Whether to enable or not, the default is enabled
@return previous setting status
@usage
-- If the firmware supports REPL, that is, REPL is enabled when compiling, the REPL function is enabled by default.
-- This function provides a way to close REPL
repl.enable(false)*/
static int l_repl_enable(lua_State* L) {
    int ret = 0;
    if (lua_isboolean(L, 1))
        ret = luat_repl_enable(lua_toboolean(L, 1));
    else
        ret = luat_repl_enable(-1);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Actively push pending data to the bottom layer
@api repl.push(data)
@string The data to be processed, usually comes from the serial port
@return nil no return value
@usage
-- This function is only needed for virtual serial port devices*/
static int l_repl_push(lua_State* L) {
    if (lua_isstring(L, 1)) {
        size_t len = 0;
        const char* buff = luaL_checklstring(L, 1, &len);
        if (len > 0) {
            luat_shell_push(buff, len);
        }
    }
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_repl[] =
{
    { "print" ,         ROREG_FUNC(l_repl_print)},
    { "enable" ,        ROREG_FUNC(l_repl_enable)},
    { "push" ,          ROREG_FUNC(l_repl_push)},
	{ NULL,             ROREG_INT(0) }
};

LUAMOD_API int luaopen_repl( lua_State *L ) {
    luat_newlib2(L, reg_repl);
    return 1;
}
