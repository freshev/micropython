
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "httpdnsdemo"
VERSION = "1.0.0"

--[[
本demo需要http库, 大部分能联网的设备都具有这个库
http也是内置库, 无需require
]]

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the http library requires the following statements]]
_G.sysplus = require("sysplus")

httpdns = require "httpdns"


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


sys.taskInit(function()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to esptouch distribution network
        --LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        -- wlan.setMode(wlan.STATION)
        wlan.connect(ssid, password, 1)
        local result, data = sys.waitUntil("IP_READY", 30000)
        log.info("wlan", "IP_READY", result, data)
        device_id = wlan.getMac()
    elseif rtos.bsp() == "AIR105" then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
        sys.wait(1000)
        --TODO Get mac address as device_id
    elseif mobile then
        --Air780E/Air600E series
        --mobile.simid(2)
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
        --log.info("ipv6", mobile.ipv6(true))
        sys.waitUntil("IP_READY", 30000)
    end
    log.info("已联网")
    sys.publish("net_ready")
end)

sys.taskInit(function()
    sys.waitUntil("net_ready")
    while 1 do
        sys.wait(1000)
        --Get results through Alibaba DNS
        local ip = httpdns.ali("air32.cn")
        log.info("httpdns", "air32.cn", ip)


        --Get results via Tencent DNS
        local ip = httpdns.tx("openluat.com")
        log.info("httpdns", "openluat.com", ip)
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
