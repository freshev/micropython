
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "wdtdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")


sys.taskInit(function()
    --This demo requires the wdt library
    --The use of wdt library, basically there is a demonstration in the head of each demo
    --The internal hard watchdog of the Modules/chip can solve the crash problem in most cases
    --But if there are very demanding scenarios, it is still recommended to plug in the hardware and then feed the watchdog regularly through gpio/i2c
    if wdt == nil then
        while 1 do
            sys.wait(1000)
            log.info("wdt", "this demo need wdt lib")
        end
    end
    --Note that most chips/Moduless will restart after 2 times the timeout period
    --The following is the general configuration, 9 seconds timeout, feed the watchdog once every 3 seconds
    --If the software crashes, loops endlessly, or the hardware freezes, it will automatically reset after up to 18 seconds.
    --Note: Software bugs causing business failure cannot be solved through wdt
    wdt.init(9000)
    sys.timerLoopStart(wdt.feed, 3000)
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
