--- Module function: rtcdemo
--@Modules rtc
--@author Dozingfiretruck
--@release 2021.01.25

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "rtcdemo"
VERSION = "1.0.1"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

sys.taskInit(function()
    log.info("os.date()", os.date())
    local t = rtc.get()
    log.info("rtc", json.encode(t))
    sys.wait(2000)
    rtc.set({year=2021,mon=8,day=31,hour=17,min=8,sec=43})
    log.info("os.date()", os.date())
    --rtc.timerStart(0, {year=2021,mon=9,day=1,hour=17,min=8,sec=43})
    -- rtc.timerStop(0)
    while 1 do
        log.info("os.date()", os.date())
        local t = rtc.get()
        log.info("rtc", json.encode(t))
        sys.wait(1000)
    end
end)

--Main loop, must be added
sys.run()
