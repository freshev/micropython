
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

local button_timer_outtime = 10 --Key timer: 10ms
local button_shake_time = 1     --Button deshake time: button_shake_time*button_timer_outtime
local button_long_time = 100    --Button long press time: button_shake_time*button_timer_outtime

local button_detect = true
local button_state = false
local button_cont = 0

local BTN_PIN = 1 --Select according to actual development board

--If the firmware supports anti-shake, enable anti-shake
if gpio.debounce then
    gpio.debounce(BTN_PIN, 5)
end

button = gpio.setup(BTN_PIN, function() 
        if not button_detect then return end
        button_detect = false
        button_state = true
    end, 
    gpio.PULLUP,
    gpio.FALLING)

button_timer = sys.timerLoopStart(function()
    if button_state then
        if button() == 0 then
            button_cont = button_cont + 1
            if button_cont > button_long_time then
                print("long pass")
            end
        else 
            if button_cont < button_shake_time then
            else
                if button_cont < button_long_time then
                    print("pass")
                else
                    print("long pass")
                end
            end
            button_cont = 0
            button_state = false
            button_detect = true
        end
    end
end,button_timer_outtime) 

sys.run()
--Do not add any statements after sys.run()!!!!!
