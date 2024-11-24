--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "onenetdemo"
VERSION = "1.0.0"

--[[
本demo演示的是 OneNet Studio, 注意区分
https://open.iot.10086.cn/studio/summary
https://open.iot.10086.cn/doc/v5/develop/detail/iot_platform

本demo演示的是coap方式
]]

--sys library is standard
_G.sys = require("sys")

local onenetcoap = require("onenetcoap")

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
        wlan.setMode(wlan.STATION)
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
        log.info("ipv6", mobile.ipv6(true))
        sys.waitUntil("IP_READY", 30000)
    elseif http then
        sys.waitUntil("IP_READY")
    else
        while 1 do
            sys.wait(1000)
            log.info("http", "当前固件未包含http库")
        end
    end
    log.info("已联网")
    sys.publish("net_ready")
end)



sys.taskInit(function()
    sys.waitUntil("net_ready")
    socket.sntp()
    sys.waitUntil("NTP_UPDATE", 1000)
    --Equipment information must be filled in according to actual conditions
    local dev = {
        product_id = "SJaLt5cVL2",
        device_name = "luatospc",
        device_key = "dUZVVWRIcjVsV2pSbTJsckd0TmgyRXNnMTJWMXhIMkk=",
        debug = false
    }
    if onenetcoap.setup(dev) then
        onenetcoap.start()
    else
        log.error("配置失败")
    end
end)

sys.taskInit(function()
    sys.waitUntil("net_ready")
    while 1 do
        --Simulate scheduled uplink data
        sys.wait(5000)
        local post = {
            id = "123",
            params = {
                WaterMeterState = {
                    value = 0
                }
            }
        }
        --What we use here is the object model
        onenetcoap.uplink("thing/property/post", post)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
