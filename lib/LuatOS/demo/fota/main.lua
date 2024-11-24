
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "fotademo"
VERSION = "1.0.0"

--[[
本demo 适用于 Air780E/Air780EG/Air600E
1. 需要 V1103及以上的固件
2. 需要 LuaTools 2.1.89 及以上的升级文件生成
]]

--This parameter is required when using the Hezhou IoT platform
PRODUCT_KEY = "1234" --Go to iot.openluat.com to create a project and get the correct project ID

sys = require "sys"
libfota = require "libfota"

--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--Unified networking functions
sys.taskInit(function()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking is supported by the ESP32 series. Please modify the ssid and password according to the actual situation!!
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to automatic network distribution
        wlan.init()
        wlan.setMode(wlan.STATION) --This is also the mode by default, and you can do it without calling it.
        wlan.connect(ssid, password, 1)
    elseif mobile then
        --EC618 series, such as Air780E/Air600E/Air700E
        --mobile.simid(2) -- Automatically switch SIM cards, enable on demand
        --The Modules will automatically connect to the Internet by default, no additional operations are required.
    elseif w5500 then
        --w5500 Ethernet
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
    elseif socket then
        --It’s OK if it’s adapted to the socket library. Just treat it as 1 second to connect to the Internet.
        sys.timerStart(sys.publish, 1000, "IP_READY")
    else
        --For other bsps that I don’t know, let me give you some hints in a loop.
        while 1 do
            sys.wait(1000)
            log.info("bsp", "本bsp可能未适配网络层, 请查证")
        end
    end
    --By default, it waits until the connection is successful
    sys.waitUntil("IP_READY")
    sys.publish("net_ready")
end)

sys.taskInit(function()
    while 1 do
        sys.wait(1000)
        log.info("fota", "version", VERSION)
    end
end)


function fota_cb(ret)
    log.info("fota", ret)
    if ret == 0 then
        rtos.reboot()
    end
end

--For firmware that supports full update of external flash, you can open the comment below to do full update. The external flash is mounted on SPI0, and GPIO27 is used to control power on and off.
--spi_flash = spi.deviceSetup(0,8,0,0,8,44*1000*1000,spi.MSB,1,0)
--fota.init(0xe0000000, nil, spi_flash, 27) --GPIO27 controls power on and off

--Use the Hezhou IoT platform to upgrade
sys.taskInit(function()
    sys.waitUntil("net_ready")
    libfota.request(fota_cb)
end)
sys.timerLoopStart(libfota.request, 3600000, fota_cb)

--Upgrade using a self-built server
--local ota_url = "http://192.168.1.5:8000/demo.fota"
--local ota_url = "http://192.168.1.5:8000/demo.fota"
-- sys.taskInit(function()
--     sys.waitUntil("net_ready")
--     sys.wait(3000)
--libfota.request(fota_cb, ota_url)
---- Button trigger
---- sys.wait(1000)
---- gpio.setup(0, function()
----     log.info("sayhi")
---- end, gpio.PULLUP)
-- end)
--sys.timerLoopStart(libfota.request, 3600000, fota_cb, ota_url)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
