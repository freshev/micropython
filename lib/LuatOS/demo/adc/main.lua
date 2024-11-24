
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "adcdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000) --Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000) --Feed the watchdog once every 3 seconds
end

local testAdc = require "testAdc"
sys.taskInit(testAdc.dotest)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
