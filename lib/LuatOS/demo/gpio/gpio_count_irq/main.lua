
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "gpio2demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--Configure gpio7 in input mode, pull down, and trigger an interrupt
--Please change the gpio number and pull-down according to actual needs
local gpio_pin = 7
gpio.setup(gpio_pin, gpio.count, gpio.PULLUP, gpio.FALLING)
pwm.open(1,2000,50) --2k 50% duty cycle as trigger source
--User code ended------------------------------------------------
--It always ends with this sentence
sys.taskInit(function()
    while true do
        sys.wait(999)
        log.info("irq cnt", gpio.count(gpio_pin))
    end
end)
sys.run()
--Do not add any statements after sys.run()!!!!!
