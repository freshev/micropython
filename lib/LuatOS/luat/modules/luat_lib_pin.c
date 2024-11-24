/*@Modules pin
@summary pin naming mapping
@version 1.0
@date 2021.12.05
@tagLUAT_USE_PIN
@usage
-- This library is used to solve the mapping problem between textual PIN naming and GPIO numbers.
-- In terms of function implementation, pin.PA01 corresponds to the value 1, which represents GPIO 1, and in terms of silk screen printing, it corresponds to PA01

-- PA12, GPIO12, is set to output, and is low level.
gpio.setup(12, 0)
gpio.setup(pin.PA12, 0) -- recommended
gpio.setup(pin.get("PA12"), 0) -- Not recommended, too long^_^

--Only some BSPs have this library, but not the ESP series.
-- This library is meaningful on Air101/Air103/Air105, but it is not necessary to use this library. Directly writing the GPIO number has the same effect.
-- In the ESP32 series, EC618 series (Air780E, etc.), the GPIO numbers are given directly, without "Pxxx" form, so this library does not exist*/

#include "luat_base.h"
#include "luat_pin.h"
#include <stdlib.h>

#define LUAT_LOG_TAG "pin"
#include "luat_log.h"

/**
Get the GPIO number corresponding to the pin, which can be abbreviated as pin.PA01. It is recommended to use abbreviation
@api pin.get(name)
@name The name of the pin, such as PA01, PB12
@return int The corresponding GPIO number. If it does not exist, it returns -1 and prints a warning message.
@usage
-- The following three statements are completely equivalent. If it prompts that the pin library does not exist, either the firmware version is low, please upgrade the underlying firmware, or this library is not needed.
-- PA12, GPIO12, is set to output, and is low level.
gpio.setup(12, 0)
gpio.setup(pin.PA12, 0) -- recommended
gpio.setup(pin.get("PA12"), 0) -- Not recommended, too long^_^*/
static int luat_pin_index(lua_State *L){
    size_t len;
    int pin = 0;
    const char* pin_name = luaL_checklstring(L, 1, &len);
    if (len < 3) {
        LLOGW("invaild pin id %s", pin_name);
        return 0;
    }
    pin = luat_pin_to_gpio(pin_name);
    if (pin >= 0) {
        lua_pushinteger(L, pin);
        return 1;
    }
    else {
        LLOGW("invaild pin id %s", pin_name);
        return 0;
    }
}

int luat_pin_parse(const char* pin_name, size_t* zone, size_t* index) {
    if (pin_name[0] != 'P' && pin_name[0] != 'p') {
        return -1;
    }
    // pa~pz
    if (pin_name[1] >= 'a' && pin_name[0] <= 'z') {
        *zone = pin_name[1] - 'a';
    }
    // PA~PZ
    else if (pin_name[1] >= 'A' && pin_name[0] <= 'Z') {
        *zone = pin_name[1] - 'A';
    }
    else {
        return -1;
    }
    // PA01 ~ PA99
    int re = atoi(&pin_name[2]);
    if (re < 0) {
        return -1;
    }
    *index = re;
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_pin[] =
{
    {"__index", ROREG_FUNC(luat_pin_index)},
    {"get",     ROREG_FUNC(luat_pin_index)},
	{ NULL,     ROREG_INT(0) }
};

LUAMOD_API int luaopen_pin( lua_State *L ) {
    luat_newlib2(L, reg_pin);
    return 1;
}
