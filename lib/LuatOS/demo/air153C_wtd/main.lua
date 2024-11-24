PROJECT = 'air153C_wtd'
VERSION = '2.0.0'
LOG_LEVEL = log.LOG_INFO
log.setLevel(LOG_LEVEL )
require 'air153C_wtd'
local sys = require "sys"
_G.sysplus = require("sysplus")

--[[
    对于喂狗以及关闭喂狗，调用函数时需要等待对应的时间
    例如:   1. 喂狗是410ms，那么需要等待至少400ms，即
            air153C_wtd.feed_dog(pin)
            sys.wait(410ms)
            2. 关闭喂狗是710ms，那么需要等待至少700ms
            air153C_wtd.close_watch_dog(pin)
            sys.wait(710ms)
]]

sys.taskInit(function ()
    log.info("main","taskInit")
    local flag = 0
    air153C_wtd.init(28)
    air153C_wtd.feed_dog(28)--The first step to start the Modules is to feed the watchdog once
    sys.wait(3000)--There is a delay of 3 seconds here to prevent feeding the watchdog twice within 1 second and causing the watchdog to enter the test mode.


    --Don't feed the dog
    log.info("WTD","not eatdog test start!")
    while 1 do
        flag=flag+1
        log.info("not feed dog",flag)
        sys.wait(1000)
    end


    --feed the dog
    --log.info("WTD","eatdog test start!")
    --while 1 do
    --air153C_wtd.feed_dog(28)--28 is the watchdog control pin
    --log.info("main","feed dog")
    -- sys.wait(200000)
    -- end

    
    --Close watchdog feeding
    --log.info("WTD","close eatdog test start!")
    --air153C_wtd.close_watch_dog(28)--28 is the watchdog control pin
    -- sys.wait(1000)
    

    --Turn off Dog Feeding first, then turn on Dog Feeding
    --log.info("WTD","close eatdog and open eatdog test start!")
    --while 1 do
    --if flag==0 then
    --flag = 1
    --log.info("main","close watch dog")
    --air153C_wtd.close_watch_dog(28)--28 is the watchdog control pin
    --sys.wait(30000) --It is convenient to observe the setting time.
    --     end
    --     flag=flag+1
    --if flag == 280 then
    --log.info("main","feed dog")
    --         air153C_wtd.feed_dog(28)
    --     end
    --     sys.wait(1000)
    --log.info("Timer count(1s):", flag);
    -- end


    --Test mode reset
    --Test mode: Feeding the watchdog 2 times within 1 second will reset the Modules and restart it.
    --log.info("WTD","testmode test start!")
    --while flag<2 do
    --flag =flag+ 1
    --air153C_wtd.feed_dog(28)--28 is the watchdog control pin
    --log.info("main","feed dog")
    -- sys.wait(500)
    -- end
end)

sys.run()
