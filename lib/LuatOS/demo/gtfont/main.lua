--- Module function: lcddemo
--@Modules lcd
--@author Dozingfiretruck
--@release 2021.01.25

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "lcddemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
wdt.init(9000)--Initialize watchdog set to 9s
sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds

local rtos_bsp = rtos.bsp()

--Return different values   according to different BSP
-- spi_id,pin_reset,pin_dc,pin_cs,bl
function lcd_pin()
    if rtos_bsp == "AIR101" then
        return 0,pin.PB03,pin.PB01,pin.PB04,pin.PB00
    elseif rtos_bsp == "AIR103" then
        return 0,pin.PB03,pin.PB01,pin.PB04,pin.PB00
    elseif rtos_bsp == "AIR105" then
        return 5,pin.PC12,pin.PE08,pin.PC14,pin.PE09
    elseif rtos_bsp == "ESP32C3" then
        return 2,10,6,7,11
    elseif rtos_bsp == "ESP32S3" then
        return 2,16,15,14,13
    elseif rtos_bsp == "EC618" then
        return 0,1,10,8,22
    elseif rtos_bsp == "EC718P" then
        return lcd.HWID_0,36,0xff,0xff,0xff --Note: EC718P has a hardware LCD driver interface, no need to use spi, of course the spi driver also supports it
    else
        log.info("main", "bsp not support")
        return
    end
end

local spi_id,pin_reset,pin_dc,pin_cs,bl = lcd_pin() 

spi_gtfont = spi.deviceSetup(1,7,0,0,8,20*1000*1000,spi.MSB,1,0) --Modify here according to your actual wiring

if spi_id ~= lcd.HWID_0 then
    spi_lcd = spi.deviceSetup(spi_id,pin_cs,0,0,8,20*1000*1000,spi.MSB,1,0)
    port = "device"
else
    port = spi_id
end


lcd.init("st7789",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd)

gtfont.init(spi_gtfont)
lcd.drawGtfontUtf8("啊啊啊",32,0,0)
lcd.drawGtfontUtf8Gray("啊啊啊",32,4,0,40)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
