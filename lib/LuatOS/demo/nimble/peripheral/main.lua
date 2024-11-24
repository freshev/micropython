
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "bledemo"
VERSION = "1.0.0"

--[[
BLE peripheral的demo, 等待被连接的设备
支持的模块:
1. Air101/Air103, 开发板的BLE天线未引出, 需要靠近使用, 且功耗高
2. ESP32系列, 包括ESP32C3/ESP32S3

--Cooperate with APP - BLE debugging treasure
]]

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--Listen to the WRITE_CHR of the GATT server, which is the callback for collecting data
sys.subscribe("BLE_GATT_WRITE_CHR", function(info, data)
    --info is a table, but there is currently no data
    log.info("ble", "data got!!", data:toHex())
end)

sys.taskInit(function()
    sys.wait(2000)

    --BLE mode, the default is SERVER/Peripheral, that is, peripheral mode, devices waiting to be connected
    --nimble.mode(nimble.MODE_BLE_SERVER) -- This is it by default, no need to call it

    --Set UUID in SERVER/Peripheral mode, support setting 3
    --The address supports 2/4/16 bytes and requires binary data. For example, string.fromHex("AABB") returns 2 bytes of data, 0xAABB.
    if nimble.setUUID then --Firmware compiled after 2023-02-25 supports this API
        nimble.setUUID("srv", string.fromHex("380D"))      --Service master UUID, default value 180D
        nimble.setUUID("write", string.fromHex("FF31"))    --UUID for writing data to this device, default value FFF1
        nimble.setUUID("indicate", string.fromHex("FF32")) --UUID for subscribing to the data of this device, default value FFF2
    end

    --Name can be customized
    --nimble.init("LuatOS-Wendal") -- The Bluetooth name can be modified, and there is also a default value of LOS-$mac address
    nimble.init() --The Bluetooth name can be modified, and there is also a default value LOS-$mac address

    sys.wait(500)
    --Print MAC address
    local mac = nimble.mac()
    log.info("ble", "mac", mac and mac:toHex() or "Unknwn")

    --Send data
    while 1 do
        sys.wait(3000)
        nimble.send_msg(1, 0, string.char(0x5A, 0xA5, 0x12, 0x34, 0x56))
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
