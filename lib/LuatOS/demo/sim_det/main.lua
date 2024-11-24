
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "simdetdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Configure gpio to input mode, pull down, and trigger an interrupt
--Please change the gpio number and pull-down according to actual needs
--SIMDET configuration is 34
local gpio_pin = 34
gpio.debounce(gpio_pin, 200, 1)
gpio.setup(gpio_pin, function(val)
	local status = gpio.get(gpio_pin)
    log.info("gpio", "IRQ",gpio_pin, val, status)
	if status == 0 then
		log.info("SIM REMOVE")
		mobile.flymode(0, true)	
	else
		log.info("SIM READY")
		mobile.flymode(0, false)	
	end	
end, gpio.PULLUP) 

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
