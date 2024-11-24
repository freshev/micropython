
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "gpiodemo"
VERSION = "1.0.1"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--Please change the GPIO pin numbers below according to actual needs!
--The three LEDs of the Air101 development board are PB08/PB09/PB10
--The three LEDs of the Air103 development board are PB24/PB25/PB26
--The three LEDs of the Air105 development board are PD14/PD15/PC3

--If it prompts that the pin library does not exist after downloading to the device, please upgrade the firmware to V0006 or above.


--【HaoSir2022】Added on April 21, 2022
local rtos_bsp = rtos.bsp()
function pinx() --According to different development boards, assign different gpio pin numbers to the LED
    if rtos_bsp == "AIR101" then --Air101 development board LED pin number
        return pin.PB08, pin.PB09, pin.PB10
    elseif rtos_bsp == "AIR103" then --Air103 development board LED pin number
        return pin.PB26, pin.PB25, pin.PB24
    elseif rtos_bsp == "AIR601" then --Air103 development board LED pin number
        return pin.PA7, 255, 255
    elseif rtos_bsp == "AIR105" then --Air105 development board LED pin number
        return pin.PD14, pin.PD15, pin.PC3
    elseif rtos_bsp == "ESP32C3" then --ESP32C3 development board pins
        return 12, 13, 255 --There are only 2 lights on the development board
    elseif rtos_bsp == "ESP32S3" then --ESP32C3 development board pins
        return 10, 11, 255 --There are only 2 lights on the development board
    elseif rtos_bsp == "EC618" then --Air780E development board pins
        return 27, 255, 255 --There is only one light on the AIR780E development board
    elseif rtos_bsp == "EC718P" then --Air780E development board pins
        return 27, 255, 255 --There is only one light on the AIR780EP development board
    elseif rtos_bsp == "UIS8850BM" then --Air780UM development board pins
        return 36, 255, 255 --There is only one light on the Air780UM development board
    else
        log.info("main", "define led pin in main.lua")
        return 0, 0, 0
    end
end


--LED pin determines the end of assignment

local P1,P2,P3=pinx()--Assign the LED pin number of the development board
local LEDA= gpio.setup(P1, 0, gpio.PULLUP)
local LEDB= gpio.setup(P2, 0, gpio.PULLUP)
local LEDC= gpio.setup(P3, 0, gpio.PULLUP)


sys.taskInit(function()
--Start running lights
    local count = 0
    while 1 do
    --Running water lamp program
        sys.wait(500) --Lighting time
        --Take turns lighting lamps
        LEDA(count % 3 == 0 and 1 or 0)
        if P2 and P2 ~=255 then
            LEDB(count % 3 == 1 and 1 or 0)
        end
        if P3 and P3 ~= 255 then
            LEDC(count % 3 == 2 and 1 or 0)
        end
        log.info("GPIO", "Go Go Go", count, rtos.bsp())
        log.info("LuatOS:", "https://wiki.luatos.com")
        count = count + 1
    end
end)

--API documentation https://wiki.luatos.com/api/gpio.html

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
