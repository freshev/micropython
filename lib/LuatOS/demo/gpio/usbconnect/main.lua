
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "usb_connect_demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

local rtos_bsp = rtos.bsp()

local function pinx()
    if rtos_bsp == "EC618" then --AIR780E -- 33 is virtual GPIO, see https://wiki.luatos.com/chips/air780e/iomux.html#id1
        return 24, 33           
    elseif rtos_bsp == "EC718P" then --AIR780EP -- 40 is virtual GPIO
        return 27, 40
    else
        return 255, 255
    end
end


local led_pin, vbus_pin = pinx()                                    --Assign LED, vbus pin number

if led_pin ~= 255 and vbus_pin ~= 255 then
    local led = gpio.setup(led_pin, 1) --If you really pull out the USB, you may not be able to print out the information, so get an IO output with the same level as the USB
    led(gpio.get(vbus_pin)) --IO output is in the same status as USB
    gpio.setup(vbus_pin, function() 
        log.info("usb", gpio.get(vbus_pin))
        led(gpio.get(vbus_pin))   --IO output is in the same status as USB
    end, gpio.PULLUP, gpio.BOTH)
    gpio.debounce(vbus_pin, 500, 1)  --The purpose of adding debounce is to see the output as much as possible
    log.info("usb", gpio.get(vbus_pin))
else
    log.info("bsp not support") 
end

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
