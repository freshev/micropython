
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "advdemo"
VERSION = "1.0.0"

--[[
BLE 自由广播示例

支持的模块:
1. Air601
2. ESP32系列, 包括ESP32C3/ESP32S3
3. Air101/Air103 开发板天线未引出, 天线未校准, 能用但功耗高

--Using Bluetooth applet, BeaconController, it can be searched and data changes can be seen.
]]

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

sys.taskInit(function()
    sys.wait(2000)

    --Set free broadcast data
    local data = string.fromHex("4C000215AABBCCDDAABBCCDDAABBCCDDAABBCCDD00")
    --local data = crypto.trng(25)
    --local data = string.char(0x11, 0x13, 0xA3, 0x5A, 0x11, 0x13, 0xA3, 0x5A, 0x11, 0x13, 0xA3, 0x5A, 0x11, 0x13, 0xA3, 0x5A)
    nimble.advData(data)
    --Configuration with flags
    --nimble.advData(data, 0x05)
    --Set broadcast parameters, optional
    --nimble.advParams(0x00, 0x01)
    log.info("adv_free", "data", data:toHex())
    
    --BLE mode, BEACON mode, compatible with free broadcast
    nimble.mode(nimble.MODE_BLE_BEACON)
    --Start it
    nimble.init()

    --It is also possible to change data at runtime
    while 1 do
        sys.wait(60000) --Changes every 1 minute
        --data = crypto.trng(25)
        --log.info("adv_free", "data", data:toHex())
        --nimble.advData(data)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
