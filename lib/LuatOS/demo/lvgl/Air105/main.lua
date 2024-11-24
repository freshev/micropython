--- Module function: lvgldemo
--@Modules lvgl
--@author Dozingfiretruck
--@release 2021.01.25

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "lvgldemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--[[
--LCD connection example, taking the HSPI of Air105 development board as an example
LCD管脚       Air105管脚
GND          GND
VCC          3.3V
SCL          (PC15/SPI0_SCK)
SDA          (PC13/SPI0_MOSI)
RES          (PC12)
DC           (PE8)
CS           (PC14)
BL           (PE9)


提示:
1. 只使用SPI的时钟线(SCK)和数据输出线(MOSI), 其他均为GPIO脚
2. 数据输入(MISO)和片选(CS), 虽然是SPI, 但已复用为GPIO, 并非固定,是可以自由修改成其他脚
]]

--Add a hard watchdog to prevent the program from freezing
wdt.init(9000)--Initialize watchdog set to 9s
sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds

log.info("hello luatos")

spi_lcd = spi.deviceSetup(5,pin.PC14,0,0,8,96*1000*1000,spi.MSB,1,1)

-- log.info("lcd.init",
--lcd.init("gc9a01",{port = "device",pin_dc = pin.PE8,pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd))
-- log.info("lcd.init",
--lcd.init("st7789",{port = "device",pin_dc = pin.PE8, pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 0,w = 240,h = 240,xoffset = 0,yoffset = 0},spi_lcd))
-- log.info("lcd.init",
--lcd.init("st7789",{port = "device",pin_dc = pin.PE8, pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 3,w = 240,h = 240,xoffset = 80,yoffset = 0},spi_lcd))
-- log.info("lcd.init",
--lcd.init("st7789",{port = "device",pin_dc = pin.PE8, pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 3,w = 320,h = 240,xoffset = 0,yoffset = 0},spi_lcd))
-- log.info("lcd.init",
--lcd.init("st7789",{port = "device",pin_dc = pin.PE8, pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd))
-- log.info("lcd.init",
--lcd.init("st7735",{port = "device",pin_dc = pin.PE8, pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 0,w = 128,h = 160,xoffset = 2,yoffset = 1},spi_lcd))

--[[This is the 0.96-inch TFT LCD LCD sold by Hezhou. Resolution: 160X80. Screen ic: st7735s. Purchase address: https://item.taobao.com/item.htm?id=661054472686]]
--lcd.init("st7735v",{port = "device",pin_dc = pin.PE08, pin_pwr = pin.PE09, pin_rst = pin.PC12,direction = 1,w = 160,h = 80,xoffset = 0,yoffset = 24},spi_lcd)
--If the colors are displayed in reverse, please uncomment the following line and turn off inverting colors.
--lcd.invoff()
--If the display is still abnormal, you can try the driver of an older version of the board.
--lcd.init("st7735s",{port = "device",pin_dc = pin.PE08, pin_pwr = pin.PE09, pin_rst = pin.PC12,direction = 2,w = 160,h = 80,xoffset = 0,yoffset = 0},spi_lcd)

-- log.info("lcd.init",
--lcd.init("st7735s",{port = "device",pin_dc = pin.PE8,pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 2,w = 160,h = 80,xoffset = 1,yoffset = 26},spi_lcd))
log.info("lcd.init",
lcd.init("gc9306x",{port = "device",pin_dc = pin.PE8,pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd))
-- log.info("lcd.init",
--lcd.init("ili9341",{port = "device",pin_dc = pin.PE8, pin_rst = pin.PC12,pin_pwr = pin.PE9,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd))


log.info("lvgl", lvgl.init())
local scr = lvgl.obj_create(nil, nil)
local btn = lvgl.btn_create(scr)

local font = lvgl.font_get("opposans_m_16") --Use whichever font is compiled according to your own firmware


lvgl.obj_align(btn, lvgl.scr_act(), lvgl.ALIGN_CENTER, 0, 0)
local label = lvgl.label_create(btn)

--Only those with Chinese fonts can display Chinese
--lvgl.label_set_text(label, "LuatOS!")
lvgl.label_set_text(label, "你好!")
lvgl.scr_load(scr)
--The following is the method of loading fonts, choose one of the two
--Method one
lvgl.obj_set_style_local_text_font(lvgl.scr_act(), lvgl.OBJ_PART_MAIN, lvgl.STATE_DEFAULT, font)
--Method two
--local style = lvgl.style_create()
--lvgl.style_set_text_font(style, lvgl.STATE_DEFAULT, font)
--lvgl.obj_add_style(lvgl.scr_act(),lvgl.OBJ_PART_MAIN, style)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!


