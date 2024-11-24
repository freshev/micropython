

PROJECT = "camerademo"
VERSION = "1.0.0"

sys = require("sys")

--[[
--LCD connection example, taking HSPI (SPI5) of Air105 development board as an example
LCD管脚       Air105管脚
GND          GND
VCC          3.3V
SCL          (PC15/HSPI_SCK)
SDA          (PC13/HSPI_MOSI)
RES          (PC12/HSPI_MISO)
DC           (PE08) --U3_RX on the development board
CS           (PC14/HSPI_CS)
BL           (PE09) --U3_TX on the development board


提示:
1. 只使用SPI的时钟线(SCK)和数据输出线(MOSI), 其他均为GPIO脚
2. 数据输入(MISO)和片选(CS), 虽然是SPI, 但已复用为GPIO, 并非固定,是可以自由修改成其他脚
]]

if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

spi_lcd = spi.deviceSetup(5,pin.PC14,0,0,8,48*1000*1000,spi.MSB,1,1)

-- log.info("lcd.init",
--lcd.init("st7735s",{port = "device",pin_dc = pin.PE08 ,pin_rst = pin.PC12,pin_pwr = pin.PE09,direction = 2,w = 160,h = 80,xoffset = 1,yoffset = 26},spi_lcd))

log.info("lcd.init",
lcd.init("st7789",{port = "device",pin_dc = pin.PE08 ,pin_rst = pin.PC12,pin_pwr = pin.PE09,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd))

-- log.info("lcd.init",
--lcd.init("st7735",{port = "device",pin_dc = pin.PE08 ,pin_rst = pin.PC12,pin_pwr = pin.PE09,direction = 0,w = 128,h = 160,xoffset = 2,yoffset = 1},spi_lcd))

-- log.info("lcd.init",
--lcd.init("gc9306x",{port = "device",pin_dc = pin.PE08 ,pin_rst = pin.PC12,pin_pwr = pin.PE09,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd))

--GC032A outputs rgb image initialization command
local GC032A_InitReg =
{
	zbar_scan = 0,--Whether it is scanning code
    draw_lcd = 1,--Whether to output to LCD
    i2c_id = 0,
	i2c_addr = 0x21,
    pwm_id = 5;
    pwm_period  = 12*1000*1000,
    pwm_pulse = 0,
	sensor_width = 640,
	sensor_height = 480,
    color_bit = 16,
	init_cmd ="/luadb/GC032A_InitReg.txt"--This method writes the initialization instructions in an external file and supports the use of # for comments.
}

--local uartid = 1 -- select a different uartid based on the actual device
----initialization
--local result = uart.setup(
--uartid,--serial port id
--115200,--Baud rate
--8,--data bits
--1--Stop bit
-- )

local camera_pwdn = gpio.setup(pin.PD06, 1, gpio.PULLUP) --PD06 camera_pwdn pin
local camera_rst = gpio.setup(pin.PD07, 1, gpio.PULLUP) --PD07 camera_rst pin

camera_rst(0)

--When taking pictures, naturally RGB output is used.
local camera_id = camera.init(GC032A_InitReg)--Screen output rgb image

log.info("摄像头启动")
camera.start(camera_id)--Start the specified camera

gpio.setup(pin.PA10, function()
    sys.publish("CAPTURE", true)
end, gpio.PULLUP,gpio.FALLING)

sys.taskInit(function()

    local spiId = 2
    local result = spi.setup(
        spiId,--serial port id
        255, --Do not use the default CS pin
        0,--CPHA
        0,--CPOL
        8,--data width
        400*1000  --Use lower frequency when initializing
    )
    local TF_CS = pin.PB3
    gpio.setup(TF_CS, 1)
    --fatfs.debug(1) -- If the mount fails, you can try to turn on debugging information to find out the reason.
    fatfs.mount(fatfs.SPI,"/sd", spiId, TF_CS, 24000000)
    local data, err = fatfs.getfree("SD")
    if data then
        log.info("fatfs", "getfree", json.encode(data))
    else
        log.info("fatfs", "err", err)
    end

    while 1 do
        result, data = sys.waitUntil("CAPTURE", 30000)
        if result==true and data==true then
            log.debug("摄像头捕获图像")
            os.remove("/sd/temp.jpg")
            camera.capture(camera_id, "/sd/temp.jpg", 1)

            --camera.capture(camera_id, "/temp.jpg", 1)
            -- sys.wait(2000)
            --local f = io.open("/temp.jpg", "r")
            --local data
            --if f then
            --data = f:read("*a")
            --log.info("fs", #data)
            --     f:close()
            -- end

            --uart.write(uartid, data) -- Find a serial port tool that can save data and save it as a file to view on the computer. The format is JPG.
        end
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
