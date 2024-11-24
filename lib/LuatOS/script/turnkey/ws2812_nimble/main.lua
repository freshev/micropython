--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "uart_irq"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
local sys = require "sys"

mcu.setClk(240)
log.info("main", "uart demo")

sys.subscribe("BLE_STATE_INC", function(state)
    log.info("ble", "ble state changed", state)
    if state == 1 then
        nimble.server_init()
    else
        nimble.server_deinit()
    end
end)

local buff = zbuff.create({8,8,24},0x000000)

--Listen to the WRITE_CHR event of the GATT server
sys.subscribe("BLE_GATT_WRITE_CHR", function(info, data)
    if data:len() == 0 then
        return
    end
    local cmd = data:split(",")
    if cmd[1]=="ws2812" then
        local rgb = tonumber(cmd[2],16)
        local grb = (rgb&0xff0000)>>8|(rgb&0xff00)<<8|(rgb&0xff)
        buff:setFrameBuffer(8,8,24,grb)
        sensor.ws2812b(pin.PB05,buff,0,300,300,300)
    end
end)

sys.taskInit(function()
    sys.wait(2000) --In order to see the log, sleep for 2 seconds
    nimble.debug(6) --Enable log
    nimble.init() --Initializing nimble will generate the event BLE_STATE_INC
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
