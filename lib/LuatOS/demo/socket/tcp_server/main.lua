
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "tcpserver"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")


srv = require("tcpsrv")


--Because this is a general demo, air101/air103 runs at full speed so that it is not too slow -_-
if mcu then
    mcu.setClk(240)
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
        --wlan.setMode(wlan.STATION) -- This is also the default mode, you can do it without calling it
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
    sys.waitUntil("net_ready")
    log.info("联网完成", "准备启动tcp server")
    sys.wait(1000)
    SerDemo(80)
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
