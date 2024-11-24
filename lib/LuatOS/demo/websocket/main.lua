
PROJECT = "airtun"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")
--_G.sysplus = require("sysplus")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

----------------------------------------
--Error information is automatically reported to the platform, the default is iot.openluat.com
--Supports customization, please refer to the API manual for detailed configuration
--After being turned on, the boot reason will be reported. This requires data consumption, please pay attention.
if errDump then
    errDump.config(true, 600)
end
----------------------------------------

local wsc = nil

sys.taskInit(function()
    if wlan and wlan.connect then
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to esptouch distribution network
        --LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        -- wlan.setMode(wlan.STATION)
        wlan.connect(ssid, password, 1)
        local result, data = sys.waitUntil("IP_READY", 30000)
        socket.sntp()
        log.info("wlan", "IP_READY", result, data)
        device_id = wlan.getMac()
    elseif rtos.bsp() == "AIR105" then
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
        sys.wait(1000)
        --TODO Get mac address as device_id
    elseif mobile then
        --mobile.simid(2)
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
        sys.waitUntil("IP_READY", 30000)
    end

    --This is a test service. When json is sent and action=echo, the content sent will be echoed.
    wsc = websocket.create(nil, "ws://echo.airtun.air32.cn/ws/echo")
    --This is another test service that can respond to websocket binary frames
    --wsc = websocket.create(nil, "ws://echo.airtun.air32.cn/ws/echo2")
    --The above two test services are written in Java, and the source code is at https://gitee.com/openLuat/luatos-airtun/tree/master/server/src/main/java/com/luatos/airtun/ws

    if wsc.headers then
        wsc:headers({Auth="Basic ABCDEGG"})
    end
    wsc:autoreconn(true, 3000) --Automatic reconnection mechanism
    wsc:on(function(wsc, event, data, fin, optcode)
        --event event, currently there are conack and recv
        --data When the event is recv, there is received data
        --fin is the last data packet, 0 means there is still data, 1 means it is the last data packet
        --optcode, 0 - intermediate packet, 1 - text packet, 2 - binary packet
        --Because Lua does not distinguish between text and binary data, optcode can usually be ignored
        --If there is not much data, less than 1400 bytes, then fid is usually 1, which can also be ignored.
        log.info("wsc", event, data, fin, optcode)
        --Display binary data
        --log.info("wsc", event, data and data:toHex() or "", fin, optcode)
        if event == "conack" then --After connecting to the websocket service, there will be this event
            wsc:send((json.encode({action="echo", device_id=device_id})))
            sys.publish("wsc_conack")
        end
    end)
    wsc:connect()
    --Waiting for conack is optional
    --sys.waitUntil("wsc_conack", 15000)
    --Sending business pings regularly is also optional, but in order to save the connection and continue to hold the wsc object, data is sent periodically here.
    while true do
        sys.wait(15000)
        --Send text frame
        -- wsc:send("{\"room\":\"topic:okfd7qcob2iujp1br83nn7lcg5\",\"action\":\"join\"}")
        wsc:send((json.encode({action="echo", msg=os.date()})))
        --Sending binary frames, supported by firmware compiled after 2023.06.21
        --wsc:send(string.char(0xA5, 0x5A, 0xAA, 0xF2), 1, 1)
    end
    wsc:close()
    wsc = nil
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
