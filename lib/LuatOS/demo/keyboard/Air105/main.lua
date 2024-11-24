--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "keyboarddemo"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end


sys.taskInit(function ()
    sys.wait(1000)
    keyboard.init(0, 0x1F, 0x14)
    sys.subscribe("KB_INC", function(port, data, state)
        --port is currently fixed at 0 and can be ignored
        --data, needs to be parsed with the map of init
        --state, 1 means pressed, 0 means released
        log.info("keyboard", port, data, state)
    end)
    while true do
        sys.wait(1000)
        log.info("gpio", "Go Go Go", "https//wiki.luatos.com", rtos.bsp())
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
