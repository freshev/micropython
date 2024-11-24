
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "udpsrvdemo"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

_G.udpsrv = require "udpsrv"

--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--Unified networking functions
sys.taskInit(function()
    local device_id = mcu.unique_id():toHex()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to automatic network distribution
        --LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        --wlan.setMac(0, string.fromHex("6055F9779010"))
        wlan.setMode(wlan.STATION) --This is also the mode by default, and you can do it without calling it.
        device_id = wlan.getMac()
        wlan.connect(ssid, password, 1)
    elseif mobile then
        --Air780E/Air600E series
        --mobile.simid(2) -- Automatically switch SIM cards
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
    elseif w5500 then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
    elseif socket then
        --The adapted socket library is also OK
        --No other operations, just give a comment
    else
        --For other bsps that I don’t know, let me give you some hints in a loop.
        while 1 do
            sys.wait(1000)
            log.info("bsp", "本bsp可能未适配网络层, 请查证")
        end
    end
    --By default, it waits until the connection is successful
    sys.waitUntil("IP_READY")
    sys.publish("net_ready", device_id)
end)

sys.taskInit(function()
    sys.waitUntil("net_ready")
    local mytopic = "my_udpsrv"
    --Note that udpsrv.create has 3 parameters, the last parameter is the network adapter number
    local srv = udpsrv.create(12345, mytopic)
    --In the wifi Modules, there are usually two adapters, STA and AP. If you need to monitor at the AP, you need to specify the number.
    --local srv = udpsrv.create(12345, mytopic, socket.LWIP_AP)
    if srv then
        --Unicast
        srv:send("I am UP", "192.168.1.5", 777)
        --broadcast
        srv:send("I am UP", "255.255.255.255", 777)
        while 1 do
            local ret, data, remote_ip, remote_port = sys.waitUntil(mytopic, 15000)
            if ret then
                --remote_ip, remote_port are new return values   in 2023.10.12
                log.info("udpsrv", "收到数据", data:toHex(), remote_ip, remote_port)
                --Process data received by business
            else
                log.info("udpsrv", "没数据,那广播一条")
                srv:send("I am UP", "255.255.255.255", 777)
            end
        end
    else
        log.info("udpsrv", "启动失败")
    end
    --If closed, call
    -- srv:close()
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
