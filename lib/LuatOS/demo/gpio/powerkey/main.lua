
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "pwrkey_demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

local rtos_bsp = rtos.bsp()

local function pinx()
    if rtos_bsp == "EC618" then --AIR780E -- 35 is a virtual GPIO, see https://wiki.luatos.com/chips/air780e/iomux.html#id1
        return 35           
    elseif rtos_bsp == "EC718P" then --AIR780EP -- 46 is virtual GPIO
        return 46
    else
        return 255 
    end
end


local powerkey_pin = pinx()                                         --Assign powerkey pin number

if powerkey_pin ~= 255 then
    gpio.setup(powerkey_pin, function() 
        log.info("pwrkey", gpio.get(powerkey_pin))
    end, gpio.PULLUP)
else
    log.info("bsp not support")
end


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
