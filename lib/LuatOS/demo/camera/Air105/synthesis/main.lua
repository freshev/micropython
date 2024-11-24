

PROJECT = "camerademo"
VERSION = "1.0.0"

sys = require("sys")

--[[
--LCD connection example, taking the HSPI of Air105 development board as an example
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

--GC032A output grayscale image initialization command
local GC032A_InitReg_Gray =
{
	zbar_scan = 1,--Whether it is scanning code
    draw_lcd = 1,--Whether to output to LCD
    i2c_id = 0,
	i2c_addr = 0x21,
    pwm_id = 5;
    pwm_period  = 24*1000*1000,
    pwm_pulse = 0,
	sensor_width = 640,
	sensor_height = 480,
    color_bit = 16,
	init_cmd ="/luadb/GC032A_InitReg_Gray.txt"--This method writes the initialization instructions in an external file and supports the use of # for comments.
}



--Register camera event callback
camera.on(0, "scanned", function(id, str)
    if type(str) == 'string' then
        log.info("扫码结果", str)
    else
        log.info("拍照结果", str)

    end
end)

local camera_pwdn = gpio.setup(pin.PD06, 1, gpio.PULLUP) --PD06 camera_pwdn pin
local camera_rst = gpio.setup(pin.PD07, 1, gpio.PULLUP) --PD07 camera_rst pin

sys.taskInit(function()
    camera_rst(0)

    --Only one of the following two lines is opened! One is the screen output rgb image, the other is the screen output grayscale image and scan the code
    --local camera_id = camera.init(GC032A_InitReg)--screen output rgb image
    local camera_id = camera.init(GC032A_InitReg_Gray)--The screen outputs a grayscale image and scans the code


    log.info("摄像头启动")
    camera.start(camera_id)--Start the specified camera
    sys.wait(2000)
    --log.info("Camera stopped")
    --camera.stop(camera_id)--Stop the specified camera
    -- sys.wait(2000)
    --log.info("Camera started")
    --camera.start(camera_id)--Start the specified camera
    -- sys.wait(2000)
    --log.info("Camera off")
    -- camera_rst(1)
    -- camera.close(camera_id)
    --The following functions must have firmware that supports camera.close to be opened.
    --log.info("Camera reinitialization")
    -- camera_rst(0)
    --camera_id = camera.init(GC032A_InitReg_Gray)--The screen outputs a grayscale image and scans the code
    --log.info("Camera started")
    --camera.start(camera_id)--Start the specified camera
    --The following functions must have firmware that supports taking pictures to enable them.
    -- camera_rst(0)
    --camera_id = camera.init(GC032A_InitReg)--output grayscale image on the screen and scan the code
    --log.info("Camera started")
    --camera.start(camera_id)--Start the specified camera
    -- sys.wait(2000)
    --log.debug("Camera capture image")
    --camera.capture(camera_id, "/temp.jpg", 1)
    -- sys.wait(2000)
    --local f = io.open("/temp.jpg", "r")
    --local data
    --if f then
    --data = f:read("*a")
    --log.info("fs", #data)
    --     f:close()
    -- end
    --local uartid = 2 -- select a different uartid based on the actual device

    ----initialization
    --local result = uart.setup(
    --uartid,--serial port id
    --115200,--Baud rate
    --8,--data bits
    --1--Stop bit
    -- )
    --uart.write(uartid, data) -- Find a serial port tool that can save data and save it as a file to view on the computer.
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
