
/*@Modules adc
@summary analog-to-digital conversion
@version 1.0
@date 2020.07.03
@demo adc
@tag LUAT_USE_ADC
@usage

-- This library can read the hardware adc channel, and also supports reading the CPU temperature and VBAT power supply (if the Modules supports it)

-- Read the CPU temperature, the unit is 0.001 degrees Celsius, which is the internal temperature, not the ambient temperature
adc.open(adc.CH_CPU)
local temp = adc.get(adc.CH_CPU)
adc.close(adc.CH_CPU)

-- Read VBAT supply voltage, unit is mV
adc.open(adc.CH_VBAT)
local vbat = adc.get(adc.CH_VBAT)
adc.close(adc.CH_VBAT)

--For the physical ADC channel, please refer to the comments of adc.get or adc.read*/
#include "luat_base.h"
#include "luat_adc.h"

/**
Open adc channel
@api adc.open(id)
@int channel id, related to the specific device, usually starts from 0
@return boolean open results
@usage
--Open adc channel 4 and read
if adc.open(4) then
    log.info("adc", adc.read(4)) -- There are two return values, the original value and the calculated value, usually only the latter is needed
    log.info("adc", adc.get(4)) -- There is 1 return value, only calculated value
end
adc.close(4) -- If continuous reading is required, close is not required and the power consumption will be higher.*/
static int l_adc_open(lua_State *L) {
    if (luat_adc_open(luaL_checkinteger(L, 1), NULL) == 0) {
        lua_pushboolean(L, 1);
    }
    else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

/**
Set the measurement range of the ADC. Note that this is related to the specific chip. Currently, it only supports the air105/Air780E series.
@api adc.setRange(range)
@int range parameter, related to the specific device, such as air105 fill in adc.ADC_RANGE_1_8 and adc.ADC_RANGE_3_6
@return nil
@usage
-- This function must be called before calling adc.open. Calling it afterward will be invalid!!!

-- Turn off the internal pressure divider of air105
adc.setRange(adc.ADC_RANGE_1_8)
-- Turn on the internal pressure divider of air105
adc.setRange(adc.ADC_RANGE_3_6)


-- EC618 series (Air780E, etc.)/EC718E series (Air780EP/Air780EPV, etc.) support the following 2 types
adc.setRange(adc.ADC_RANGE_1_2) -- Turn off voltage division
adc.setRange(adc.ADC_RANGE_3_8) -- enable voltage division*/
static int l_adc_set_range(lua_State *L) {
	luat_adc_global_config(ADC_SET_GLOBAL_RANGE, luaL_checkinteger(L, 1));
	return 0;
}

/**
Read adc channel
@api adc.read(id)
@int channel id, related to the specific device, usually starts from 0
@return int original value, generally useless, can be discarded directly
@return int The actual value converted from the original value, usually in mV
@usage
--Open adc channel 2 and read
if adc.open(2) then
    -- The adc.read used here will return 2 values. It is recommended to use the adc.get function to get the actual value directly.
    log.info("adc", adc.read(2))
end
adc.close(2)*/
static int l_adc_read(lua_State *L) {
    int val = 0xFF;
    int val2 = 0xFF;
    if (luat_adc_read(luaL_checkinteger(L, 1), &val, &val2) == 0) {
        lua_pushinteger(L, val);
        lua_pushinteger(L, val2);
        return 2;
    }
    else {
        lua_pushinteger(L, 0xFF);
        return 1;
    }
}

/**
Get adc calculated value
@api adc.get(id)
@int channel id, related to the specific device, usually starts from 0
@return int The unit is usually mV. Some channels will return the temperature value in thousandths of a degree Celsius. If the reading fails, -1 will be returned.
@usage
--This API is available for firmware compiled after 2022.10.01
--Open adc channel 2 and read
if adc.open(2) then
    log.info("adc", adc.get(2))
end
adc.close(2) -- close on demand*/
static int l_adc_get(lua_State *L) {
    int val = 0xFF;
    int val2 = 0xFF;
    if (luat_adc_read(luaL_checkinteger(L, 1), &val, &val2) == 0) {
        lua_pushinteger(L, val2);
    }
    else {
        lua_pushinteger(L, -1);
    }
    return 1;
}

/**
Close adc channel
@api adc.close(id)
@int channel id, related to the specific device, usually starts from 0
@usage
--Open adc channel 2 and read
if adc.open(2) then
    log.info("adc", adc.read(2))
end
adc.close(2)*/
static int l_adc_close(lua_State *L) {
    luat_adc_close(luaL_checkinteger(L, 1));
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_adc[] =
{
    { "open" ,           ROREG_FUNC(l_adc_open)},
	{ "setRange" ,       ROREG_FUNC(l_adc_set_range)},
    { "read" ,           ROREG_FUNC(l_adc_read)},
    { "get" ,            ROREG_FUNC(l_adc_get)},
    { "close" ,          ROREG_FUNC(l_adc_close)},
	//@const ADC_RANGE_3_6 number The ADC voltage dividing resistor of air105 is turned on, the range is 0~3.76V
	{ "ADC_RANGE_3_6",   ROREG_INT(1)},
	//@const ADC_RANGE_1_8 number The ADC voltage dividing resistor of air105 is turned off, the range is 0~1.88V
	{ "ADC_RANGE_1_8",   ROREG_INT(0)},
	//@const ADC_RANGE_3_8 number air780E turns on ADC0,1 voltage divider resistor, range 0~3.8V
	{ "ADC_RANGE_3_8",   ROREG_INT(7)},
	//@const ADC_RANGE_1_2 number air780E turns off ADC0,1 voltage divider resistor, range 0~1.2V
	{ "ADC_RANGE_1_2",   ROREG_INT(0)},
	{ "ADC_RANGE_MAX",   ROREG_INT(LUAT_ADC_AIO_RANGE_MAX)},
    //@const CH_CPU number Channel id of CPU internal temperature
    { "CH_CPU",          ROREG_INT(LUAT_ADC_CH_CPU)},
    //@const CH_VBAT number VBAT supply voltage channel id
    { "CH_VBAT",         ROREG_INT(LUAT_ADC_CH_VBAT)},

    //@const T1 number ADC1 (If there are multiple adcs, you can use this constant to use multiple ADCs. For example, adc.open(ADC1+2) opens ADC1 channel 2)
    { "T1",             ROREG_INT(16)},
    //@const T2 number ADC2 (If there are multiple adcs, you can use this constant to use multiple ADCs. For example, adc.open(ADC2+3) opens ADC2 channel 3)
    { "T2",             ROREG_INT(32)},
	{ NULL,              ROREG_INT(0) }
};

LUAMOD_API int luaopen_adc( lua_State *L ) {
    luat_newlib2(L, reg_adc);
    return 1;
}
