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

--Projects with UI screens generally do not require low power consumption. Air101/Air103 is set to the highest performance.
if mcu and (rtos.bsp() == "AIR101" or rtos.bsp() == "AIR103" or rtos.bsp() == "AIR601" ) then
    mcu.setClk(240)
end

--[[
--LCD connection example
LCD管脚       Air780E管脚    Air101/Air103管脚   Air105管脚         
GND          GND            GND                 GND                 
VCC          3.3V           3.3V                3.3V                
SCL          (GPIO11)       (PB02/SPI0_SCK)     (PC15/HSPI_SCK)     
SDA          (GPIO09)       (PB05/SPI0_MOSI)    (PC13/HSPI_MOSI)    
RES          (GPIO01)       (PB03/GPIO19)       (PC12/HSPI_MISO)    
DC           (GPIO10)       (PB01/GPIO17)       (PE08)              
CS           (GPIO08)       (PB04/GPIO20)       (PC14/HSPI_CS)      
BL(可以不接)  (GPIO22)       (PB00/GPIO16)       (PE09)              


提示:
1. 只使用SPI的时钟线(SCK)和数据输出线(MOSI), 其他均为GPIO脚
2. 数据输入(MISO)和片选(CS), 虽然是SPI, 但已复用为GPIO, 并非固定,是可以自由修改成其他脚
3. 若使用多个SPI设备, 那么RES/CS请选用非SPI功能脚
4. BL可以不接的, 若使用Air10x屏幕扩展板,对准排针插上即可
]]

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

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
        return lcd.HWID_0,36,0xff,0xff,25 --Note: EC718P has a hardware LCD driver interface, no need to use spi, of course the spi driver also supports it
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

    --[[This is the 1.8-inch TFT LCD LCD sold by Hezhou. Resolution: 128X160. Screen ic: st7735. Purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.19. 6c2275a1Pa8F9o&id=560176729178]]
    lcd.init("st7735",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 0,w = 128,h = 160,xoffset = 0,yoffset = 0},spi_lcd)
    
    --[[This is the 0.96-inch TFT LCD LCD sold by Hezhou. Resolution: 160X80. Screen ic: st7735s. Purchase address: https://item.taobao.com/item.htm?id=661054472686]]
    --lcd.init("st7735v",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 1,w = 160,h = 80,xoffset = 0,yoffset = 24},spi_lcd)
    
    --[[This is the ec718 series dedicated hardware dual data driver TFT LCD sold by Hezhou. LCD resolution: 320x480 screen ic: nv3037 purchase address: https://item.taobao.com/item.htm?id=764253232987&skuId=5258482696347&spm=a1z10 .1-c-s.w4004-24087038454.8.64961170w5EdoA]]
    --lcd.init("nv3037",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 0,w = 320,h = 480,xoffset = 0,yoffset = 0,interface_mode=lcd.DATA_2_LANE},spi_lcd)
    
    --lcd.init("st7789",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd)

    --If the colors are displayed in reverse, please uncomment the following line and turn off inverting colors.
    --lcd.invoff()
    --If the display of 0.96-inch TFT is still abnormal, you can try the driver of the old version of the board.
    --lcd.init("st7735s",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 2,w = 160,h = 80,xoffset = 0,yoffset = 0},spi_lcd)

--If there is no built-in driver, see demo/lcd_custom

sys.taskInit(function()
    --Turning on the buffer will speed up the screen refresh, but it will also consume twice the memory of the screen resolution.
    --lcd.setupBuff() -- use lua memory
    --lcd.setupBuff(nil, true) -- Use sys memory, only need to choose one
    -- lcd.autoFlush(false)

    while 1 do 
        lcd.clear()
        log.info("wiki", "https://wiki.luatos.com/api/lcd.html")
        --API documentation https://wiki.luatos.com/api/lcd.html
        if lcd.showImage then
            --Note that jpg needs to be in regular format, not progressive JPG
            --If it cannot be decoded, you can use the drawing tool to save it as, and the new file can be decoded.
            lcd.showImage(40,0,"/luadb/logo.jpg")
            sys.wait(100)
        end
        log.info("lcd.drawLine", lcd.drawLine(20,20,150,20,0x001F))
        log.info("lcd.drawRectangle", lcd.drawRectangle(20,40,120,70,0xF800))
        log.info("lcd.drawCircle", lcd.drawCircle(50,50,20,0x0CE0))
        sys.wait(1000)
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
