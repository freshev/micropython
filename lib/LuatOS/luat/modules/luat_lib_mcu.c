
/*@Modules mcu
@summary encapsulates some special operations of MCU
@version core V0007
@date 2021.08.18
@tag LUAT_USE_MCU*/
#include "luat_base.h"
#include "luat_mcu.h"
#include "luat_zbuff.h"
#include "luat_spi.h"
#include "luat_i2c.h"
#include "luat_uart.h"
#define LUAT_LOG_TAG "mcu"
#include "luat_log.h"
/*Set the main frequency, unit MHZ
@api mcu.setClk(mhz)
@int Main frequency, there are different valid values   depending on the device, please refer to the manual
@return bool returns true if successful, otherwise returns false
@usage

-- Note: Not all Moduless support frequency adjustment, please check the manual
-- Air101/Air103/Air601 supports setting to 2/40/80/160/240. Special reminder, after setting to 2M, if you want to sleep, you must first set to 80M
-- ESP32 series supports setting to 40/80/160/240, which requires firmware after 2024.1.1
-- Air780 series, Air105, do not support setting the main frequency
-- Air780 series, automatically downclock to 24M when entering sleep mode

-- Set to 80MHZ
mcu.setClk(80)
sys.wait(1000)
-- Set to 240MHZ
mcu.setClk(240)
sys.wait(1000)
--Set to 2MHZ
mcu.setClk(2)
sys.wait(1000)*/
static int l_mcu_set_clk(lua_State* L) {
    int ret = luat_mcu_set_clk((size_t)luaL_checkinteger(L, 1));
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    return 1;
}

/*Get the main frequency, unit MHZ
@api mcu.getClk()
@return int If it fails, it returns -1, otherwise it returns the main frequency value. If it is equal to 0, it may be in the power saving mode of the 32k crystal oscillator.
@usage
local mhz = mcu.getClk()
print("Boom", mhz)*/
static int l_mcu_get_clk(lua_State* L) {
    int mhz = luat_mcu_get_clk();
    lua_pushinteger(L, mhz);
    return 1;
}

/*Get the unique id of the device. Note that it may contain invisible characters. If you need to check, it is recommended to print after toHex()
@api mcu.unique_id()
@return string The unique ID of the device. If it is not supported, an empty string will be returned.
@usage
local unique_id = mcu.unique_id()
print("unique_id", unique_id)*/
static int l_mcu_unique_id(lua_State* L) {
    size_t len = 0;
    const char* id = luat_mcu_unique_id(&len);
    lua_pushlstring(L, id, len);
    return 1;
}

/*Get the number of ticks after startup, which is an unsigned value in the range 0~0xffffffff. Lua is a signed calculation. When calculating, it will become negative if it exceeds 0x7ffffffff.
@api mcu.ticks()
@return int current tick value
@usage
local tick = mcu.ticks()
print("ticks", tick)
-- If you need a value that will not overflow, you can use mcu.ticks32(), added in 2024.5.7*/
static int l_mcu_ticks(lua_State* L) {
    long tick = luat_mcu_ticks();
    lua_pushinteger(L, tick);
    return 1;
}

/*Get the number of ticks per second
@api mcu.hz()
@return int The number of ticks per second, usually 1000
@usage
local hz = mcu.hz()
print("mcu.hz", hz)*/
static int l_mcu_hz(lua_State* L) {
    uint32_t hz = luat_mcu_hz();
    lua_pushinteger(L, hz);
    return 1;
}

/*Read and write the 32bit register or ram of the MCU. Use the write function with caution. Please be familiar with the use of MCU registers before using it.
@api mcu.reg32(address, value, mask)
@int register or ram address
@int The value written, if not, the current value is returned directly
@int bit mask, you can modify the bits in specific positions, the default is 0xffffffff, modify all 32 bits
@return int returns the currently registered value
@usage
local value = mcu.reg32(0x2009FFFC, 0x01, 0x01) --For the value at the 0x2009FFFC address, modify bit0 to 1*/
static int l_mcu_reg32(lua_State* L) {
    volatile uint32_t *address = (uint32_t *)(luaL_checkinteger(L, 1) & 0xfffffffc);
    if (lua_isinteger(L, 2)) {
    	volatile uint32_t value = lua_tointeger(L, 2);
    	volatile uint32_t mask = luaL_optinteger(L, 3, 0xffffffff);
    	volatile uint32_t org = *address;
    	*address = (org & ~mask)| (value & mask);
    	lua_pushinteger(L, *address);
    } else {
    	lua_pushinteger(L, *address);
    }
    return 1;
}

/*Convert decimal number to hexadecimal string output
@api mcu.x32(value)
@int The value to be converted
@return string hexadecimal string
@usage
local value = mcu.x32(0x2009FFFC) --output "0x2009fffc"*/
static int l_mcu_x32(lua_State* L) {
    uint32_t value = luaL_checkinteger(L, 1);
    char c[16];
    sprintf_(c, "0x%x", value);
    lua_pushstring(L, c);
    return 1;
}

// #ifdef __LUATOS_TICK_64BIT__
/*Get the high-precision tick after startup. If the bit64 library is supported, you can directly output the converted bit64 structure.
@api mcu.tick64()
@boolean Whether to output bit64 structure, true is, others are false, leaving blank is also false, for compatibility with old demos
@return string current tick value, 8-byte uint64. If the 64bit library is supported and a 64bit structure is required to be output, a 9-byte string will be output.
@return int 1us has several ticks, 0 means unknown
@usage
local tick_str, tick_per = mcu.tick64()
print("ticks", tick_str, tick_per)*/
static int l_mcu_hw_tick64(lua_State* L) {

    uint64_t tick = luat_mcu_tick64();
    uint32_t us_period = luat_mcu_us_period();
#ifdef LUAT_USE_BIT64
	if (lua_isboolean(L, 1) && lua_toboolean(L, 1))
	{
		uint8_t data[9] = {0};
		memcpy(data, &tick, 8);
		lua_pushlstring(L, (const char*)data, 9);
	}
	else
	{
		lua_pushlstring(L, (const char*)&tick, 8);
	}
#else
    lua_pushlstring(L, (const char*)&tick, 8);
#endif
    lua_pushinteger(L, us_period);
    return 2;
}

/*Calculate the difference between two 64bit ticks
@api mcu.dtick64(tick1, tick2, check_value)
@string 64bit string
@string 64bit string
@int reference value, optional, if it is 0, the first item in the returned result is true
@return boolean Compare with the reference value, if it is greater than or equal to true, otherwise it will be false
@return int difference tick1 - tick2, if it exceeds 0x7fffffff, the result may be wrong
@usage
local result, diff_tick = mcu.dtick64(tick1, tick2)
print("ticks", result, diff_tick)*/
static int l_mcu_hw_diff_tick64(lua_State* L) {
	uint64_t tick1, tick2;
	int64_t diff;
	int check_value = 0;
    size_t len1;
    const char *data1 = luaL_checklstring(L, 1, &len1);
    size_t len2;
    const char *data2 = luaL_checklstring(L, 2, &len2);
    check_value = luaL_optinteger(L, 3, 0);

    memcpy(&tick1, data1, len1);
    memcpy(&tick2, data2, len2);
    diff = tick1 - tick2;
    lua_pushboolean(L, (diff >= (int64_t)check_value)?1:0);
    lua_pushinteger(L, diff);
    return 2;
}

/*Select clock source, currently only supported by air105
@api mcu.setXTAL(source_main, source_32k, delay)
@boolean Whether the high-speed clock uses an external clock source, if it is empty, it will not change
@boolean Whether to use an external clock source for low-speed 32K, if it is empty, it will not change
@int PLL stabilization time. When switching high-speed clock, depending on the hardware environment, it is necessary to delay for a period of time to wait for the PLL to stabilize. The default is 1200, and it is recommended not to be less than 1024.
@usage
mcu.setXTAL(true, true, 1248) --The high-speed clock uses an external clock, the low-speed 32K uses an external crystal oscillator, delay1248*/
static int l_mcu_set_xtal(lua_State* L) {
	int source_main = 255;
	int source_32k = 255;
	int delay = luaL_optinteger(L, 3, 1200);
	if (lua_isboolean(L, 1)) {
		source_main = lua_toboolean(L, 1);
	}
	if (lua_isboolean(L, 2)) {
		source_32k = lua_toboolean(L, 2);
	}
	luat_mcu_set_clk_source(source_main, source_32k, delay);
    return 0;
}
// #endif
#ifdef LUAT_COMPILER_NOWEAK
#else
LUAT_WEAK void luat_mcu_set_hardfault_mode(int mode) {;}
LUAT_WEAK void luat_mcu_xtal_ref_output(uint8_t main_enable, uint8_t slow_32k_enable) {;}
LUAT_WEAK int luat_uart_pre_setup(int uart_id, uint8_t use_alt_type){return -1;}
LUAT_WEAK int luat_i2c_set_iomux(int id, uint8_t value){return -1;}
#endif
/*Processing mode when MCU crashes, currently only applicable to EC618 platform
@api mcu.hardfault(mode)
@int Processing mode, 0 crash and stop, 1 restart after crash, 2 try to submit the error information to external tools after crash and restart 3. Write key information to flash and restart immediately after crash.
@usage
mcu.hardfault(0) --stop after crash, generally used for debugging state
mcu.hardfault(1) --Restart after crash, generally used for official products
mcu.hardfault(2) --After a crash, try to submit the error message to an external tool and then restart. It is generally used for stress testing or official products.*/
static int l_mcu_set_hardfault_mode(lua_State* L)
{
	luat_mcu_set_hardfault_mode(luaL_optinteger(L, 1, 0));
	return 0;
}

/*Before opening the peripheral, reuse the peripheral IO to a non-default configuration. Currently, only some peripherals of Air780E are supported to be reused in other configurations. This is a temporary interface. If a more suitable API is available in the future, this interface will no longer be available. renew
@api mcu.iomux(type, channel, value)
@int Peripheral type, currently only mcu.UART, mcu.I2C
@int Bus serial number, 0~N,
@int New configuration, this needs to be determined according to the specific platform
@usage
mcu.iomux(mcu.UART, 2, 1) -- Air780E's UART2 is multiplexed to gpio12 and gpio13 (Air780EG defaults to this multiplexing, don't change it)
mcu.iomux(mcu.UART, 2, 2) -- Air780E's UART2 is multiplexed to gpio6 and gpio7
mcu.iomux(mcu.I2C, 0, 1) -- I2C0 of Air780E is multiplexed to gpio12 and gpio13
mcu.iomux(mcu.I2C, 0, 2) -- I2C0 of Air780E is multiplexed to gpio16 and gpio17
mcu.iomux(mcu.I2C, 1, 1) -- Air780E's I2C1 is multiplexed to gpio4 and gpio5*/
static int l_mcu_iomux(lua_State* L)
{
	int type = luaL_optinteger(L, 1, 0xff);
	int channel = luaL_optinteger(L, 2, 0xff);
	int value = luaL_optinteger(L, 3, 0);
	LLOGD("mcu iomux %d,%d,%d", type, channel, value);
	switch(type)
	{
	case LUAT_MCU_PERIPHERAL_UART:
		luat_uart_pre_setup(channel, value);
		break;
	case LUAT_MCU_PERIPHERAL_I2C:
		luat_i2c_set_iomux(channel, value);
		break;
	}
	return 0;
}

/*IO peripheral function reuse selection, please note that ordinary MCUs use the GPIO number as the serial number, but dedicated SOCs, such as CAT1, use the PAD number as the serial number. This function is not applicable to all platforms
@api mcu.altfun(type, sn, pad_index, alt_fun, is_input)
@int Peripheral type, currently there are mcu.UART,mcu.I2C,mcu.SPI,mcu.PWM,mcu.GPIO,mcu.I2S,mcu.LCD,mcu.CAM, the specific need depends on the platform
@int Bus serial number, 0~N, if it is mcu.GPIO, it is the GPIO number. For details, please refer to the IOMUX multiplexing table of the platform.
@int pad number, if left blank, it means clearing the configuration and using the platform's default configuration. For details, please refer to the IOMUX multiplexing table of the platform.
@int Multiplex function serial number, 0~N. For details, please refer to the IOMUX multiplexing table of the platform.
@boolean Whether it is an input function, true is true, leaving blank is false
@usage
-- Take Air780EP as an example
-- Map GPIO46 to paddr 32 alt 1
mcu.altfun(mcu.GPIO, 46, 32, 1, 0)
-- mcu.altfun(mcu.GPIO, 46) -- Restore to default configuration

-- UART2 multiplexed to paddr 25/26 alt 3
mcu.altfun(mcu.UART,2, 25, 3, 1)
mcu.altfun(mcu.UART,2, 26, 3, 0)*/
static int l_mcu_alt_ctrl(lua_State* L)
{
#ifdef LUAT_MCU_IOMUX_CTRL
	int type = luaL_optinteger(L, 1, 0xff);
	int sn = luaL_optinteger(L, 2, 0xff);
	int pad = luaL_optinteger(L, 3, -1);
	int alt_fun = luaL_optinteger(L, 4, 0);
	uint8_t is_input = 0;
	if (lua_isboolean(L, 5))
	{
		is_input = lua_toboolean(L, 5);
	}
	LLOGD("mcu altfun %d,%d,%d,%d,%d", type, sn, pad, alt_fun, is_input);
	luat_mcu_iomux_ctrl(type, sn, pad, alt_fun, is_input);
#else
	LLOGW("no support mcu.altfun");
#endif
	return 0;
}

/*Get high-precision counts
@api mcu.ticks2(mode)
@int mode, see usage instructions later
@return int Depending on the mode, the return value has different meanings
@usage
-- This function was added on 2024.5.7
--The difference from mcu.ticks() is that the underlying counter is 64bit and will not overflow in the foreseeable future.
--So the value returned by this function is always increasing, and 32bit firmware can also handle it

-- Pattern optional values   and corresponding return values
-- 0: Returns the number of microseconds, divided into seconds, for example, 1234567890us returns 2 values: 1234, 567890
-- 1: Returns the number of milliseconds, divided into thousands of seconds, for example, 1234567890ms returns 2 values: 1234, 567890
-- 2: Returns the number of seconds, divided into millions of seconds, for example, 1234567890s returns 2 values: 1234, 567890

local us_h, us_l = mcu.ticks2(0)
local ms_h, ms_l = mcu.ticks2(1)
local sec_h, sec_l   = mcu.ticks2(2)
log.info("us_h", us_h, "us_l", us_l)
log.info("ms_h", ms_h, "ms_l", ms_l)
log.info("sec_h", sec_h, "sec_l", sec_l)*/
static int l_mcu_ticks2(lua_State* L) {
	int mode = luaL_optinteger(L, 1, 0);
    uint64_t tick = luat_mcu_tick64();
    uint32_t us_period = luat_mcu_us_period();
	uint64_t us = tick / us_period;
	uint64_t ms = us / 1000;
	uint64_t sec = ms / 1000;
    switch (mode)
	{
	case 0:
		// Mode 0, returns microseconds
		lua_pushinteger(L, (uint32_t)(us / 1000000));
		lua_pushinteger(L, (uint32_t)(us % 1000000));
		return 2;
	case 1:
		// Mode 1, returns milliseconds
		lua_pushinteger(L, (uint32_t)(ms / 1000000));
		lua_pushinteger(L, (uint32_t)(ms % 1000000));
		return 2;
	case 2:
		// Mode 2, return seconds
		lua_pushinteger(L, (uint32_t)(sec / 1000000));
		lua_pushinteger(L, (uint32_t)(sec % 1000000));
		return 2;
	default:
		break;
	}
    return 0;
}


/*Crystal oscillator reference clock output
@api mcu.XTALRefOutput(source_main, source_32k)
@boolean Whether the high-speed crystal oscillator reference clock is output
@boolean Whether the low-speed 32K crystal oscillator reference clock is output
@usage
-- This function was added on 2024.5.17
-- Currently only supported by Air780EP series
mcu.XTALRefOutput(true, false) --High-speed crystal oscillator reference clock output, low-speed 32K does not output*/
static int l_mcu_xtal_ref_output(lua_State* L) {
	int source_main = 0;
	int source_32k = 0;
	int delay = luaL_optinteger(L, 3, 1200);
	if (lua_isboolean(L, 1)) {
		source_main = lua_toboolean(L, 1);
	}
	if (lua_isboolean(L, 2)) {
		source_32k = lua_toboolean(L, 2);
	}
	luat_mcu_xtal_ref_output(source_main, source_32k);
    return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_mcu[] =
{
    { "setClk" ,        ROREG_FUNC(l_mcu_set_clk)},
    { "getClk",         ROREG_FUNC(l_mcu_get_clk)},
    { "unique_id",      ROREG_FUNC(l_mcu_unique_id)},
    { "ticks",          ROREG_FUNC(l_mcu_ticks)},
    { "hz",             ROREG_FUNC(l_mcu_hz)},
	{ "reg32",          ROREG_FUNC(l_mcu_reg32)},
	{ "x32",            ROREG_FUNC(l_mcu_x32)},
// #ifdef __LUATOS_TICK_64BIT__
	{ "tick64",			ROREG_FUNC(l_mcu_hw_tick64)},
	{ "dtick64",		ROREG_FUNC(l_mcu_hw_diff_tick64)},
	{ "setXTAL",		ROREG_FUNC(l_mcu_set_xtal)},
	{ "hardfault",		ROREG_FUNC(l_mcu_set_hardfault_mode)},
#ifdef LUAT_MCU_IOMUX_CTRL
	{ "altfun",			ROREG_FUNC(l_mcu_alt_ctrl)},
#else
	{ "iomux",			ROREG_FUNC(l_mcu_iomux)},
#endif
    { "ticks2",         ROREG_FUNC(l_mcu_ticks2)},
	{ "XTALRefOutput",         ROREG_FUNC(l_mcu_xtal_ref_output)},
// #endif
	//@const UART number Peripheral type-serial port
	{ "UART",             ROREG_INT(LUAT_MCU_PERIPHERAL_UART) },
	//@const I2C number Peripheral type-I2C
	{ "I2C",             ROREG_INT(LUAT_MCU_PERIPHERAL_I2C) },
	//@const SPI number Peripheral type-SPI
	{ "SPI",             ROREG_INT(LUAT_MCU_PERIPHERAL_SPI) },
	//@const PWM number Peripheral type-PWM
	{ "PWM",             ROREG_INT(LUAT_MCU_PERIPHERAL_PWM) },
	//@const GPIO number Peripheral type-GPIO
	{ "GPIO",             ROREG_INT(LUAT_MCU_PERIPHERAL_GPIO) },
	//@const I2S number Peripheral type-I2S
	{ "I2S",             ROREG_INT(LUAT_MCU_PERIPHERAL_I2S) },
	//@const LCD number Peripheral type-LCD
	{ "LCD",             ROREG_INT(LUAT_MCU_PERIPHERAL_LCD) },
	//@const CAM number Peripheral type-CAM
	{ "CAM",             ROREG_INT(LUAT_MCU_PERIPHERAL_CAM) },

	{ NULL,             ROREG_INT(0) }
};

LUAMOD_API int luaopen_mcu( lua_State *L ) {
    luat_newlib2(L, reg_mcu);
    return 1;
}
