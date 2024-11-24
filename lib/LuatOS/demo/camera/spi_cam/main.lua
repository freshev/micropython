PROJECT = "camerademo"
VERSION = "1.0.0"
--In actual use, just choose one
require "bf30a2"
require "gc032a"
require "gc0310"
sys = require("sys")
log.style(1)

local SCAN_MODE = 0 --Write 1 Demo Scan Code
local scan_pause = true
local getRawStart = false
local RAW_MODE = 0 --Write 1 demo to get original image
--SCAN_MODE and RAW_MODE do not write 1 to take pictures

--Return different values   according to different BSP
-- spi_id,pin_reset,pin_dc,pin_cs,bl
local function lcd_pin()
    local rtos_bsp = rtos.bsp()
    if rtos_bsp == "EC718P" then
        return lcd.HWID_0, 36, 0xff, 0xff, 0xff --Note: EC718P has a hardware LCD driver interface, no need to use spi, of course the spi driver also supports it
    else
        log.info("main", "bsp not support")
        return
    end
end
local spi_id, pin_reset, pin_dc, pin_cs, bl = lcd_pin()
if spi_id ~= lcd.HWID_0 then
    spi_lcd = spi.deviceSetup(spi_id, pin_cs, 0, 0, 8, 20 * 1000 * 1000, spi.MSB, 1, 0)
    port = "device"
else
    port = spi_id
end

--[[This is the 1.8-inch TFT LCD LCD sold by Hezhou. Resolution: 128X160. Screen ic: st7735. Purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.19. 6c2275a1Pa8F9o&id=560176729178]]
--lcd.init("st7735",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 0,w = 128,h = 160,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is a 1.54-inch TFT LCD sold by Hezhou. LCD resolution: 240X240, screen ic: st7789, purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.20. 391445d5Ql4uJl&id=659456700222]]
--lcd.init("st7789",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd,true)

--[[This is the 0.96-inch TFT LCD LCD sold by Hezhou. Resolution: 160X80. Screen ic: st7735s. Purchase address: https://item.taobao.com/item.htm?id=661054472686]]
--lcd.init("st7735v",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 1,w = 160,h = 80,xoffset = 0,yoffset = 24},spi_lcd)
--If the colors are displayed in reverse, please uncomment the following line and turn off inverting colors.
-- lcd.invoff()
--If the display is still abnormal, you can try the driver of an older version of the board.
--lcd.init("st7735s",{port = port,pin_dc = pin_dc, pin_pwr = bl, pin_rst = pin_reset,direction = 2,w = 160,h = 80,xoffset = 0,yoffset = 0},spi_lcd)
lcd.init("gc9306x", {
    port = port,
    pin_dc = pin_dc,
    pin_pwr = bl,
    pin_rst = pin_reset,
    direction = 0,
    w = 240,
    h = 320,
    xoffset = 0,
    yoffset = 0
}, spi_lcd, true)

local uartid = uart.VUART_0 --Select different uartid according to the actual device
--initialization
local result = uart.setup(uartid, --serial port id
115200, --baud rate
8, --data bits
1 --Stop bit
)

camera.on(0, "scanned", function(id, str)
    if type(str) == 'string' then
        log.info("扫码结果", str)
    elseif str == false then
        log.error("摄像头没有数据")
    else
        log.info("摄像头数据", str)
        sys.publish("capture done", true)
    end
end)

local function press_key()
    log.info("boot press")
    sys.publish("PRESS", true)
end
gpio.setup(0, press_key, gpio.PULLDOWN, gpio.RISING)
gpio.debounce(0, 100, 1)
local rawbuff, err
if RAW_MODE ~= 1 then
    rawbuff, err = zbuff.create(60 * 1024, 0, zbuff.HEAP_AUTO)
else
    rawbuff, err = zbuff.create(640 * 480 * 2, 0, zbuff.HEAP_AUTO) -- gc032a
    --local rawbuff = zbuff.create(240 * 320 * 2, zbuff.HEAP_AUTO)  --bf302a
end
if rawbuff == nil then
    log.info(err)
end

sys.taskInit(function()
    log.info("摄像头启动")
    local cspiId, i2cId = 1, 1
    local camera_id
    i2c.setup(i2cId, i2c.FAST)
    gpio.setup(5, 0) --PD pulls down
    --camera_id = bf30a2Init(cspiId,i2cId,25500000,SCAN_MODE,SCAN_MODE)
    camera_id = gc0310Init(cspiId, i2cId, 25500000, SCAN_MODE, SCAN_MODE)
    --camera_id = gc032aInit(cspiId,i2cId,24000000,SCAN_MODE,SCAN_MODE)
    camera.stop(camera_id)
    camera.preview(camera_id, true)
    log.info("按下boot开始测试")
    log.info(rtos.meminfo("sys"))
    log.info(rtos.meminfo("psram"))
    while 1 do
        result, data = sys.waitUntil("PRESS", 30000)
        if result == true and data == true then
            if SCAN_MODE == 1 then
                if scan_pause then
                    log.info("启动扫码")
                    --camera_id = gc0310Init(cspiId,i2cId,25500000,SCAN_MODE,SCAN_MODE)
                    camera.start(camera_id)
                    scan_pause = false
                    sys.wait(200)
                    log.info(rtos.meminfo("sys"))
                    log.info(rtos.meminfo("psram"))
                else
                    log.info("停止扫码")
                    --camera.close(camera_id) -- Use this to completely close the camera
                    camera.stop(camera_id)
                    scan_pause = true
                    sys.wait(200)
                    log.info(rtos.meminfo("sys"))
                    log.info(rtos.meminfo("psram"))
                end
            elseif RAW_MODE == 1 then
                if getRawStart == false then
                    getRawStart = true
                    log.debug("摄像头首次捕获原始图像")
                    camera.startRaw(camera_id, 640, 480, rawbuff) -- gc032a
                    --camera.startRaw(camera_id,240,320,rawbuff) --bf302a
                else
                    log.debug("摄像头继续捕获原始图像")
                    camera.getRaw(camera_id)
                end
                result, data = sys.waitUntil("capture done", 30000)
                log.info("摄像头捕获原始图像完成")
                log.info(rtos.meminfo("sys"))
                log.info(rtos.meminfo("psram"))
                --uart.tx(uartid, rawbuff) -- Find a serial port tool that can save data and save it as a file to view on the computer. The format is JPG.
            else
                log.debug("摄像头拍照")
                --camera_id = gc0310Init(cspiId,i2cId,25500000,SCAN_MODE,SCAN_MODE)
                camera.capture(camera_id, rawbuff, 1) --2 and 3 require a lot of psram, try not to use them
                result, data = sys.waitUntil("capture done", 30000)
                log.info(rawbuff:used())
                --camera.close(camera_id) -- Use this to completely close the camera
                camera.stop(camera_id)
                uart.tx(uartid, rawbuff) --Find a serial port tool that can save data and save it as a file to view on your computer. The format is JPG.
                rawbuff:resize(60 * 1024)
                log.info(rtos.meminfo("sys"))
                log.info(rtos.meminfo("psram"))
            end

        end
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
