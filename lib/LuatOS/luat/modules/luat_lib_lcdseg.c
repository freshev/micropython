/*@Modules  lcdseg
@summary 段式lcd
@version 1.0
@date    2021.10.22
@tag LUAT_USE_LCDSEG*/

#include "luat_base.h"
#include "luat_lcdseg.h"


/**
Initialize lcdseg library
@api lcdseg.setup(bias, duty, vlcd, com_number, fresh_rate, com_mark, seg_mark)
@int bias value, usually 1/3 bias, corresponding to lcdseg.BIAS_ONETHIRD
@int duty value, usually 1/4 duty, corresponding to lcdseg.DUTY_ONEFOURTH
@int voltage, unit 100mV, for example, write 27 for 2.7v. The values   supported by air103 are 27/29/31/33
@int Number of COM pins, depends on the specific Modules, air103 supports 1-4
@int refresh rate, usually 60, corresponding to 60HZ
@int Mask of whether COM is enabled or not, the default is 0xFF, all are enabled. If only COM0/COM1 is enabled, then 0x03
@int Mask of whether seg is enabled or not, the default is 0xFFFFFFFF, that is, all are enabled. If only the first 16 are enabled, 0xFFFF
@return bool returns true if successful, otherwise returns false
@usage
--Initialize lcdseg
if lcdseg.setup(lcdseg.BIAS_ONETHIRD, lcdseg.DUTY_ONEFOURTH, 33, 4, 60) then
    lcdseg.enable(1)

    lcdseg.seg_set(0, 1, 1)
    lcdseg.seg_set(2, 0, 1)
    lcdseg.seg_set(3, 31, 1)
end*/
static int l_lcdseg_setup(lua_State* L) {
    luat_lcd_options_t opts = {0};
    opts.bias = luaL_checkinteger(L, 1);
    opts.duty = luaL_checkinteger(L, 2);
    opts.vlcd = luaL_checkinteger(L, 3);
    opts.com_number = luaL_checkinteger(L, 4);
    opts.fresh_rate = luaL_checkinteger(L, 5);
    opts.com_mark = luaL_optinteger(L, 6, 0xFF);
    opts.seg_mark = luaL_optinteger(L, 7, 0xFFFFFFFF);

    lua_pushboolean(L, luat_lcdseg_setup(&opts) == 0 ? 1 : 0);
    return 1;
}

/**
Enable or disable the lcdseg library
@api lcdseg.enable(en)
@int 1 enables, 0 disables
@return bool success or failure*/
static int l_lcdseg_enable(lua_State* L) {
    uint8_t enable = luaL_checkinteger(L, 1);
    lua_pushboolean(L, luat_lcdseg_enable(enable) == 0 ? 1 : 0);
    return 1;
}

/**
Enable or disable the output of lcdseg
@api lcdseg.power(en)
@int 1 enables, 0 disables
@return bool success or failure*/
static int l_lcdseg_power(lua_State* L) {
    uint8_t enable = luaL_checkinteger(L, 1);
    lua_pushboolean(L, luat_lcdseg_power(enable) == 0 ? 1 : 0);
    return 1;
}

/**
Set the status of a specific segment
@api lcdseg.seg_set(com, seg, en)
@int COM number
@int seg number The bit index of the field to be changed
@int 1 enables, 0 disables
@return bool success or failure*/
static int l_lcdseg_seg_set(lua_State* L) {
    uint8_t com = luaL_checkinteger(L, 1);
    uint8_t seg = luaL_checkinteger(L, 2);
    uint8_t enable = luaL_checkinteger(L, 3);
    lua_pushboolean(L, luat_lcdseg_seg_set(com, seg, enable) == 0 ? 1 : 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_lcdseg[] = {
    { "setup",      ROREG_FUNC(l_lcdseg_setup)},
    { "enable",     ROREG_FUNC(l_lcdseg_enable)},
    { "power",      ROREG_FUNC(l_lcdseg_power)},
    { "seg_set",    ROREG_FUNC(l_lcdseg_seg_set)},

    //@const BIAS_STATIC number No bias voltage (bias)
    { "BIAS_STATIC",    ROREG_INT(0)},
    //@const BIAS_ONEHALF number 1/2 bias voltage (bias)
    { "BIAS_ONEHALF",   ROREG_INT(2)},
    //@const BIAS_ONETHIRD number 1/3 bias voltage (bias)
    { "BIAS_ONETHIRD",  ROREG_INT(3)},
    //@const BIAS_ONEFOURTH number 1/4 bias voltage (bias)
    { "BIAS_ONEFOURTH", ROREG_INT(4)},


    //@const DUTY_STATIC number 100% duty cycle (duty)
    { "DUTY_STATIC",    ROREG_INT(0)},
    //@const DUTY_ONEHALF number 1/2 duty cycle (duty)
    { "DUTY_ONEHALF",   ROREG_INT(2)},
    //@const DUTY_ONETHIRD number 1/3 duty cycle (duty)
    { "DUTY_ONETHIRD",  ROREG_INT(3)},
    //@const DUTY_ONEFOURTH number 1/4 duty cycle (duty)
    { "DUTY_ONEFOURTH", ROREG_INT(4)},
    //@const DUTY_ONEFIFTH number 1/5 duty cycle (duty)
    { "DUTY_ONEFIFTH",  ROREG_INT(5)},
    //@const DUTY_ONESIXTH number 1/6 duty cycle (duty)
    { "DUTY_ONESIXTH",  ROREG_INT(6)},
    //@const DUTY_ONESEVENTH number 1/7 duty cycle (duty)
    { "DUTY_ONESEVENTH", ROREG_INT(7)},
    //@const DUTY_ONEEIGHTH number 1/8 duty cycle (duty)
    { "DUTY_ONEEIGHTH", ROREG_INT(8)},
};

LUAMOD_API int luaopen_lcdseg( lua_State *L ) {
    luat_newlib2(L, reg_lcdseg);
    return 1;
}

