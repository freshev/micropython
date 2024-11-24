
/*@Modules just kidding
@summary io sequence operation
@version 1.0
@date 2022.03.13
@demo io_queue
@tag LUAT_USE_IO_QUEUE*/
#include "luat_base.h"
#include "luat_multimedia.h"
#include "luat_msgbus.h"
#include "luat_zbuff.h"
#include "luat_lib_io_queue.h"
#define LUAT_LOG_TAG "io_queue"
#include "luat_log.h"
int l_io_queue_done_handler(lua_State *L, void* ptr)
{
	rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
	lua_pop(L, 1);
	lua_getglobal(L, "sys_pub");
	lua_pushfstring(L, "IO_QUEUE_DONE_%d", msg->ptr);
	lua_call(L, 1, 0);
	return 1;
}

int l_io_queue_capture_handler(lua_State *L, void* ptr)
{
	volatile uint64_t tick;
	uint32_t pin;
	uint32_t val;
	rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
	lua_getglobal(L, "sys_pub");
	pin = ((uint32_t)msg->ptr >> 8) & 0x000000ff;
	val = (uint32_t)msg->ptr & 0x00000001;
	tick = ((uint64_t)msg->arg1 << 32) | (uint32_t)msg->arg2;
//	LLOGD("%u, %x,%x, %llu",pin, msg->arg1, msg->arg2, tick);
	lua_pushfstring(L, "IO_QUEUE_EXTI_%d", pin);
	lua_pushinteger(L, val);
	lua_pushlstring(L, &tick, 8);
	lua_call(L, 3, 0);
	return 1;
}

/*Initialize an io operation queue
@api ioqueue.init(hwtimer_id,cmd_cnt,repeat_cnt)
@int Hardware timer id, the default is 0, determined according to the actual MCU, air105 is 0~5, shared with pwm, the same channel number cannot be for pwm and ioqueue at the same time
@int The commands required for a complete cycle can be more than the actual ones
@int The number of repetitions, the default is 1, if you write 0, it means an unlimited number of loops
@return None
@usage
ioqueue.init(0,10,5) -- Initialize an io operation queue with timer0 as the clock source, with 10 valid commands, looping 5 times*/
static int l_io_queue_init(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	int cmd_cnt = luaL_optinteger(L, 2, 0);
	int repeat_cnt = luaL_optinteger(L, 3, 1);
	luat_io_queue_init(timer_id, cmd_cnt, repeat_cnt);
	return 0;
}

/*Add delay command to io operation queue
@api ioqueue.setdelay(hwtimer_id,time_us,time_tick,continue)
@int hardware timer id
@int delay time, 0~65535us
@int Delay fine-tuning time, 0~255tick, the total delay time is time_us * 1us_tick + time_tick
@boolean Whether it is a continuous delay, the default is no, if so, the timer will not stop but will retime after the time is up.
In this way, the same time delay will be repeated every time delay is called before the next setdelay command, improving the accuracy of continuous timing.
@return None
@usage
ioqueue.setdelay(0,10,0) --delay 10us+0 ticks
ioqueue.setdelay(0,9,15,true) --Delay 9us+15 ticks. When encountering the delay command later, it will be delayed 9us+15 ticks.*/
static int l_io_queue_set_delay(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	uint16_t us_delay = luaL_optinteger(L, 2, 1);
	uint8_t us_delay_tick = luaL_optinteger(L, 3, 0);
	uint8_t is_continue = 0;
	if (lua_isboolean(L, 4))
	{
		is_continue = lua_toboolean(L, 4);
	}
	luat_io_queue_set_delay(timer_id, us_delay, us_delay_tick, is_continue);
	return 0;
}

/*Add a repeated delay to the io operation queue. There must be setdelay in front and it must be a continuous delay.
@api ioqueue.delay(hwtimer_id)
@int hardware timer id
@return None
@usage
ioqueue.setdelay(0,9,15,true) --Delay 9us+15 ticks. When encountering the delay command later, it will be delayed 9us+15 ticks.
ioqueue.delay(0)*/
static int l_io_queue_delay(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	luat_io_queue_repeat_delay(timer_id);
	return 0;
}



/*Add the set gpio command to the io operation queue
@api ioqueue.setgpio(hwtimer_id,pin,is_input,pull_mode,init_level)
@int hardware timer id
@int pin
@boolean whether it is input
@int Pull-down mode can only be 0, gpio.PULLUP, gpio.PULLDOWN
@int initial output level
@return None
@usage
ioqueue.setgpio(0,pin.PB01,true,gpio.PULLUP,0) --PB01 is set to pull-up input
ioqueue.setgpio(0,pin.PB01,false,0,1)--PB01 is set to the default pull-down output high level*/
static int l_io_queue_set_gpio(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	uint8_t pin = luaL_checkinteger(L, 2);
	uint8_t pull_mode = luaL_optinteger(L, 4, 0);
	uint8_t init_level = luaL_optinteger(L, 5, 0);
	uint8_t is_input = 0;
	if (lua_isboolean(L, 3))
	{
		is_input = lua_toboolean(L, 3);
	}

	luat_io_queue_add_io_config(timer_id, pin, is_input, pull_mode, init_level);
	return 0;
}

/*Add read gpio command to io operation queue
@api ioqueue.input(hwtimer_id,pin)
@int hardware timer id
@int pin
@return None
@usage
ioqueue.input(0,pin.PB01)*/
static int l_io_queue_gpio_input(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	uint8_t pin = luaL_checkinteger(L, 2);
	luat_io_queue_add_io_in(timer_id, pin, NULL, NULL);
	return 0;
}

/*Add output GPIO command to io operation queue
@api ioqueue.output(hwtimer_id,pin,level)
@int hardware timer id
@int pin
@int output level
@return None
@usage
ioqueue.output(0,pin.PB01,0)*/
static int l_io_queue_gpio_output(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	uint8_t pin = luaL_checkinteger(L, 2);
	uint8_t level = luaL_optinteger(L, 3, 0);
	luat_io_queue_add_io_out(timer_id, pin, level);
	return 0;
}

/*Add settings to the io operation queue to capture an IO command
@api ioqueue.set_cap(hwtimer_id,pin,pull_mode,irq_mode,max_tick)
@int hardware timer id
@int pin
@int Pull-down mode can only be 0, gpio.PULLUP, gpio.PULLDOWN
@int interrupt mode, can only be gpio.BOTH, gpio.RISING, gpio.FALLING
@int The maximum timing time of the timer. Considering that Lua is an int type, the minimum value is 0x10000 and the maximum value is 0x7fffffff. The default is the maximum value.
@return None
@usage
ioqueue.setcap(0,pin.PB01,gpio.PULLUP,gpio.FALLING,48000000)*/
static int l_io_queue_set_capture(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	uint8_t pin = luaL_checkinteger(L, 2);
	uint8_t pull_mode = luaL_optinteger(L, 3, 0);
	uint8_t irq_mode = luaL_optinteger(L, 4, 0);
	int max_cnt = luaL_optinteger(L, 5, 0x7fffffff);
	if (max_cnt < 65536)
	{
		max_cnt = 65536;
	}
	luat_io_queue_capture_set(timer_id, max_cnt, pin, pull_mode, irq_mode);
	return 0;
}

/*Add a capture IO status command to the io operation queue
@api ioqueue.capture(hwtimer_id)
@int hardware timer id
@return None
@usage
ioqueue.capture(0)*/
static int l_io_queue_capture_pin(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	luat_io_queue_capture(timer_id, NULL, NULL);
	return 0;
}

/*Add an IO operation queue to end capturing an IO command
@api ioqueue.capend(hwtimer_id,pin)
@int hardware timer id
@int pin
@return None
@usage
ioqueue.capend(0,pin.PB01)*/
static int l_io_queue_capture_end(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	uint8_t pin = luaL_checkinteger(L, 2);
	luat_io_queue_capture_end(timer_id, pin);
	return 0;
}

/** Get input and captured data in the io operation queue
@api ioqueue.get(hwtimer_id, input_buff, capture_buff)
@int hardware timer id
@zbuff is a buff that stores IO input data. The data is stored in the form of 1byte pin + 1byte level.
@zbuff is a buff that stores IO capture data. The data is stored in the form of 1byte pin + 1byte level + 4byte tick.
@return int How many sets of IO input data are returned
@return int How many sets of IO capture data are returned
@usage
local input_cnt, capture_cnt = ioqueue.get(0, input_buff, capture_buff)*/
static int l_io_queue_get(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	luat_zbuff_t *buff1 = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
	luat_zbuff_t *buff2 = ((luat_zbuff_t *)luaL_checkudata(L, 3, LUAT_ZBUFF_TYPE));
	uint32_t input_cnt, capture_cnt;
	uint32_t size = luat_io_queue_get_size(timer_id);
	if (buff1->len < (size * 2)) __zbuff_resize(buff1, (size * 2));
	if (buff2->len < (size * 6)) __zbuff_resize(buff2, (size * 6));
	luat_io_queue_get_data(timer_id, buff1->addr, &input_cnt, buff2->addr, &capture_cnt);
	buff1->used = input_cnt * 2;
	buff2->used = capture_cnt * 6;
	lua_pushinteger(L, input_cnt);
	lua_pushinteger(L, capture_cnt);
	return 2;
}

/*Start io operation queue
@api ioqueue.start(hwtimer_id)
@int hardware timer id
@return None
@usage
ioqueue.start(0)*/
static int l_io_queue_start(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	luat_io_queue_start(timer_id);
	return 0;
}

/*Stop the io operation queue and start from scratch via start
@api ioqueue.stop(hwtimer_id)
@int hardware timer id
@return int Returns the number of times it has been cycled. If it is 0, it means that even one cycle has not been completed.
@return int Returns the number of times cmd has been executed in a single loop. If it is 0, it may be that a loop has just ended.
@usage
ioqueue.stop(0)*/
static int l_io_queue_stop(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	uint32_t repeat_cnt = 0;
	uint32_t cmd_cnt = 0;
	luat_io_queue_stop(timer_id, &repeat_cnt, &cmd_cnt);
	lua_pushinteger(L, repeat_cnt);
	lua_pushinteger(L, cmd_cnt);
	return 2;
}


/*Release the resources of the io operation queue. You must re-init the next time you use it.
@api ioqueue.release(hwtimer_id)
@int hardware timer id
@return None
@usage
ioqueue.clear(0)*/
static int l_io_queue_release(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	luat_io_queue_release(timer_id);
	return 0;
}

/*Clear the io operation queue
@api ioqueue.clear(hwtimer_id)
@int hardware timer id
@return None
@usage
ioqueue.clear(0)*/
static int l_io_queue_clear(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	luat_io_queue_clear(timer_id);
	return 0;
}

/*Check whether the io operation queue has been completed
@api ioqueue.done(hwtimer_id)
@int hardware timer id
@return boolean Whether the queue execution is completed,
@usage
local result = ioqueue.done(0)*/
static int l_io_queue_is_done(lua_State *L) {
	uint8_t timer_id = luaL_optinteger(L, 1, 0);
	lua_pushboolean(L, luat_io_queue_check_done(timer_id));
	return 1;
}

/*Start/stop an external interrupt with system tick return
@api ioqueue.exti(pin,pull_mode,irq_mode,onoff)
@int pin
@int Pull-down mode can only be 0, gpio.PULLUP, gpio.PULLDOWN
@int interrupt mode, can only be gpio.BOTH, gpio.RISING, gpio.FALLING
@boolean switch, the default is false
@return None
@usage
ioqueue.exti(pin.PB01, gpio.PULLUP, gpio.BOTH, true)
ioqueue.exti(pin.PB01)*/
static int l_io_queue_exti(lua_State *L) {
	uint8_t pin = luaL_checkinteger(L, 1);
	uint8_t pull_mode = luaL_optinteger(L, 2, 0);
	uint8_t irq_mode = luaL_optinteger(L, 3, 0);
	uint8_t on_off = 0;
	if (lua_isboolean(L, 4)) {
		on_off = lua_toboolean(L, 4);
	}
	if (on_off) {
		luat_io_queue_capture_start_with_sys_tick(pin, pull_mode, irq_mode);
	} else {
		luat_io_queue_capture_end_with_sys_tick(pin);
	}
	return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_io_queue[] =
{
    { "init" ,       ROREG_FUNC(l_io_queue_init)},
    { "setdelay" ,   ROREG_FUNC(l_io_queue_set_delay)},
	{ "delay" ,      ROREG_FUNC(l_io_queue_delay)},
	{ "setgpio",	 ROREG_FUNC(l_io_queue_set_gpio)},
	{ "input",		 ROREG_FUNC(l_io_queue_gpio_input)},
	{ "output",      ROREG_FUNC(l_io_queue_gpio_output)},
	{ "set_cap",     ROREG_FUNC(l_io_queue_set_capture)},
	{ "capture",     ROREG_FUNC(l_io_queue_capture_pin)},
	{ "cap_done",    ROREG_FUNC(l_io_queue_capture_end)},
	{ "clear",	     ROREG_FUNC(l_io_queue_clear)},
    { "start",       ROREG_FUNC(l_io_queue_start)},
    { "stop",        ROREG_FUNC(l_io_queue_stop)},
	{ "done",        ROREG_FUNC(l_io_queue_is_done)},
	{ "get",         ROREG_FUNC(l_io_queue_get)},
	{ "release",     ROREG_FUNC(l_io_queue_release)},
	{ "exti",        ROREG_FUNC(l_io_queue_exti)},
	{ NULL,          {}}
};

LUAMOD_API int luaopen_io_queue( lua_State *L ) {
    luat_newlib2(L, reg_io_queue);
    return 1;
}
