
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "bledemo"
VERSION = "1.0.0"

--[[
BLE 扫描
状态: 
1. 扫描, 可用
2. 接收扫描结果, 可用

支持的模块:
1. Air601
2. ESP32系列, 包括ESP32C3/ESP32S3
3. Air101/Air103 开发板天线未引出, 天线未校准, 能用但功耗高
]]

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--Receive scan results
sys.subscribe("BLE_SCAN_RESULT", function(addr, name, uuids, mfg_data)
    --addr Bluetooth device address, 7 bytes
    --The first byte is the address type, 0 represents a random address, 1 represents a real address
    --The last 6 bytes are the Bluetooth address
    --name device name, may not exist
    --uuids service id
    --mfg_data factory default information, mainly iBeacon or free broadcast data, added on 2023-03-19
    log.info("blescan", (addr:toHex()), name, json.encode(uuids), mfg_data and mfg_data:toHex() or "")
end)

sys.taskInit(function()
    sys.wait(2000)

    --BLE mode, the default is SERVER/Peripheral, that is, peripheral mode, devices waiting to be connected
    nimble.mode(nimble.CLIENT) --This is it by default, no need to call

    --Name can be customized
    --nimble.init("LuatOS-Wendal") -- The Bluetooth name can be modified, and there is also a default value of LOS-$mac address
    nimble.init() --The Bluetooth name can be modified, and there is also a default value LOS-$mac address

    sys.wait(500)
    --Print MAC address
    local mac = nimble.mac()
    log.info("ble", "mac", mac and mac:toHex() or "Unknwn")
    sys.wait(1000)

    --Send data
    while 1 do
        log.info("ble", "start SCAN ...")
        nimble.scan()
        --TODO After scanning the specified device, the loop should be jumped out
        sys.wait(30000)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
