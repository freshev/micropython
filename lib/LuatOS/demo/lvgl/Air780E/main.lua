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

--Projects with UI screens generally do not require low power consumption, so set it to the highest performance
if mcu then
    pm.request(pm.NONE)
end

--[[
--LCD connection example, taking Air780E development board as an example
LCD管脚       Air780E管脚
GND          GND
VCC          3.3V
SCL          (GPIO11)
SDA          (GPIO9)
RES          (GPIO1)
DC           (GPIO10)
CS           (GPIO8)
BL           (GPIO22)


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

--v0006 and later versions can be pinned, please upgrade to the latest firmware https://gitee.com/openLuat/LuatOS/releases
spi_lcd = spi.deviceSetup(0,8,0,0,8,20*1000*1000,spi.MSB,1,1)

--[[This is a 2.4-inch TFT LCD sold by Hezhou. Resolution: 240X320. Screen ic: GC9306. Purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.39.6c2275a1Pa8F9o&id =655959696358]]
--lcd.init("gc9a01",{port = "device",pin_dc = 10, pin_pwr = 22, pin_rst = 1,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is the 1.8-inch TFT LCD LCD sold by Hezhou. Resolution: 128X160. Screen ic: st7735. Purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.19. 6c2275a1Pa8F9o&id=560176729178]]
--lcd.init("st7735",{port = "device",pin_dc = 10, pin_pwr = 22, pin_rst = 1,direction = 0,w = 128,h = 160,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is a 1.54-inch TFT LCD sold by Hezhou. LCD resolution: 240X240, screen ic: st7789, purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.20. 391445d5Ql4uJl&id=659456700222]]
--lcd.init("st7789",{port = "device",pin_dc = 10, pin_pwr = 22, pin_rst = 1,direction = 0,w = 240,h = 240,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is the 0.96-inch TFT LCD LCD sold by Hezhou. Resolution: 160X80. Screen ic: st7735s. Purchase address: https://item.taobao.com/item.htm?id=661054472686]]
--lcd.init("st7735v",{port = "device",pin_dc = 10, pin_pwr = 22, pin_rst = 1,direction = 1,w = 160,h = 80,xoffset = 0,yoffset = 24},spi_lcd)
--If the colors are displayed in reverse, please uncomment the following line and turn off inverting colors.
--lcd.invoff()
--If the display is still abnormal, you can try the driver of an older version of the board.
--lcd.init("st7735s",{port = "device",pin_dc = 10, pin_pwr = 22, pin_rst = 1,direction = 2,w = 160,h = 80,xoffset = 0,yoffset = 0},spi_lcd)

--[[This is a 2.4-inch TFT LCD sold by Hezhou. Resolution: 240X320. Screen ic: GC9306. Purchase address: https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-24045920841.39.6c2275a1Pa8F9o&id =655959696358]]
lcd.init("gc9306",{port = "device",pin_dc = 10 , pin_pwr = 22,pin_rst = 1,direction = 0,w = 240,h = 320,xoffset = 0,yoffset = 0},spi_lcd)

--For those that are not built-in drivers above, see demo/lcd_custom

log.info("lvgl", lvgl.init())
local scr = lvgl.obj_create(nil, nil)
local btn = lvgl.btn_create(scr)

local font = lvgl.font_get("opposans_m_16") --Use whichever font is compiled according to your own firmware


lvgl.obj_align(btn, lvgl.scr_act(), lvgl.ALIGN_CENTER, 0, 0)
local label = lvgl.label_create(btn)

--Only those with Chinese fonts can display Chinese
lvgl.label_set_text(label, "LuatOS!")
--lvgl.label_set_text(label, "Hello!")
lvgl.scr_load(scr)
--The following is the method of loading fonts, choose one of the two
--Method one
--lvgl.obj_set_style_local_text_font(lvgl.scr_act(), lvgl.OBJ_PART_MAIN, lvgl.STATE_DEFAULT, font)
--Method two
--local style = lvgl.style_create()
--lvgl.style_set_text_font(style, lvgl.STATE_DEFAULT, font)
--lvgl.obj_add_style(lvgl.scr_act(),lvgl.OBJ_PART_MAIN, style)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!


