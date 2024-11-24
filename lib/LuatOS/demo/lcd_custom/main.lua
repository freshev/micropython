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
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--[[
    ===============================================
    ===============================================
    ===============================================
    这个demo必须修改才能运行, 把注释看完 特别主要：2023.11.29之后要自定义方向命令，即 direction0 direction90 direction180 direction270,使用哪个方向给哪个就好
    ===============================================
    ===============================================
    ===============================================
]]

local rtos_bsp = rtos.bsp()

--Return different values   according to different BSP
-- spi_id,pin_reset,pin_dc,pin_cs,bl
local function lcd_pin()
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

if spi_id ~= lcd.HWID_0 then
    spi_lcd = spi.deviceSetup(spi_id,pin_cs,0,0,8,20*1000*1000,spi.MSB,1,0)
    port = "device"
else
    port = spi_id
end

--[[ The following is an example of custom mode. You can pass in the LCD command to implement the driver. The example is shown with st7735s ]]
--[[ Pay attention to modify the gpio information corresponding to pin_xx below, the value can be the same as pin.XXX]]
--[[
    initcmd的含义, 分3种指令, 每个指令都是int32, 前2个字节是命令, 后2个字节是值
    0001 delay      延时,   例如 0x00010002 , 延时2ms
    0000/0002 cmd   发命令, 例如 0x0002004
    0003 data       发数据, 例如 0x0003004
]]

-- lcd.init("custom",{
--port = port,
--pin_dc = pin_dc,
--pin_pwr = bl,
--pin_rst = pin_reset,
--direction = 0,
--direction0 = 0x08,
--w = 128,
--h = 160,
--xoffset = 2,
--yoffset = 1,
--sleepcmd = 0x10,
--wakecmd = 0x11,
--initcmd = {
--0x00020011,0x00010078,0x00020021, -- reverse display
--         0x000200B1,0x00030002,0x00030035,
--         0x00030036,0x000200B2,0x00030002,
--         0x00030035,0x00030036,0x000200B3,
--         0x00030002,0x00030035,0x00030036,
--         0x00030002,0x00030035,0x00030036,
--         0x000200B4,0x00030007,0x000200C0,
--         0x000300A2,0x00030002,0x00030084,
--         0x000200C1,0x000300C5,0x000200C2,
--         0x0003000A,0x00030000,0x000200C3,
--         0x0003008A,0x0003002A,0x000200C4,
--         0x0003008A,0x000300EE,0x000200C5,
--         0x0003000E,0x00020036,0x000300C0,
--         0x000200E0,0x00030012,0x0003001C,
--         0x00030010,0x00030018,0x00030033,
--         0x0003002C,0x00030025,0x00030028,
--         0x00030028,0x00030027,0x0003002F,
--         0x0003003C,0x00030000,0x00030003,
--         0x00030003,0x00030010,0x000200E1,
--         0x00030012,0x0003001C,0x00030010,
--         0x00030018,0x0003002D,0x00030028,
--         0x00030023,0x00030028,0x00030028,
--         0x00030026,0x0003002F,0x0003003B,
--         0x00030000,0x00030003,0x00030003,
--         0x00030010,0x0002003A,0x00030005,
--         0x00020029,
--     },
-- },
-- spi_lcd)

--[[ The following is an example of custom mode. You can pass in the LCD command to implement the driver. The example is shown in st7789 ]]
-- lcd.init("custom",{
--port = port,
--pin_dc = pin_dc,
--pin_pwr = bl,
--pin_rst = pin_reset,
--direction = 0,
--direction0 = 0x00,
--w = 240,
--h = 320,
--xoffset = 0,
--yoffset = 0,
--sleepcmd = 0x10,
--wakecmd = 0x11,
--initcmd = {--0001 delay  0002 cmd  0003 data
--         0x02003A,0x030005,
--         0x0200B2,0x03000C,0x03000C,0x030000,0x030033,0x030033,
--         0x0200B7,0x030035,
--         0x0200BB,0x030032,
--         0x0200C2,0x030001,
--         0x0200C3,0x030015,
--         0x0200C4,0x030020,
--         0x0200C6,0x03000F,
--         0x0200D0,0x0300A4,0x0300A1,
--         0x0200E0,0x0300D0,0x030008,0x03000E,0x030009,0x030009,0x030005,0x030031,0x030033,0x030048,0x030017,0x030014,0x030015,0x030031,0x030034,
--         0x0200E1,0x0300D0,0x030008,0x03000E,0x030009,0x030009,0x030015,0x030031,0x030033,0x030048,0x030017,0x030014,0x030015,0x030031,0x030034,
--0x00020021, -- If you find that the screen is inverted, comment out this line
--     },
--     },
--     spi_lcd)

sys.taskInit(function() 
    sys.wait(500)

    log.info("lcd.drawLine", lcd.drawLine(20, 20, 150, 20, 0x001F))
    log.info("lcd.drawRectangle", lcd.drawRectangle(20, 40, 120, 70, 0xF800))
    log.info("lcd.drawCircle", lcd.drawCircle(50, 50, 20, 0x0CE0))
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!

