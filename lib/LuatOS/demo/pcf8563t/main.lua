
PROJECT = "pcf8563t"
VERSION = "2.0.0"

_G.sys = require "sys"

_G.pcf8563t = require "pcf8563t"

local function PCF8563T()

    sys.wait(3000)
    
    local i2cid = 1
    i2c.setup(i2cid, i2c.FAST)
    pcf8563t.setup(i2cid) --Select an i2c, or it can be a software i2c object
    
    --Set time
    local time = {year=2023,mon=11,day=2,wday=5,hour=13,min=14,sec=15}
    pcf8563t.write(time)
    
    --Reading time
    local time = pcf8563t.read()
    log.info("time",time.year,time.mon,time.day, time.hour,time.min,time.sec, "week", time.wday)
    
    --Set the alarm clock, automatically clear the interrupt flag, and enable the alarm function
    alarm = {day=2,hour=13,min=14,sec=15}
    pcf8563t.alarm(alarm)
    local alarm_int = 1 --Select a GPIO and connect it to the INT pin of the clock Modules
    gpio.setup(1, function()
        log.info("alarm!!!")
        pcf8563t.control(nil, nil, 0, nil)
    end, gpio.PULLUP)
end
sys.taskInit(PCF8563T)

sys.run()
