/*@Modules ht1621
@summary LCD screen driver (HT1621/HT1621B)
@version 1.0
@author wendal
@date 2024.04.15
@tag LUAT_USE_GPIO
@usage
--You need to connect 3 GPIO pins, and then connect the power supply to ht1621
-- Assume that the CS pin is connected to GPIO4 of the Modules
-- Assume that the DATA pin is connected to GPIO5 of the Modules
-- Assume that the WR pin is connected to the GPIO3 of the Modules
local seg = ht1621.setup(4, 5, 3)
ht1621.lcd(seg, true) -- backlight on
ht1621.data(seg, 0, 0xeb) -- Position 0 displays the number 1*/
#include "luat_base.h"
#include "luat_ht1621.h"
#include "luat_gpio.h"

/*Initialize ht1621
@api ht1621.setup(pin_cs, pin_data, pin_wr, cmd_com_mode, cmd_rc, cmd_sysen)
@int Chip select pin, fill in the GPIO code of the Modules
@int data pin, fill in the GPIO code of the Modules
@int WR pin, fill in the GPIO code of the Modules
@int command mode, default is 0x52
@int Internal RC oscillator, default 0x30
@int System oscillator is on, default 0x02
@return userdata returns ht1621 object
@usage
local seg = ht1621.setup(4, 5, 3)
ht1621.data(seg, 0, 0xeb)*/
static int l_ht1621_setup(lua_State *L) {
    int pin_cs = luaL_checkinteger(L, 1);
    int pin_data = luaL_checkinteger(L, 2);
    int pin_wr = luaL_checkinteger(L, 3);
    luat_ht1621_conf_t* conf = lua_newuserdata(L, sizeof(luat_ht1621_conf_t));
    conf->pin_cs = pin_cs;
    conf->pin_data = pin_data;
    conf->pin_wr = pin_wr;
    if (lua_isinteger(L, 4)) {
        conf->cmd_com_mode = luaL_checkinteger(L, 4);
    }
    else {
        conf->cmd_com_mode = ComMode;
    }
    
    if (lua_isinteger(L, 5)) {
        conf->cmd_rc = luaL_checkinteger(L, 5);
    }
    else {
        conf->cmd_rc = RCosc;
    }

    if (lua_isinteger(L, 6)) {
        conf->cmd_sysen = luaL_checkinteger(L, 6);
    }
    else {
        conf->cmd_sysen = Sys_en;
    }

    luat_ht1621_init(conf);
    return 1;
}

/*LCD switch
@api ht1621.lcd(seg, onoff)
ht1621 object returned by @userdata ht1621.setup
@boolean true on, false off
@return nil no return value
@usage
local seg = ht1621.setup(4, 5, 3)
ht1621.lcd(seg, true)*/
static int l_ht1621_lcd(lua_State *L) {
    luat_ht1621_conf_t* conf = lua_touserdata(L, 1);
    if (conf == NULL) return 0;
    int onoff = lua_toboolean(L, 2);
    luat_ht1621_lcd(conf, onoff);
    return 0;
}

/*display data
@api ht1621.data(seg, addr, sdat)
ht1621 object returned by @userdata ht1621.setup
@int address, 0-6, invalid if more than 6
@int data, 0-255
@return nil no return value
@usage
local seg = ht1621.setup(4, 5, 3)
ht1621.lcd(seg, true)
ht1621.data(seg, 0, 0xF1)
-- Attached is a value table for numbers 0-9
-- 0,1,2,3,4,5,6,7,8,9
-- 0xeb,0x0a,0xad,0x8f,0x4e,0xc7,0xe7,0x8a,0xef,0xcf*/
static int l_ht1621_data(lua_State *L) {
    luat_ht1621_conf_t* conf = lua_touserdata(L, 1);
    if (conf == NULL) return 0;
    int addr = luaL_checkinteger(L, 2);
    int sdat = luaL_checkinteger(L, 3);
    luat_ht1621_write_data(conf, addr, sdat);
    return 0;
}

/*Send command
@api ht1621.cmd(seg, cmd)
ht1621 object returned by @userdata ht1621.setup
@int command, 0-255
@return nil no return value
@usage
-- Please refer to the hardware manual for specific instructions.*/
static int l_ht1621_cmd(lua_State *L) {
    luat_ht1621_conf_t* conf = lua_touserdata(L, 1);
    if (conf == NULL) return 0;
    int cmd = luaL_checkinteger(L, 2);
    luat_ht1621_write_cmd(conf, cmd);
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_ht1621[] =
{
    { "setup" ,           ROREG_FUNC(l_ht1621_setup)},
    { "lcd" ,             ROREG_FUNC(l_ht1621_lcd)},
    { "data" ,           ROREG_FUNC(l_ht1621_data)},
    { "cmd" ,             ROREG_FUNC(l_ht1621_cmd)},
	{ NULL,               ROREG_INT(0)}
};

LUAMOD_API int luaopen_ht1621( lua_State *L ) {
    luat_newlib2(L, reg_ht1621);
    return 1;
}
