
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "fotademo"
VERSION = "1.0.0"

--[[
本demo 适用于 任何支持 fota 功能的模块, 包括:
1. Cat.1模块, 如: Air700E/Air600E/Air780E/Air780EP/Air780EPV
2. wifi模块, 如: ESP32C3/ESP32S3/Air601
3. 外挂以太网的模块, 例如 Air105 + W5500
]]

--This parameter is required when using the Hezhou IoT platform
PRODUCT_KEY = "123" --Go to iot.openluat.com to create a project and get the correct project ID

sys = require "sys"
libfota2 = require "libfota2"


--Unified networking functions, which can be deleted by yourself
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

--Print the version number in a loop for easy viewing of version number changes, not necessary
sys.taskInit(function()
    while 1 do
        sys.wait(1000)
        log.info("fota", "version", VERSION)
    end
end)

--Callback function for upgrade results
local function fota_cb(ret)
    log.info("fota", ret)
    if ret == 0 then
        rtos.reboot()
    end
end

--Use the Hezhou IoT platform to upgrade, which supports custom parameters or does not need to be configured.
local ota_opts = {
    --The default upgrade URL of the Hezhou IOT platform. If not filled in, this is the default value.
    --If it is a self-built OTA server, you need to fill in the correct URL, such as http://192.168.1.5:8000/update
    --If you build your own OTA server and the url contains all parameters, there is no need to add additional parameters. Please add ### in front of the url.
    -- url="http://iot.openluat.com/api/site/firmware_upgrade",
    --Requested version number. Hezhou IOT has a version number system. If you don’t pass it on, it is Hezhou’s rule. If you build your own server, you must agree on the version number yourself.
    -- version=_G.VERSION,
    --For more parameters, please consult the documentation of libfota2 https://wiki.luatos.com/api/libs/libfota2.html
}
sys.taskInit(function()
    --This judgment is a reminder to set PRODUCT_KEY. Please delete it for actual production.
    if "123" == _G.PRODUCT_KEY and not ota_opts.url then
        while 1 do
            sys.wait(1000)
            log.info("fota", "请修改正确的PRODUCT_KEY")
        end
    end
    --Wait for the network to be ready and then start checking for upgrades
    sys.waitUntil("net_ready")
    sys.wait(500)
    libfota2.request(fota_cb, ota_opts)
    --Demo button trigger upgrade, here it is assumed that GPIO0 is used for triggering
    --sys.wait(1000)
    --gpio.debounce(0, 3000, 1)
    --gpio.setup(0, function()
    --libfota2.request(fota_cb, ota_opts)
    --end, gpio.PULLUP)
end)
--The demo is automatically updated regularly and checked every 4 hours.
sys.timerLoopStart(libfota2.request, 4*3600000, fota_cb, ota_opts)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
