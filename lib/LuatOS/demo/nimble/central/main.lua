
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "bledemo"
VERSION = "1.0.0"

--[[
BLE 中心/从机模式 --Not finished yet, still testing
状态: 
1. 扫描, 可用
2. 接收扫描结果, 可用
3. 连接到指定设备, 可用
4. 获取已连接设备的描述符
5. 订阅特征值
6. 发送数据

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

--Listen to the WRITE_CHR of the GATT server, which is the callback for collecting data
sys.subscribe("BLE_GATT_WRITE_CHR", function(info, data)
    --info is a table, but there is currently no data
    log.info("ble", "data got!!", data:toHex())
end)

--Receive scan results
sys.subscribe("BLE_SCAN_RESULT", function(addr, name, uuids, mfg_data)
    --addr Bluetooth device address, 7 bytes
    --The first byte is the address type, 0 represents a random address, 1 represents a real address
    --The last 6 bytes are the Bluetooth address
    --name device name, may not exist
    --uuids service id
    --mfg_data factory default information, mainly iBeacon or free broadcast data, added on 2023-03-19
    --log.info("ble scan", (addr:toHex()), name, json.encode(uuids), mfg_data and mfg_data:toHex() or "")
    if name == "KT6368A-BLE-1.9" then
        log.info("ble", "发现目标设备,发起连接")
        nimble.connect(addr)
    end
end)

sys.subscribe("BLE_CONN_RESULT", function(succ, ret, serv_count)
    log.info("ble", "连接结果", succ, "底层结果", ret, "服务特征数量", serv_count)
    if succ then
        log.info("ble", "设备的服务UUID列表", json.encode(nimble.listSvr()))
        nimble.discChr(string.fromHex("FF00"))
    end
end)

sys.subscribe("BLE_CHR_DISC_RESULT", function(succ, ret, chr_count)
    log.info("ble", "特征值扫描结果", succ, "底层结果", ret, "特征数量", chr_count)
    if succ then
        log.info("ble", "特征值列表", json.encode(nimble.listChr(string.fromHex("FF00"))))
        nimble.subChr(string.fromHex("FF00"), string.fromHex("FF01"))
    end
end)

sys.subscribe("BLE_GATT_READ_CHR", function(data)
    log.info("ble", "read result", data)
end)

sys.subscribe("BLE_GATT_TX_DATA", function(data)
    log.info("ble", "tx data", data)
end)

sys.taskInit(function()
    sys.wait(2000)

    nimble.config(nimble.CFG_ADDR_ORDER, 1)
    --BLE mode, the default is SERVER/Peripheral, that is, peripheral mode, devices waiting to be connected
    nimble.mode(nimble.CLIENT) --This is it by default, no need to call

    --Name can be customized
    --nimble.init("LuatOS-Wendal") -- The Bluetooth name can be modified, and there is also a default value of LOS-$mac address
    nimble.init() --The Bluetooth name can be modified, and there is also a default value LOS-$mac address

    sys.wait(500)
    --Print MAC address
    local mac = nimble.mac()
    log.info("ble", "mac", mac:toHex())
    sys.wait(1000)

    --Send data
    while 1 do 
        sys.wait(500)
        while not nimble.connok() do
            nimble.scan()
            sys.waitUntil("BLE_CONN_RESULT", 60000)
        end
        log.info("ble", "结束扫描, 进入数据传输测试")
        sys.wait(500)
        while nimble.connok() do
            --log.info("Connected", "Trying to write data")
            nimble.writeChr(string.fromHex("FF00"), string.fromHex("FF01"), "from LuatOS")
            sys.wait(1000)
        end
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
