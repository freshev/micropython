
--LuaTools requires two pieces of information, PROJECT and VERSION
--This demo is suitable for Air101/Air103
PROJECT = "nimbledemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--[[
支持的模块:
1. Air601
2. ESP32系列, 包括ESP32C3/ESP32S3
3. Air101/Air103 开发板天线未引出, 天线未校准, 能用但功耗高
]]

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
else
    log.warn("wdt", "not wdt found!!!")
end


leds = {}
if rtos.bsp():lower() == "air101" then --Equivalent to w800/805
    leds["a"] = gpio.setup(pin.PB08, 0, gpio.PULLUP) --PB_08, output mode
    leds["b"] = gpio.setup(pin.PB09, 0, gpio.PULLUP) --PB_09, output mode
    leds["c"] = gpio.setup(pin.PB10, 0, gpio.PULLUP) --PB_10, output mode
elseif rtos.bsp():lower() == "air103" then --Equivalent to w806
    leds["a"] = gpio.setup(pin.PB24, 0, gpio.PULLUP) --PB_24, output mode
    leds["b"] = gpio.setup(pin.PB25, 0, gpio.PULLUP) --PB_25, output mode
    leds["c"] = gpio.setup(pin.PB26, 0, gpio.PULLUP) --PB_26, output mode
else
    log.info("gpio", "pls add gpio.setup for you board")
end

local count = true
local function led_bulingbuling()
    leds["a"](count == true and 1 or 0)
    leds["b"](count == true and 1 or 0)
    leds["c"](count == true and 1 or 0)
    count = not count
    nimble.send_msg(1, 0, "led_bulingbuling")
end
local bulingbuling = sys.timerLoopStart(led_bulingbuling, 1000)

if lcd then
    spi_lcd = spi.deviceSetup(0,20,0,0,8,20*1000*1000,spi.MSB,1,1)
    lcd.setColor(0x0000,0xFFFF)
    log.info("lcd.init",
    lcd.init("st7735s",{port = "device",pin_dc = 17, pin_pwr = 7,pin_rst = 19,direction = 2,w = 160,h = 80,xoffset = 1,yoffset = 26},spi_lcd))
    lcd.clear()
    lcd.setFont(lcd.font_opposansm12_chinese)
    lcd.drawStr(30,15,"nimbledemo",0X07FF)
    lcd.drawStr(50,35,"监听中",0x001F)
else
    log.info("lcd", "lcd not found, display is off")
end

gpio.setup(pin.PA00, function(val) print("PA0 R",val) if lcd then lcd.fill(0,40,160,80) if val == 0 then lcd.drawStr(50,60,"R down",0x07E0) end end end, gpio.PULLUP)
gpio.setup(pin.PA07, function(val) print("PA7 U",val) if lcd then lcd.fill(0,40,160,80) if  val == 0 then lcd.drawStr(50,60,"U down",0x07E0) end end end, gpio.PULLUP)
gpio.setup(pin.PA04, function(val) print("PA4 C",val) if lcd then lcd.fill(0,40,160,80) if  val == 0 then lcd.drawStr(50,60,"C down",0x07E0) end end end, gpio.PULLUP)
gpio.setup(pin.PA01, function(val) print("PA1 L",val) if lcd then lcd.fill(0,40,160,80) if  val == 0 then lcd.drawStr(50,60,"L down",0x07E0) end end end, gpio.PULLUP)
gpio.setup(pin.PB11, function(val) print("PB11 D",val) if lcd then lcd.fill(0,40,160,80) if  val == 0 then lcd.drawStr(50,60,"D down",0x07E0) end end end, gpio.PULLUP)


function decodeURI(s)
    s = string.gsub(s, '%%(%x%x)', function(h) return string.char(tonumber(h, 16)) end)
    return s
end

--Register a command list
cmds = {
    --Commands to control LEDs
    led = function(id, val)
        local led = leds[id]
        if led then
            led(val == "on" and 1 or 0)
        end
    end,
    --Command to restart the board
    reboot = function()
        sys.taskInit(function()
            log.info("ble", "cmd reboot, after 5s")
            sys.wait(5000)
            rtos.reboot()
        end)
    end,
    --Commands for display output content
    display = function(text)
        lcd.fill(0, 20, 160, 36)
        lcd.drawStr(50 , 35, decodeURI(text) ,0x001F)
    end,
}

--Monitoring the status changes of the BLE master adapter requires the nimble library
if nimble then
    --This event will be generated if BLE initialization is successful or failed.
    sys.subscribe("BLE_STATE_INC", function(state)
        log.info("ble", "ble state changed", state)
        if state == 1 then
            nimble.server_init()
        else
            nimble.server_deinit()
        end
    end)

    --Listen to the WRITE_CHR event of the GATT server
    sys.subscribe("BLE_GATT_WRITE_CHR", function(info, data)
        sys.timerStop(bulingbuling)
        --info is a table, but there is currently no data
        log.info("ble", "data got!!", data:toHex())
        if data:len() == 0 then
            return
        end
        --led,a,on corresponds to hex value 6c65642c612c6f6e
        --led,b,on corresponds to hex value 6c65642c622c6f6e
        --led,c,on corresponds to hex value 6c65642c632c6f6e
        --led,a,off corresponds to 6c65642c612c6f6666
        --led,b,off corresponds to 6c65642c622c6f6666
        --led,c,off corresponds to 6c65642c632c6f6666
        --display,xxx corresponds to 646973706C6179xxx, supports Chinese

        local cmd = data:split(",")
        if cmd[1] and cmds[cmd[1]] then
            cmds[cmd[1]](table.unpack(cmd, 2))
        else
            log.info("ble", "unkown cmd", json.encode(cmd))
        end
    end)


    --TODO supports transmitting data (read) and pushing data (notify)

--Cooperate with APP - BLE debugging treasure

    sys.taskInit(function()
        sys.wait(2000) --In order to see the log, sleep for 2 seconds
        nimble.debug(6) --Enable log
        nimble.init() --Initializing nimble will generate the event BLE_STATE_INC
    end)
else
    --If there is no nimble, then just flash the light.
    log.info("gpio", "no nimble, just leds")
end




--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
