
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "statemdemo"
VERSION = "1.0.0"

sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
wdt.init(9000)--Initialize watchdog set to 9s
sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds

sys.taskInit(function()
    gpio.setup(7, 0, gpio.PULLUP)
    local sm = statem.create()
                :gpio_set(7, 0) --gpio set low
                :usleep(10)     --Sleep 10us
                :gpio_set(7, 1) --gpio set to high
                :gpio_set(7, 0) --gpio set to high
                :gpio_set(7, 1) --gpio set to high
                :gpio_set(7, 0) --gpio set to high
                :gpio_set(7, 1) --gpio set to high
                :gpio_set(7, 0) --gpio set to high
                :gpio_set(7, 1) --gpio set to high
                :usleep(40)     --Sleep 40us
                :finish()
    --After executing it, background execution will be supported later.
    sm:exec()
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
