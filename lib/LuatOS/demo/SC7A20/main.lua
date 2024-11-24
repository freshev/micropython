
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "sc7a20_demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000) --Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000) --Feed the watchdog once every 3 seconds
end
 
local sc7a20 = require "sc7a20"

i2cid = 0
i2c_speed = i2c.FAST

gpio.setup(24, function() --Configuration interrupt, used for external wake-up
    log.info("gpio 24")
end, gpio.PULLUP, gpio.BOTH)

gpio.setup(2, function() --Configuration interrupt, used for external wake-up
    log.info("gpio 2")
end, gpio.PULLUP, gpio.BOTH)

sys.taskInit(function()
    i2c.setup(i2cid, i2c_speed)

    sc7a20.init(i2cid)--Initialization, pass in i2c_id

    sys.wait(50)
    sc7a20.set_thresh(i2cid, string.char(0x05), string.char(0x05))  --Set activity threshold
    sys.wait(50)
    sc7a20.set_irqf(i2cid, 1, string.char(0x1F), string.char(0x03), string.char(0xFF))  --AOI1 interrupt is mapped to INT1
    sys.wait(50)
    sc7a20.set_irqf(i2cid, 2, string.char(0x1F), string.char(0x03), string.char(0xFF))  --AOI2 interrupt is mapped to INT2

    while 1 do
        local sc7a20_data = sc7a20.get_data()
        log.info("sc7a20_data", "sc7a20_data.x"..(sc7a20_data.x),"sc7a20_data.y"..(sc7a20_data.y),"sc7a20_data.z"..(sc7a20_data.z))
        sys.wait(1000)
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
