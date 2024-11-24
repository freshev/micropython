
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "pwmdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

local PWM_ID = 0
if rtos.bsp() == "EC618" then
    PWM_ID = 4 --GPIO 27, NetLed
elseif rtos.bsp() == "EC718P" then
    PWM_ID = 2 --GPIO 25
elseif rtos.bsp() == "AIR101" or rtos.bsp() == "AIR103" or rtos.bsp() == "AIR601"  then
    PWM_ID = 4 --GPIO 4
elseif rtos.bsp():startsWith("ESP32") then
    --Note that the PWM and PWM channels of the ESP32 series are the same as the GPIO numbers.
    --For example, if you need to use GPIO1 to output PWM, the corresponding PWM channel is 1
    --GPIO16 needs to be used to output PWM, and the corresponding PWM channel is 16
    if rtos.bsp() == "ESP32C3" then
        PWM_ID = 12 --GPIO 12
    elseif rtos.bsp() == "ESP32S3" then
        PWM_ID = 11 --GPIO 11
    end
elseif rtos.bsp() == "AIR105" then
    PWM_ID = 1 --GPIO 17
end

sys.taskInit(function()
    log.info("pwm", "ch", PWM_ID)
    while 1 do
        --Imitation breathing light effect
        log.info("pwm", ">>>>>")
        for i = 10,1,-1 do 
            pwm.open(PWM_ID, 1000, i*9) --Frequency 1000hz, duty cycle 0-100
            sys.wait(100 + i*10)
        end
        for i = 10,1,-1 do 
            pwm.open(PWM_ID, 1000, 100 - i*9)
            sys.wait(100 + i*10)
        end
        sys.wait(2000)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
