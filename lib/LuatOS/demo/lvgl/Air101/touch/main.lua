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

log.info("lcd.init",
lcd.init("st7796",{port = "device",pin_dc = pin.PB01, pin_pwr = pin.PB00,pin_rst = pin.PB03,direction = 0,w = 320,h = 480,xoffset = 0,yoffset = 0},spi_lcd))


local function event_handler(obj, event)
    if(event == lvgl.EVENT_CLICKED) then
            print("Clicked")
    elseif(event == lvgl.EVENT_VALUE_CHANGED) then
            print("Toggled")
    end
end

local function demo1()
local label
local btn1 = lvgl.btn_create(lvgl.scr_act(), nil)
lvgl.obj_set_event_cb(btn1, event_handler)
lvgl.obj_align(btn1, nil, lvgl.ALIGN_CENTER, 0, -40)

label = lvgl.label_create(btn1, nil)
lvgl.label_set_text(label, "Button")

local btn2 = lvgl.btn_create(lvgl.scr_act(), nil)
lvgl.obj_set_event_cb(btn2, event_handler)
lvgl.obj_align(btn2, nil, lvgl.ALIGN_CENTER, 0, 40)
lvgl.btn_set_checkable(btn2, true)
lvgl.btn_toggle(btn2)
lvgl.btn_set_fit2(btn2, lvgl.FIT_NONE, lvgl.FIT_TIGHT)

label = lvgl.label_create(btn2, nil)
lvgl.label_set_text(label, "Toggled")
end

local function gt911CallBack(press_sta,i,x,y)
    if press_sta then
        lvgl.indev_point_emulator_update(x,y,1)
    else
        lvgl.indev_point_emulator_update(x,y,0)
    end
end

sys.subscribe("GT911",gt911CallBack)

local gt911 = require "gt911"
local i2cid = 0
local gt911_res = pin.PA07
local gt911_int = pin.PA00
local i2c_speed = i2c.FAST
sys.taskInit(function()
    i2c.setup(i2cid,i2c_speed)
    gt911.init(i2cid,gt911_res,gt911_int)

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
