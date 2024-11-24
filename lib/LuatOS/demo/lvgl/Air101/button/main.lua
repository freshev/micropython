--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "touch"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--v0006 and later versions can be pinned, please upgrade to the latest firmware https://gitee.com/openLuat/LuatOS/releases
spi_lcd = spi.deviceSetup(0,pin.PB04,0,0,8,20*1000*1000,spi.MSB,1,1)

--[[This is the 0.96-inch TFT LCD LCD sold by Hezhou. Resolution: 160X80. Screen ic: st7735s. Purchase address: https://item.taobao.com/item.htm?id=661054472686]]
lcd.init("st7735v",{port = "device",pin_dc = pin.PB01, pin_pwr = pin.PB00,pin_rst = pin.PB03,direction = 1,w = 160,h = 80,xoffset = 0,yoffset = 24},spi_lcd)
--If the colors are displayed in reverse, please uncomment the following line and turn off inverting colors.
--lcd.invoff()
--If the display is still abnormal, you can try the driver of an older version of the board.
--lcd.init("st7735s",{port = "device",pin_dc = pin.PB01, pin_pwr = pin.PB00,pin_rst = pin.PB03,direction = 2,w = 160,h = 80,xoffset = 1,yoffset = 26},spi_lcd)

local function event_handler(obj, event)
    if(event == lvgl.EVENT_CLICKED) then
            print("Clicked")
    elseif(event == lvgl.EVENT_VALUE_CHANGED) then
            print("Toggled")
    end
end

local function demo1()
local label

local btn2 = lvgl.btn_create(lvgl.scr_act(), nil)
lvgl.obj_set_event_cb(btn2, event_handler)
lvgl.obj_align(btn2, nil, lvgl.ALIGN_CENTER, 0, 0)
lvgl.btn_set_checkable(btn2, true)
lvgl.btn_toggle(btn2)
lvgl.btn_set_fit2(btn2, lvgl.FIT_NONE, lvgl.FIT_TIGHT)

label = lvgl.label_create(btn2, nil)
lvgl.label_set_text(label, "Toggled")
end

gpio.setup(pin.PA04,
    function(val)
        if val==0 then
            lvgl.indev_point_emulator_update(80,40,1)
        else
            lvgl.indev_point_emulator_update(80,40,0)
        end
    end, gpio.PULLUP)


sys.taskInit(function()
    log.info("lvgl", lvgl.init())
    demo1()
    lvgl.indev_drv_register("pointer", "emulator")
    while 1 do
        sys.wait(1000)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
