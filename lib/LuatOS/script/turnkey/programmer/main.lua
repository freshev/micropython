--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "download"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
local sys = require "sys"

local air101 = require "air101"

gpio.setup(pin.PA10, function()
    sys.publish("download_force")
end, gpio.PULLUP,gpio.RISING)

sys.taskInit(function()
    led_func= gpio.setup(pin.PB03, 0,gpio.PULLUP)
    led_func(1)
    while 1 do
        air101.download(pin.PB05,pin.PB02,1,"/luadb/AIR101.fls","1.0.8",led_func)
        local result = sys.waitUntil("download_force", 1000)
        if result and air101.download_state==false then
            log.info("download_force")
            air101.download_force(pin.PB05,pin.PB02,1,"/luadb/AIR101.fls","1.0.8",led_func)
        end
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
