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
spi_lcd = spi.deviceSetup(2, 7, 0, 0, 8, 40*1000*1000, spi.MSB, 1, 0)

--[[This is a 2.4-inch TFT LCD sold by Hezhou. Resolution: 240X320. Screen ic: GC9306. Purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.39.6c2275a1Pa8F9o&id =655959696358]]
--lcd.init("gc9a01",{port = "device",pin_dc = 6, pin_pwr = 11,pin_rst = 10,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is the 1.8-inch TFT LCD LCD sold by Hezhou. Resolution: 128X160. Screen ic: st7735. Purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.19. 6c2275a1Pa8F9o&id=560176729178]]
lcd.init("st7735",{port = "device",pin_dc = 6, pin_pwr = 11,pin_rst = 10,direction = 0,w = 128,h = 160,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is a 1.54-inch TFT LCD sold by Hezhou. LCD resolution: 240X240, screen ic: st7789, purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.20. 391445d5Ql4uJl&id=659456700222]]
--lcd.init("st7789",{port = "device",pin_dc = 6, pin_pwr = 11,pin_rst = 10,direction = 0,w = 240,h = 240,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is the 0.96-inch TFT LCD LCD sold by Hezhou. Resolution: 160X80. Screen ic: st7735s. Purchase address: https://item.taobao.com/item.htm?id=661054472686]]
--lcd.init("st7735v",{port = "device",pin_dc = 6, pin_pwr = 11,pin_rst = 10,direction = 1,w = 160,h = 80,xoffset = 0,yoffset = 24},spi_lcd)
--If the colors are displayed in reverse, please uncomment the following line and turn off inverting colors.
--lcd.invoff()
--If the display is still abnormal, you can try the driver of an older version of the board.
--lcd.init("st7735s",{port = "device",pin_dc = 6, pin_pwr = 11,pin_rst = 10,direction = 2,w = 160,h = 80,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is a 2.4-inch TFT LCD sold by Hezhou. Resolution: 240X320. Screen ic: GC9306. Purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.39.6c2275a1Pa8F9o&id =655959696358]]
--lcd.init("gc9306",{port = "device",pin_dc = 6, pin_pwr = 11,pin_rst = 10,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd)

--For those that are not built-in drivers above, see demo/lcd_custom

log.info("lvgl", lvgl.init())

local scr = lvgl.obj_create(nil, nil)
local btn = lvgl.btn_create(scr)
local btn2 = lvgl.btn_create(scr)
lvgl.obj_align(btn, lvgl.scr_act(), lvgl.ALIGN_CENTER, 0, 0)
lvgl.obj_align(btn2, lvgl.scr_act(), lvgl.ALIGN_CENTER, 0, 50)
local label = lvgl.label_create(btn)
local label2 = lvgl.label_create(btn2)
lvgl.label_set_text(label, "LuatOS!")
lvgl.label_set_text(label2, "Hi")

lvgl.scr_load(scr)



--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!


