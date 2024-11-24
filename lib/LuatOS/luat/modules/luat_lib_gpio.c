
/*@Modules  gpio
@summary GPIO操作
@catalog 外设API
@version 1.0
@date    2020.03.30
@demo gpio
@video https://www.bilibili.com/video/BV1hr4y1p7dt
@tag LUAT_USE_GPIO*/
#include "luat_base.h"
#include "luat_gpio.h"
#include "luat_mem.h"
#include "luat_mcu.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_rtos.h"
#include "luat_mcu.h"
#include <math.h>

#define LUAT_LOG_TAG "gpio"
#include "luat_log.h"

static int l_gpio_set(lua_State *L);
static int l_gpio_get(lua_State *L);
static int l_gpio_close(lua_State *L);
static int l_gpio_get_count(lua_State *L);
int l_gpio_handler(lua_State *L, void* ptr) ;

typedef struct gpio_ctx
{
    int lua_ref; //Callback function under irq
    luat_rtos_timer_t timer;
    uint32_t irq_cnt;		//count of interrupts
    uint32_t latest_tick; //The last number of ticks of the anti-shake function
    uint16_t conf_tick;   //Number of timeout ticks for anti-shake settings
    uint8_t debounce_mode;
    uint8_t latest_state;
    uint8_t irq_type;
}gpio_ctx_t;

//Save the array of interrupt callbacks
static gpio_ctx_t gpios[LUAT_GPIO_PIN_MAX];
static uint32_t default_gpio_pull = Luat_GPIO_DEFAULT;


// Record GPIO level, only available when OUTPUT
static uint8_t gpio_out_levels[(LUAT_GPIO_PIN_MAX + 7) / 8];

static uint8_t gpio_bit_get(int pin) {
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX)
        return 0;
    return (gpio_out_levels[pin/8] >> (pin%8)) & 0x01;
}

static void gpio_bit_set(int pin, uint8_t value) {
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX)
        return;
    uint8_t val = (gpio_out_levels[pin/8] >> (pin%8)) & 0x01;
    if (val == value)
        return; // No change
    if (value == 0) {
        gpio_out_levels[pin/8] -= (1 << (pin%8));
    }
    else {
        gpio_out_levels[pin/8] += (1 << (pin%8));
    }
}

int l_gpio_debounce_timer_handler(lua_State *L, void* ptr) {
    (void)L;
    (void)ptr;

    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    int pin = msg->arg1;
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX)
        return 0; //Out of range, memory exception
    if (gpios[pin].lua_ref == 0)
        return 0; // turned off long ago
    if (gpios[pin].latest_state != luat_gpio_get(pin))
        return 0; //The level has changed
    if (gpios[pin].debounce_mode)
    {
        switch(gpios[pin].irq_type)
        {
        case Luat_GPIO_RISING:
        case Luat_GPIO_HIGH_IRQ:
        	if (!gpios[pin].latest_state) return 0;
        	break;
        case Luat_GPIO_FALLING:
        case Luat_GPIO_LOW_IRQ:
        	if (gpios[pin].latest_state) return 0;
        	break;
        }
    }

    lua_geti(L, LUA_REGISTRYINDEX, gpios[pin].lua_ref);
    if (!lua_isnil(L, -1)) {
        lua_pushinteger(L, gpios[pin].latest_state);
        lua_call(L, 1, 0);
    }
    return 0;
}

#ifndef LUAT_RTOS_API_NOTOK
static LUAT_RT_RET_TYPE l_gpio_debounce_mode1_cb(LUAT_RT_CB_PARAM) {
    int pin = (int)param;
    rtos_msg_t msg = {0};
    msg.handler = l_gpio_debounce_timer_handler;
    msg.arg1 = pin;
    luat_msgbus_put(&msg, 0);
}
#endif

static int luat_gpio_irq_count(int pin, void* args) {
	gpios[pin].irq_cnt++;
	return 0;
}

int luat_gpio_irq_default(int pin, void* args) {
    rtos_msg_t msg = {0};
#ifdef LUAT_GPIO_PIN_MAX
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX) {
#else
    if (pin < 0 || pin >= Luat_GPIO_MAX_ID) {
#endif
        return 0;
    }

    if (pin < LUAT_GPIO_PIN_MAX && gpios[pin].conf_tick > 0) {
        // Anti-shake mode 0, cool down N ms after triggering
        if (gpios[pin].debounce_mode == 0) {
            uint32_t ticks = (uint32_t)luat_mcu_ticks();
            uint32_t diff = (ticks > gpios[pin].latest_tick) ? (ticks - gpios[pin].latest_tick) : (gpios[pin].latest_tick - ticks);
            if (diff >= gpios[pin].conf_tick) {
                gpios[pin].latest_tick = ticks;
            }
            else {
                // Anti-shake takes effect, return directly
            return 0;
            }
        }
        #ifndef LUAT_RTOS_API_NOTOK
        // Anti-shake mode 1, delay N ms after triggering, and trigger only when the level remains unchanged
        else if (gpios[pin].debounce_mode == 1) {
            if (gpios[pin].timer == NULL || gpios[pin].conf_tick == 0) {
                return 0; // timer is released?
            }
            gpios[pin].latest_state = luat_gpio_get(pin);
            luat_rtos_timer_stop(gpios[pin].timer);
            luat_rtos_timer_start(gpios[pin].timer, gpios[pin].conf_tick, 0, l_gpio_debounce_mode1_cb, (void*)pin);
            return 0;
        }
        #endif
    }

    msg.handler = l_gpio_handler;
    msg.ptr = NULL;
    msg.arg1 = pin;
    msg.arg2 = (int)args;
    return luat_msgbus_put(&msg, 0);
}

int l_gpio_handler(lua_State *L, void* ptr) {
    (void)ptr; // unused
    //Send data to sys.publish method
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    int pin = msg->arg1;
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX)
        return 0;
    if (gpios[pin].lua_ref == 0)
        return 0;
    lua_geti(L, LUA_REGISTRYINDEX, gpios[pin].lua_ref);
    if (!lua_isnil(L, -1)) {
        lua_pushinteger(L, msg->arg2);
        lua_pushinteger(L, msg->arg1);
        lua_call(L, 2, 0);
    }
    return 0;
}

/*Set pin function
@api gpio.setup(pin, mode, pull, irq, alt)
@int pin gpio number, must be a numerical value
@any mode input and output mode:<br>The number 0/1 represents the output mode<br>nil represents the input mode<br>function represents the interrupt mode. If gpio.count is filled in, it is the interrupt counting function, and there is no callback during the interrupt.
@int pull pull-up and pull-down mode, which can be pull-up mode gpio.PULLUP or pull-down mode gpio.PULLDOWN, or open-drain mode 0. It needs to be selected according to the actual hardware
@int irq interrupt trigger mode, default gpio.BOTH. Interrupt trigger mode<br>Rising edge gpio.RISING<br>Falling edge gpio.FALLING<br>Both rising and falling trigger gpio.BOTH
@int alt Multiplexing option. Currently only the EC618 platform requires this parameter. Some GPIOs can be multiplexed to different pins. You can select the multiplexing option (0 or 4) to multiplex to the corresponding pins.
@return any Output mode returns a closure that sets the level, input mode and interrupt mode return a closure that gets the level
@usage

--Set gpio17 as input
gpio.setup(17, nil)

--Set gpio17 as output, and the initialization level is low, and use the hardware default pull-down configuration.
gpio.setup(17, 0)

--Set gpio17 as output, and the initialization level is high, and the internal pull-up is enabled
gpio.setup(17, 1, gpio.PULLUP)

--Set gpio27 as interrupt, bidirectional trigger by default
gpio.setup(27, function(val)
    print("IRQ_27",val) -- Reminder, val does not represent the trigger direction, but only represents the level at a certain point in time after the interruption
end, gpio.PULLUP)

--Set gpio27 as interrupt, only rising edge trigger
gpio.setup(27, function(val)
    print("IRQ_27",val) -- Reminder, val does not represent the trigger direction, but only represents the level at a certain point in time after the interruption
end, gpio.PULLUP, gpio.RISING)

-- Interrupt Count Added on 2024.5.8
--Set gpio7 as interrupt count, see gpio/gpio_irq_count for detailed demo
gpio.setup(7, gpio.count)

-- alt_func added in 2023.7.2
-- This function is only valid for some platforms and is only used to adjust GPIO reuse. For other reuse methods, please use the muc.iomux function.
--The following sample code multiplexes I2S_DOUT into gpio18
-- PIN33 (Modules pin number) of AIR780E corresponds to paddr 38. The default function is I2S_DOUT, which can be reused as gpio18.
-- Direction output, and the initialization level is low, using the hardware default pull-down configuration
-- Air780E (For GPIO reuse of the EC618 series, please refer to Air780E&Air780EG&Air780EX&Air700E_GPIO_table_20231227.pdf in the hardware information table on the homepage of https://air780e.cn)
-- Air780EP (For GPIO reuse of the EC718P series, please refer to Air780E&Air780EG&Air780EX&Air700E_GPIO_table_20231227.pdf in the hardware information table on the home page of https://air780ep.cn)
gpio.setup(18, 0, nil, nil, 4)

-- remind: 
-- When the pin is in input mode or interrupted, the level can be obtained through gpio.get()
-- When the pin is in output mode, the level can be set through gpio.set()
-- When the pin is in output mode, you will always get 0 through gpio.get()
-- The val parameter of the interrupt callback does not represent the trigger direction, but only represents the level at a certain point in time after the interrupt.
-- For Cat.1 Moduless, only AONGPIO of the EC618 series can be bidirectionally triggered, and all GPIOs of other series can be bidirectionally triggered. Please refer to the hardware manual for details.
-- Under the default settings, interrupts have no debounce time. You can set the debounce time through gpio.set_debounce(pin, 50)

--Additional description of pull parameters, pull-up/pull-down configuration
-- For some BSPs, only gpio.PULLUP or gpio.PULLDOWN is supported, but some BSPs support open-drain mode.
-- For bsps that support open drain, the pull parameter must be passed 0 to enable open drain mode, not nil.
-- For example:
-- EC618 series (Air780E/Air780EG/Air780EX/Air700E, etc.)
-- EC718 series (Air780EP/Air780EPV, etc.)
-- XT804 series (Air101/Air103/Air601)*/
static int l_gpio_setup(lua_State *L) {
    luat_gpio_t conf = {0};
    conf.pin = luaL_checkinteger(L, 1);
    if (conf.pin >= LUAT_GPIO_PIN_MAX) {
        LLOGW("MUST pin < %d", LUAT_GPIO_PIN_MAX);
        return 0;
    }
    //conf->mode = luaL_checkinteger(L, 2);
    conf.lua_ref = 0;
    conf.irq = 0;
    gpios[conf.pin].irq_type = 0xff;
    if (lua_isfunction(L, 2)) {
    	conf.irq_cb = 0;
        conf.mode = Luat_GPIO_IRQ;
        if (lua_tocfunction(L, 2) == l_gpio_get_count) {
        	if (gpios[conf.pin].lua_ref) {
                luaL_unref(L, LUA_REGISTRYINDEX, gpios[conf.pin].lua_ref);
                gpios[conf.pin].lua_ref = 0;
        	}
        	conf.irq_cb = luat_gpio_irq_count;
        	LLOGD("pin %d use irq count mode", conf.pin);
        } else {
            lua_pushvalue(L, 2);
            conf.lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        conf.irq = luaL_optinteger(L, 4, Luat_GPIO_BOTH);
        gpios[conf.pin].irq_type = conf.irq;

    }
    else if (lua_isinteger(L, 2)) {
        conf.mode = Luat_GPIO_OUTPUT;
        conf.irq = luaL_checkinteger(L, 2) == 0 ? 0 : 1; //Reuse irq as initial value
    }
    else {
        conf.mode = Luat_GPIO_INPUT;
    }
    conf.pull = luaL_optinteger(L, 3, default_gpio_pull);

    if (lua_isinteger(L, 5)) {
        conf.alt_func = luaL_checkinteger(L, 5);
    }
    else
    {
    	conf.alt_func = -1;
    }
    int re = luat_gpio_setup(&conf);
    if (re != 0) {
        LLOGW("gpio setup fail pin=%d", conf.pin);
        return 0;
    }
    if (conf.mode == Luat_GPIO_IRQ) {
        if (gpios[conf.pin].lua_ref && gpios[conf.pin].lua_ref != conf.lua_ref) {
            luaL_unref(L, LUA_REGISTRYINDEX, gpios[conf.pin].lua_ref);
            gpios[conf.pin].lua_ref = conf.lua_ref;
        }
        gpios[conf.pin].lua_ref = conf.lua_ref;
    }
    else if (conf.mode == Luat_GPIO_OUTPUT) {
        luat_gpio_set(conf.pin, conf.irq); // irq is reused as the initial value of OUTPUT
    }
    // Generate closure
    lua_settop(L, 1);
    if (conf.mode == Luat_GPIO_OUTPUT) {
        lua_pushcclosure(L, l_gpio_set, 1);
    }
    else {
        lua_pushcclosure(L, l_gpio_get, 1);
    }
    return 1;
}
static int cap_target_level;//Capture target level
static uint64_t rising_tick,falling_tick;//The system tick64 recorded when capturing the level
int l_caplevel_handler(lua_State *L, void* ptr) {
    (void)ptr; // unused
    //Send data to sys.publish method
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    int pin = msg->arg1;
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX)
        return 0;
    if (gpios[pin].lua_ref == 0)
        return 0;
    lua_geti(L, LUA_REGISTRYINDEX, gpios[pin].lua_ref);
    if (!lua_isnil(L, -1)) {
        if(cap_target_level == 1){
            lua_pushnumber(L, (float)(falling_tick-rising_tick)/luat_mcu_us_period());
        }else{
            lua_pushnumber(L, (float)(rising_tick-falling_tick)/luat_mcu_us_period());
        }
        lua_call(L, 1, 0);
    }  
    return 0;
}
int luat_caplevel_irq_cb(int pin, void* args) {
    rtos_msg_t msg = {0};
    msg.handler = l_caplevel_handler;
    msg.ptr = NULL;
    msg.arg1 = pin;
#ifdef LUAT_GPIO_PIN_MAX
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX) {
#else
    if (pin < 0 || pin >= Luat_GPIO_MAX_ID) {
#endif
        return 0;
    }
    luat_gpio_t conf={0};
    conf.pin = pin;
    conf.mode = Luat_GPIO_IRQ;
    conf.irq_cb = luat_caplevel_irq_cb;
    conf.alt_func = -1;
    conf.pull=Luat_GPIO_DEFAULT;
    if(gpios[pin].irq_type == Luat_GPIO_RISING){
        rising_tick = luat_mcu_tick64();
        conf.irq =Luat_GPIO_FALLING;
        luat_gpio_setup(&conf);
        gpios[pin].irq_type = Luat_GPIO_FALLING;
        if(cap_target_level == 1){
            return 1;
        }else{
            return luat_msgbus_put(&msg, 0); 
        }
        
    }else{
        falling_tick = luat_mcu_tick64();
        conf.irq =Luat_GPIO_RISING;
        luat_gpio_setup(&conf);
        gpios[pin].irq_type = Luat_GPIO_RISING;
        if(cap_target_level == 1){
            return luat_msgbus_put(&msg, 0);
        }else{
            return 1;
        }
    }

}
/*The duration of the capture pin level, in us
@api gpio.caplevel(pin, level,func)
@int pin GPIO number, must be a numerical value
@int level The level that needs to be captured can be high level gpio.HIGH, low level gpio.LOW, or directly write the value 1 or 0, that is, the pin is at the opposite level of the normal time, and the level set by the capture continues time
@function func is the callback function after the capture is completed. It has only one parameter. The parameter is the number type value of the captured time length. The unit is us.
@return any Returns the closure that obtains the level
@usage
-- Capture the duration of pin.PA07 being high level
gpio.caplevel(pin.PA07,1,function(val) print(val) end)*/
static int l_gpio_caplevel(lua_State *L){
    luat_gpio_t conf = {0};
    conf.pin = luaL_checkinteger(L, 1);
    cap_target_level = luaL_checkinteger(L,2);
    //According to the target level, configure the edge that the pin processes first
    if(cap_target_level == 1){//The goal is to capture the high level
        conf.irq = Luat_GPIO_RISING;//The pin first processes the rising edge
    }else{
        conf.irq = Luat_GPIO_FALLING;
    }
    conf.mode=Luat_GPIO_IRQ;
    if (lua_isfunction(L, 3)) {
        lua_pushvalue(L, 3);
        conf.lua_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        if (gpios[conf.pin].lua_ref && gpios[conf.pin].lua_ref != conf.lua_ref) {
            luaL_unref(L, LUA_REGISTRYINDEX, gpios[conf.pin].lua_ref);
        }
        gpios[conf.pin].irq_type = conf.irq;
        gpios[conf.pin].lua_ref = conf.lua_ref;
    }else{
        return 0;
    }
    conf.irq_cb = luat_caplevel_irq_cb;
    int re = luat_gpio_setup(&conf);
    if (re != 0) {
        LLOGW("gpio setup fail pin=%d", conf.pin);
        return 0;
    }
    // Generate closure
    lua_settop(L, 1);
    lua_pushcclosure(L, l_gpio_get, 1);
    return 1;
}
/*Set pin level
@api gpio.set(pin, value)
@int pin GPIO number, must be a numerical value
@int value level, which can be high level gpio.HIGH, low level gpio.LOW, or directly write the value 1 or 0
@return nil no return value
@usage
--Set gpio17 to low level
gpio.set(17, 0)*/
static int l_gpio_set(lua_State *L) {
    int pin = 0;
    int value = 0;
    if (lua_isinteger(L, lua_upvalueindex(1))) {
        pin = lua_tointeger(L, lua_upvalueindex(1));
        value = luaL_checkinteger(L, 1);
    }
    else {
        pin = luaL_checkinteger(L, 1);
        value = luaL_checkinteger(L, 2);
    }
    luat_gpio_set(pin, value);
    gpio_bit_set(pin, (uint8_t)value);
    return 0;
}

/*Get pin level
@api gpio.get(pin)
@int pin GPIO number, must be a numerical value
@return value level, high level gpio.HIGH, low level gpio.LOW, corresponding to values   1 and 0
@usage
-- Get the current level of gpio17
gpio.get(17)*/
static int l_gpio_get(lua_State *L) {
    if (lua_isinteger(L, lua_upvalueindex(1)))
        lua_pushinteger(L, luat_gpio_get(luaL_checkinteger(L, lua_upvalueindex(1))) & 0x01 ? 1 : 0);
    else
        lua_pushinteger(L, luat_gpio_get(luaL_checkinteger(L, 1)) & 0x01 ? 1 : 0);
    return 1;
}

/*Turn off pin function (high impedance input state) and turn off interrupts
@api gpio.close(pin)
@int pin GPIO number, must be a numerical value
@return nil no return value, always executed successfully
@usage
-- Close gpio17
gpio.close(17)*/
static int l_gpio_close(lua_State *L) {
    int pin = luaL_checkinteger(L, 1);
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX)
        return 0;
    luat_gpio_close(pin);
    if (gpios[pin].lua_ref) {
        luaL_unref(L, LUA_REGISTRYINDEX, gpios[pin].lua_ref);
        gpios[pin].lua_ref = 0;
    }
#ifndef LUAT_RTOS_API_NOTOK
    if (gpios[pin].timer != NULL) {
        gpios[pin].conf_tick = 0;
        luat_rtos_timer_stop(gpios[pin].timer);
        luat_rtos_timer_delete(gpios[pin].timer);
        gpios[pin].timer = NULL;
    }
#endif
    return 0;
}

/*Set the default pull-up/pull-down setting of the GPIO pin. The default is platform customized (usually open drain).
@api gpio.setDefaultPull(val)
@int val 0 platform customization, 1 pull-up, 2 pull-down
@return boolean returns true if the passed value is correct, otherwise returns false
@usage
--Set the default value of pull of gpio.setup to pull-up
gpio.setDefaultPull(1)*/
static int l_gpio_set_default_pull(lua_State *L) {
    int value = luaL_checkinteger(L, 1);
    if (value >= 0 && value <= 2) {
        default_gpio_pull = value;
        lua_pushboolean(L, 1);
    }
    else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

/*Change the GPIO pin output level, only output mode is available
@api gpio.toggle(pin)
@int GPIO number of pin
@return nil no return value
@usage
--This API was added on 2022.05.17
-- Assuming there is an LED on GPIO16, the switch is switched every 500ms
gpio.setup(16, 0)
sys.timerLoopStart(function()
    gpio.toggle(16)
end, 500)*/
static int l_gpio_toggle(lua_State *L) {
    int pin = 0;
    if (lua_isinteger(L, lua_upvalueindex(1)))
        pin = lua_tointeger(L, lua_upvalueindex(1));
    else
        pin = luaL_checkinteger(L, 1);
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX) {
        LLOGW("pin id out of range (0-127)");
        return 0;
    }
    uint8_t value = gpio_bit_get(pin);
    luat_gpio_set(pin, value == 0 ? Luat_GPIO_HIGH : Luat_GPIO_LOW);
    gpio_bit_set(pin, value == 0 ? 1 : 0);
    return 0;
}

/*Output a group of pulses on the same GPIO. Note that the unit of len is bit, with the high-order bit first.
@api gpio.pulse(pin,level,len,delay)
@int gpio number
@int/string Numeric value or string.
@int len   length unit is bit, high bit first.
@int delay delay, currently there is no fixed time unit
@return nil no return value
@usage
-- Output 8 level changes through the PB06 pin output.
gpio.pulse(pin.PB06,0xA9, 8, 0)*/
static int l_gpio_pulse(lua_State *L) {
    int pin,delay = 0;
    char tmp = 0;
    size_t len = 0;
    char* level = NULL;
    if (lua_isinteger(L, lua_upvalueindex(1))){
        pin = lua_tointeger(L, lua_upvalueindex(1));
        if (lua_isinteger(L, 1)){
            tmp = (char)luaL_checkinteger(L, 1);
            level = &tmp;
        }else if (lua_isstring(L, 1)){
            level = (char*)luaL_checklstring(L, 1, &len);
        }
        len = luaL_checkinteger(L, 2);
        delay = luaL_checkinteger(L, 3);
    } else {
        pin = luaL_checkinteger(L, 1);
        if (lua_isinteger(L, 2)){
            tmp = (char)luaL_checkinteger(L, 2);
            level = &tmp;
        }else if (lua_isstring(L, 2)){
            level = (char*)luaL_checklstring(L, 2, &len);
        }
        len = luaL_checkinteger(L, 3);
        delay = luaL_checkinteger(L, 4);
    }
    if (pin < 0 || pin >= LUAT_GPIO_PIN_MAX) {
        LLOGD("pin id out of range (0-127)");
        return 0;
    }
    luat_gpio_pulse(pin,(uint8_t*)level,len,delay);
    return 0;
}

/*Anti-shake settings, anti-shake based on hardware ticks
@api gpio.debounce(pin, ms, mode)
@int gpio number, 0~127, related to hardware
@int Anti-shake duration, unit millisecond, maximum 65555 ms, set to 0 to turn off
@int mode, 0 cooling mode, 1 delay mode. Default is 0
@return nil no return value
@usage
--Debounce mode, currently supports 2 types, mode=1 will be supported from 2022.12.16
-- 0 After triggering the interrupt, report it immediately, and then wait for N milliseconds before accepting the interrupt again.
-- 1 After triggering the interrupt, delay for N milliseconds. During this period, there is no new interrupt and the level does not change. Report once.

-- Turn on anti-shake, mode 0-cooling, report immediately after interruption, but only report once in 100ms
gpio.debounce(7, 100) -- If the chip supports the pin library, pin.PA7 can be used instead of the number 7
-- Turn on anti-shake, mode 1-delay, wait for 100ms after interruption, if the level is maintained during the period, report once after the time is up
-- Correspondingly, if the input is a 50hz square wave, no report will be triggered.
gpio.debounce(7, 100, 1)

-- Turn off anti-shake, set the time to 0 and turn it off
gpio.debounce(7, 0)*/
static int l_gpio_debounce(lua_State *L) {
    uint8_t pin = luaL_checkinteger(L, 1);
    uint16_t timeout = luaL_checkinteger(L, 2);
    uint8_t mode = luaL_optinteger(L, 3, 0);
    if (pin >= LUAT_GPIO_PIN_MAX) {
        LLOGW("MUST pin < %d", LUAT_GPIO_PIN_MAX);
        return 0;
    }
    //LLOGD("debounce %d %d %d", pin, timeout, mode);
    gpios[pin].conf_tick = timeout;
    gpios[pin].latest_tick = 0;
    gpios[pin].debounce_mode = mode;
#ifndef LUAT_RTOS_API_NOTOK
    if ((mode == 0 && gpios[pin].timer != NULL) || timeout == 0) {
        luat_rtos_timer_stop(gpios[pin].timer);
        luat_rtos_timer_delete(gpios[pin].timer);
        gpios[pin].timer = NULL;
    }
    else if (mode == 1 && gpios[pin].timer == NULL && timeout > 0) {
        //LLOGD("GPIO debounce mode 1 %d %d", pin, timeout);
        if (gpios[pin].timer == NULL)
            luat_rtos_timer_create(&gpios[pin].timer);
        if (gpios[pin].timer == NULL) {
            LLOGE("out of memory when malloc debounce timer");
            return 0;
        }
    }
#endif
    return 0;
}

/*Get the number of gpio interrupts and clear the accumulated value, similar to the pulse count of air724
@api gpio.count(pin)
@int gpio number, 0~127, related to hardware
@return int Returns the current interrupt count since the last time the interrupt number was obtained
@usage
log.info("irq cnt", gpio.count(10))*/
static int l_gpio_get_count(lua_State *L) {
    uint8_t pin = luaL_checkinteger(L, 1);
    if (pin >= LUAT_GPIO_PIN_MAX) {
        LLOGW("MUST pin < %d", LUAT_GPIO_PIN_MAX);
        lua_pushinteger(L, 0);
        return 1;
    }
    uint32_t v,cr;
    cr = luat_rtos_entry_critical();
    v = gpios[pin].irq_cnt;
    gpios[pin].irq_cnt = 0;
    luat_rtos_exit_critical(cr);
    lua_pushinteger(L, v);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_gpio[] =
{
    { "setup" ,         ROREG_FUNC(l_gpio_setup )},
    { "set" ,           ROREG_FUNC(l_gpio_set)},
    { "get" ,           ROREG_FUNC(l_gpio_get)},
    { "close" ,         ROREG_FUNC(l_gpio_close)},
    { "toggle",         ROREG_FUNC(l_gpio_toggle)},
    { "debounce",       ROREG_FUNC(l_gpio_debounce)},
    { "pulse",          ROREG_FUNC(l_gpio_pulse)},
    { "setDefaultPull", ROREG_FUNC(l_gpio_set_default_pull)},
	{ "count",			ROREG_FUNC(l_gpio_get_count)},
#ifdef LUAT_USE_MCU
    { "caplevel" ,      ROREG_FUNC(l_gpio_caplevel)},
#endif
    //@const NONE number Invalid pin, generally used to tell the bottom layer that a certain function pin is not specified
    { "NONE",          ROREG_INT(LUAT_GPIO_NONE)},
    //@const LOW number low level
    { "LOW",            ROREG_INT(Luat_GPIO_LOW)},
    //@const HIGH number high level
    { "HIGH",           ROREG_INT(Luat_GPIO_HIGH)},

    { "OUTPUT",         ROREG_INT(Luat_GPIO_OUTPUT)}, // Reserved for compatibility

    //@const PULLUP number Pull-up
    { "PULLUP",         ROREG_INT(Luat_GPIO_PULLUP)},
    //@const PULLDOWN number drop-down
    { "PULLDOWN",       ROREG_INT(Luat_GPIO_PULLDOWN)},

    //@const RISING number rising edge trigger
    { "RISING",         ROREG_INT(Luat_GPIO_RISING)},
    //@const FALLING number falling edge trigger
    { "FALLING",        ROREG_INT(Luat_GPIO_FALLING)},
    //@const BOTH number Bidirectional trigger, supported by some devices
    { "BOTH",           ROREG_INT(Luat_GPIO_BOTH)},
    //@const HIGH_IRQ number High level trigger, supported by some devices
    { "HIGH_IRQ",       ROREG_INT(Luat_GPIO_HIGH_IRQ)},
    //@const LOW_IRQ number Low level trigger, supported by some devices
    { "LOW_IRQ",        ROREG_INT(Luat_GPIO_LOW_IRQ)},
	{ NULL,             ROREG_INT(0) }
};

LUAMOD_API int luaopen_gpio( lua_State *L ) {
    memset(gpios, 0, sizeof(gpio_ctx_t) * LUAT_GPIO_PIN_MAX);
    luat_newlib2(L, reg_gpio);
    return 1;
}

//-------------------- Some auxiliary functions

void luat_gpio_mode(int pin, int mode, int pull, int initOutput) {
    if (pin == 255) return;
    luat_gpio_t conf = {0};
    conf.pin = pin;
    conf.mode = mode == Luat_GPIO_INPUT ? Luat_GPIO_INPUT : Luat_GPIO_OUTPUT; // Can only be input/output, not interrupt.
    conf.pull = pull;
    conf.irq = initOutput;
    conf.lua_ref = 0;
    conf.irq_cb = 0;
    conf.alt_func = -1;
    luat_gpio_setup(&conf);
    if (conf.mode == Luat_GPIO_OUTPUT)
        luat_gpio_set(pin, initOutput);
}

#ifndef LUAT_COMPILER_NOWEAK
void LUAT_WEAK luat_gpio_pulse(int pin, uint8_t *level, uint16_t len, uint16_t delay_ns) {

}
#endif
