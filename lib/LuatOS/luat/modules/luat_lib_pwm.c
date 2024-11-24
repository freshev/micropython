/*@Modules  pwm
@summary PWM模块
@version 1.0
@date    2020.07.03
@demo pwm
@tag LUAT_USE_PWM*/
#include "luat_base.h"
#include "luat_pwm.h"

/**
Turn on the specified PWM channel
@api pwm.open(channel, period, pulse, pnum, precision)
@int PWM channel
@int frequency, 1-1000000hz
@int duty cycle 0-frequency division accuracy
@int Output period 0 is continuous output, 1 is single output, others are specified pulse number output
@int Frequency division accuracy, 100/256/1000, default is 100, if the device does not support it, there will be a log prompt
@return boolean processing result, returns true if successful, false if failed
@usage
-- Turn on PWM5, frequency 1kHz, duty cycle 50%
pwm.open(5, 1000, 50)
-- Turn on PWM5, frequency 10kHz, frequency division is 31/256
pwm.open(5, 10000, 31, 0, 256)*/
static int l_pwm_open(lua_State *L) {
    luat_pwm_conf_t conf = {
        .pnum = 0,
        .precision = 100
    };
    conf.channel = luaL_checkinteger(L, 1);
    conf.period = luaL_checkinteger(L, 2);
    conf.pulse = luaL_optnumber(L, 3,0);
    if (lua_isnumber(L, 4) || lua_isinteger(L, 4)){
        conf.pnum = luaL_checkinteger(L, 4);
    }
    if (lua_isnumber(L, 5) || lua_isinteger(L, 5)){
        conf.precision = luaL_checkinteger(L, 5);
    }
    int ret = luat_pwm_setup(&conf);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/**
Close the specified PWM channel
@api pwm.close(channel)
@int PWM channel
@return nil no processing result
@usage
-- Turn off PWM5
pwm.close(5)*/
static int l_pwm_close(lua_State *L) {
    luat_pwm_close(luaL_checkinteger(L, 1));
    return 0;
}

/**
PWM capture
@api pwm.capture(channel)
@int PWM channel
@int capture frequency
@return boolean processing result, returns true if successful, false if failed
@usage
--PWM0 capture
while 1 do
    pwm.capture(0,1000)
    local ret,channel,pulse,pwmH,pwmL = sys.waitUntil("PWM_CAPTURE", 2000)
    if ret then
        log.info("PWM_CAPTURE","channel"..channel,"pulse"..pulse,"pwmH"..pwmH,"pwmL"..pwmL)
    end
end*/
static int l_pwm_capture(lua_State *L) {
    int ret = luat_pwm_capture(luaL_checkinteger(L, 1),luaL_checkinteger(L, 2));
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_pwm[] =
{
    { "open" ,       ROREG_FUNC(l_pwm_open )},
    { "close" ,      ROREG_FUNC(l_pwm_close)},
    { "capture" ,    ROREG_FUNC(l_pwm_capture)},
	{ NULL,          ROREG_INT(0) }
};

LUAMOD_API int luaopen_pwm( lua_State *L ) {
    luat_newlib2(L, reg_pwm);
    return 1;
}
