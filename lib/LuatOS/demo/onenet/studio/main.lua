--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "onenetdemo"
VERSION = "1.0.0"

--[[
本demo需要mqtt库, 大部分能联网的设备都具有这个库
mqtt也是内置库, 无需require

本demo演示的是 OneNet Studio, 注意区分
https://open.iot.10086.cn/studio/summary
https://open.iot.10086.cn/doc/v5/develop/detail/iot_platform
]]

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the mqtt library requires the following statements]]
_G.sysplus = require("sysplus")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--Modify the following parameters according to your own equipment
----------------------------------------------
--OneNet Studio
mqtt_host = "mqtts.heclouds.com"
mqtt_port = 1883
mqtt_isssl = false
local pid = "KzUSNA8BPh" --product id
local device = "abcdef" --Device name, set as needed. If it is Cat.1 series, mobile.imei() is usually used.
local device_secret = "21BhRAyik1mGCWoFNMFj+INgCr7QZw/pDTXnHpRaf3U=" --device key
client_id, user_name, password = iotauth.onenet(pid, device, device_secret)

--The following are commonly used topics. For the complete topic, please refer to https://open.iot.10086.cn/doc/v5/develop/detail/639
pub_topic = "$sys/" .. pid .. "/" .. device .. "/thing/property/post"
sub_topic = "$sys/" .. pid .. "/" .. device .. "/thing/property/set"
pub_custome = "$sys/" .. pid .. "/" .. device .. "/custome/up"
pub_custome_reply = "$sys/" .. pid .. "/" .. device .. "/custome/up_reply"
sub_custome = "$sys/" .. pid .. "/" .. device .. "/custome/down/+"
sub_custome_reply = "$sys/" .. pid .. "/" .. device .. "/custome/down_reply/"
------------------------------------------------

local mqttc = nil
local payloads = {}
LED = function() end

--if wdt then
--     wdt.init(9000)
--sys.timerLoopStart(wdt.feed, 3000)
-- end

function on_downlink(topic, payload)
    --Demonstrate various processing methods

    --Method one, publish the data and process it in another sys.subscribe
    sys.publish("mqtt_payload", data, payload)

    --Method 2: Add data to the queue and then notify the task to process it
    --table.insert(payloads, {topic, payload})

    --Method three, deal with it directly
    if payload.startsWith("{") and payload.endsWith("}") then
        --It may be json, try to parse it
        local jdata = json.decode(payload)
        if jdata then
            --Property settings
            if topic == sub_topic then
                local params = jdata.params
                if params then
                    local power_switch = jdata.params.power_switch
                    if power_switch then
                        if power_switch == 1 then
                            log.info("led", "on")
                            LED(1)
                        else
                            log.info("led", "off")
                            LED(1)
                        end
                    end
                    --Continue with other parameters
                end
            end
        end
    end
end

--Unified networking functions, which can be deleted by yourself
sys.taskInit(function()
    if mqtt == nil then
        while 1 do
            sys.wait(1000)
            log.info("bsp", "本固件未包含mqtt库, 请查证")
        end
    end
    local device_id = mcu.unique_id():toHex()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        mcu.setClk(240)
        --TODO Change to automatic network distribution
        --LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        wlan.setMode(wlan.STATION) --This is also the mode by default, and you can do it without calling it.
        device_id = wlan.getMac()
        wlan.connect(ssid, password, 1)
    elseif mobile then
        --Air780E/Air600E series
        --mobile.simid(2) -- Automatically switch SIM cards
        LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
    elseif w5500 then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        LED = gpio.setup(62, 0, gpio.PULLUP)
    elseif mqtt then
        --The adapted mqtt library is also OK
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
    --Waiting for Internet connection
    sys.waitUntil("net_ready")

    --Print the topic names reported (pub) and issued (sub)
    --Report: Device ---> Server
    --Delivery: device <--- server
    --You can use clients such as mqtt.x for debugging
    log.info("mqtt", "pub", pub_topic)
    log.info("mqtt", "sub", sub_topic)
    log.info("mqtt", mqtt_host, mqtt_port, client_id, user_name, password)

    -------------------------------------
    -------- MQTT demo code ---------------
    -------------------------------------

    mqttc = mqtt.create(nil, mqtt_host, mqtt_port, mqtt_isssl, ca_file)

    mqttc:auth(client_id, user_name, password) --client_id is required, the rest are optional
    --mqttc:keepalive(240) -- default value 240s
    mqttc:autoreconn(true, 3000) --Automatic reconnection mechanism

    mqttc:on(function(mqtt_client, event, data, payload)
        --User defined code
        log.info("mqtt", "event", event, mqtt_client, data, payload)
        if event == "conack" then
            --Connected
            sys.publish("mqtt_conack")
            local topics = {}
            --Topic of object model
            topics[sub_topic] = 2

            --Topics in transparent transmission mode
            --The first is that after reporting, the server will reply
            if pub_custome_reply then
                topics[pub_custome_reply] = 1
            end
            --Then there is the delivery of the server
            if sub_custome then
                topics[sub_custome] = 1
            end
            --mqtt_client:subscribe(sub_topic, 2)--Single topic subscription
            mqtt_client:subscribe(topics) --Multiple topic subscription
        elseif event == "recv" then
            --Print the received content. It is recommended to comment it out in the production environment, otherwise it will be too much.
            log.info("mqtt", "downlink", "topic", data, "payload", payload)
            on_downlink(data, payload)
        elseif event == "sent" then
            log.info("mqtt", "sent", "pkgid", data)
            --elseif event == "disconnect" then
            --When not automatically reconnecting, restart mqttc as needed
            -- mqtt_client:connect()
        end
    end)

    --mqttc automatically handles reconnection unless it closes itself
    mqttc:connect()
    sys.waitUntil("mqtt_conack")
    while true do
        --Demonstrates waiting for reporting information sent by other tasks
        local ret, topic, data, qos = sys.waitUntil("mqtt_pub", 300000)
        if ret then
            --Provides a way to close this while loop. Comment out if not needed.
            if topic == "close" then
                break
            end
            mqttc:publish(topic, data, qos)
        end
        --If there are no other tasks to report, you can write an empty wait
        -- sys.wait(60000000)
    end
    mqttc:close()
    mqttc = nil
end)

--Here is a demonstration of reporting data in another task. Data will be reported regularly. Comment out if not needed.
sys.taskInit(function()
    local qos = 1 --QOS0 does not have puback, QOS1 has puback
    while true do
        sys.wait(15000)
        log.info("准备发布数据", mqttc and mqttc:ready())
        --onenet uses OneJson, which is a standardized Json structure.
        -- https://open.iot.10086.cn/doc/v5/develop/detail/508
        if mqttc and mqttc:ready() then
            local data = {}
            data["id"] = tostring(mcu.ticks())
            data["params"] = {}
            --Business custom data
            --For example:
            --Upload a Power attribute, the type is integer and the value is 1
            data["params"]["Power"] = {
                value = 1
            }
            --Upload a WF attribute, type floating point, value 2.35
            data["params"]["Power2"] = {
                value = 2.35
            }
            --Upload a string
            data["params"]["Power3"] = {
                value = "abcdefg"
            }

            -- ==========================================================
            --Since onenet supports positioning function, the reporting of base station data is added here.
            --The base station positioning below is different from the base station positioning provided by Hezhou, please choose according to your needs.
            --The daily quota of 3 million is obtained through an additional HTTP API https://open.iot.10086.cn/doc/v5/develop/detail/722
            --Not necessary, comment out if not needed
            if mobile then
                local infos = mobile.getCellInfo()
                if infos then
                    local LBS = {}
                    for _, v in pairs(infos) do
                        local cell = {
                            cid = v.cid,
                            lac = v.lac,
                            mcc = v.mcc,
                            mnc = v.mnc,
                            networkType = 5, -- LTE,
                            ss = v.rssi,
                            ta = v.snr
                        }
                        table.insert(LBS, cell)
                    end
                    data["params"]["$OneNET_LBS"] = {
                        value = LBS
                    }
                end
            end
            --There is also wifi positioning, but it is only available for wifi networking devices
            if wlan and wlan.connect and wlan.ready() then
                local WINFO = {
                    macs = "FC:D7:33:55:92:6A,-77|B8:F8:83:E6:24:DF,-60"
                }
                data["params"]["$OneNET_LBS_WIFI"] = {
                    value = WINFO
                }
            end
            -- ==========================================================
            local updata = json.encode(data)
            log.info("mqtt", "待上报数据", updata)
            local pkgid = mqttc:publish(pub_topic, updata, qos)
        end
    end
end)


---- ======================================================================
---- -- The following is a demonstration combined with uart, a simple mqtt-uart transparent transmission implementation, comment out if not needed
--local uart_id = 1
--uart.setup(uart_id, 9600)
--uart.on(uart_id, "receive", function(id, len)
--local data = ""
--while 1 do
--local tmp = uart.read(uart_id)
--if not tmp or #tmp == 0 then
--             break
--         end
--data = data .. tmp
--     end
--log.info("uart", "uart received data length", #data)
--sys.publish("mqtt_pub", pub_custome, data)
-- end)
--sys.subscribe("mqtt_payload", function(topic, payload)
--if topic == sub_custome then
--log.info("uart", "uart sends data length", #payload)
--uart.write(1, payload)
--     end
-- end)
-- ======================================================================

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
