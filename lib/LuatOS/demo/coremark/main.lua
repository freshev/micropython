
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "coremark"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
--wdt.init(9000)--Initialize watchdog set to 9s
--sys.timerLoopStart(wdt.feed, 3000)--feed the watchdog once every 3 seconds

sys.taskInit(function()
    sys.wait(2000)
    if coremark then
        if mcu and (rtos.bsp():lower() == "air101" or rtos.bsp():lower() == "air103") then
            mcu.setClk(240)
        end
        log.info("coremark", "G0-----------------------------")
        coremark.run()
        log.info("coremark", "Done---------------------------")
    else
        log.info("coremark", "need coremark lib")
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
