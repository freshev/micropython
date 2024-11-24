
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "adxl345_demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000) --Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000) --Feed the watchdog once every 3 seconds
end

local adxl34x = require "adxl34x"

i2cid = 0
i2c_speed = i2c.FAST

gpio.setup(24, function() --Configure wakeup interrupt for external wake-up. Connect ADXL34X INT1 pin
    log.info("gpio ri")
end, gpio.PULLUP, gpio.FALLING)

sys.taskInit(function()
    i2c.setup(i2cid, i2c_speed)

    adxl34x.init(i2cid)--Initialization, pass in i2c_id

    sys.wait(50)
    adxl34x.set_thresh(i2cid, string.char(0x05), string.char(0x02), string.char(0x05))  --Set threshold
    sys.wait(50)
    adxl34x.set_irqf(i2cid, string.char(0x00), string.char(0xff), string.char(0x10))     --The activity is mapped to INT1 and the corresponding interrupt function is enabled.

    while 1 do
        adxl34x.get_int_source(i2cid)    --Not adding this will not trigger an interrupt
        
        local adxl34x_data = adxl34x.get_data()
        log.info("adxl34x_data", "adxl34x_data.x"..(adxl34x_data.x),"adxl34x_data.y"..(adxl34x_data.y),"adxl34x_data.z"..(adxl34x_data.z))
        sys.wait(1000)
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
