--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "gnsstest"
VERSION = "1.0.1"

--[[
Air780EG 对接 腾讯连连
注意: 室内无信号!! 无法定位!!!
]]

--sys library is standard
local sys = require("sys")
require("sysplus")

local gps_uart_id = 2
local mqttc = nil

---------------------------------------------------------------------
--[[
使用本代码需要自行调整下列配置参数:
1. qcloud_pid 项目id
2. qcloud_dev 默认是使用设备的IMEI,如果自定义了名字,改成固定的
3. qcloud_key 设备密钥,在页面新建设备后,设备详情中可以查看到
4. qcloud_host 默认值就是最常见的配置,一般不需要修改
5. qcloud_port 密钥登录模式,只需要非加密的1883端口
]]
local qcloud_pid = "JTWPP5SPLA"
local qcloud_dev = mobile.imei()
local qcloud_key = "mZE56wmoBFG3J3gG7xvTsg=="
local qcloud_host = qcloud_pid .. ".iotcloud.tencentdevices.com"
local qcloud_port = 1883
---------------------------------------------------------------------

--libgnss library initialization
libgnss.clear() --Clear data and initialize

--Initialize storage
fskv.init()

--LED and ADC initialization
LED_GNSS = 24
LED_VBAT = 26
SWITCH_1 = 1
SWITCH_2 = 22
gpio.setup(LED_GNSS, 0) --GNSS positioning success light
gpio.setup(LED_VBAT, 0) --Low voltage warning light
adc.open(adc.CH_VBAT)
-- adc.open(adc.CH_CPU)

--Determine the initialization configuration
if fskv.get("SWITCH_1") == nil then
    fskv.set("SWITCH_1", 0)
end
if fskv.get("SWITCH_2") == nil then
    fskv.set("SWITCH_2", 0)
end

gpio.setup(SWITCH_1, fskv.get("SWITCH_1"), gpio.PULLUP)
gpio.setup(SWITCH_2, fskv.get("SWITCH_2"))

--Serial port initialization
uart.setup(gps_uart_id, 115200)

function exec_agnss()
    local url = "http://download.openluat.com/9501-xingli/HXXT_GPS_BDS_AGNSS_DATA.dat"
    if http then
        --AGNSS has been tuned
        while 1 do
            local code, headers, body = http.request("GET", url).wait()
            log.info("gnss", "AGNSS", code, body and #body or 0)
            if code == 200 and body and #body > 1024 then
                for offset = 1, #body, 512 do
                    log.info("gnss", "AGNSS", "write >>>", #body:sub(offset, offset + 511))
                    uart.write(gps_uart_id, body:sub(offset, offset + 511))
                    --sys.waitUntil("UART2_SEND", 100)
                    sys.wait(100) --Waiting for 100ms will be more successful
                end
                --sys.waitUntil("UART2_SEND", 1000)
                io.writeFile("/6228.bin", body)
                break
            end
            sys.wait(60 * 1000)
        end
    end
    sys.wait(20)
    -- "$AIDTIME,year,month,day,hour,minute,second,millisecond"
    local date = os.date("!*t")
    if date.year > 2022 then
        local str = string.format("$AIDTIME,%d,%d,%d,%d,%d,%d,000", date["year"], date["month"], date["day"],
            date["hour"], date["min"], date["sec"])
        log.info("gnss", str)
        uart.write(gps_uart_id, str .. "\r\n")
        sys.wait(20)
    end
    --Read previous location information
    local gnssloc = io.readFile("/gnssloc")
    if gnssloc then
        str = "$AIDPOS," .. gnssloc
        log.info("POS", str)
        uart.write(gps_uart_id, str .. "\r\n")
        str = nil
        gnssloc = nil
    else
        --TODO initiate base station positioning
        uart.write(gps_uart_id, "$AIDPOS,3432.70,N,10885.25,E,1.0\r\n")
    end
end

function upload_stat(is_first)
    --if mqttc == nil or not mqttc:ready() then return end
    local cell = mobile.getCellInfo()
    local rmc = libgnss.getRmc(2)
    local params = {}
    if libgnss.isFix() then
        params["GPS_Info"]  = {
            longitude = rmc.lng,
            latitude = rmc.lat
        }
    end
    if is_first then
        params["switch_1"] = fskv.get("SWITCH_1")
        params["switch_2"] = fskv.get("SWITCH_2")
        params.networkType = 5
    end
    if mqttc and mqttc:ready() then
        local topic = "$thing/up/property/" .. qcloud_pid .. "/" .. qcloud_dev
        local payload = json.encode({
            method = "report",
            clientToken = "0",
            params = params
        })
        mqttc:publish(topic, payload, 1)
    end
end

function on_downlink(topic, payload) 
    local jdata = json.decode(payload)
    if not jdata then
        return
    end
    if jdata.method == "control" and jdata.params then
        local changed = false
        if jdata.params.switch_1 then
            gpio.set(SWITCH_1, jdata.params.switch_1)
            fskv.set("SWITCH_1", jdata.params.switch_1)
            changed = true
        end
        if jdata.params.switch_2 then
            gpio.set(SWITCH_2, jdata.params.switch_2)
            fskv.set("SWITCH_2", jdata.params.switch_2)
            changed = true
        end

        --Report results
        local topic = "$thing/up/property/" .. qcloud_pid .. "/" .. qcloud_dev
        local payload = json.encode({
            method = "report",
            clientToken = jdata.clientToken,
            params = jdata.params
        })
        mqttc:publish(topic, payload, 1)
    end
end

sys.taskInit(function()
    --The default baud rate of Air780EG is 115200
    log.info("GPS", "start")
    pm.power(pm.GPS, true)
    --Debug log, optional
    libgnss.debug(true)
    sys.wait(200) --It takes time for the GPNSS chip to start, about 150ms
    --Bind uart, the bottom layer automatically processes GNSS data
    --Here, data processing is delayed until the setting command is sent, and the previous data will not be uploaded.
    libgnss.bind(gps_uart_id)
    log.debug("提醒", "室内无GNSS信号,定位不会成功, 要到空旷的室外,起码要看得到天空")
    exec_agnss()
end)

--Subscribe to GNSS status encoding
sys.subscribe("GNSS_STATE", function(event, ticks)
    --The event value is
    --FIXED Positioning successful
    --LOSE positioning lost
    --Ticks is the time when the event occurs and can generally be ignored.
    local onoff = libgnss.isFix() and 1 or 0
    log.info("GNSS", "LED", onoff)
    gpio.set(LED_GNSS, onoff)
    log.info("gnss", "state", event, ticks)
    if event == "FIXED" then
        local locStr = libgnss.locStr()
        log.info("gnss", "locStr", locStr)
        if locStr then
            io.writeFile("/gnssloc", locStr)
        end
        upload_stat()
    end
end)

--mqtt upload task
sys.taskInit(function()
    sys.waitUntil("IP_READY", 15000)
    mqttc = mqtt.create(nil, qcloud_host, qcloud_port) --mqtt client creation
    sys.wait(500) --Wait for network time

    local client_id, user, passwd = iotauth.qcloud(qcloud_pid, qcloud_dev, qcloud_key)
    log.info("params", qcloud_pid, qcloud_dev, qcloud_key)
    log.info("ret", client_id, user, passwd)
    mqttc:auth(client_id, user, passwd) --mqtt triplet configuration
    log.info("mqtt", client_id, user, passwd)
    mqttc:keepalive(300) --Default value 240s
    mqttc:autoreconn(true, 3000) --Automatic reconnection mechanism

    mqttc:on(function(mqtt_client, event, data, payload) --mqtt callback registration
        --User-defined code, processed by event
        --log.info("mqtt", "event", event, mqtt_client, data, payload)
        if event == "conack" then --mqtt message after successfully completing authentication
            sys.publish("mqtt_conack") --Topics with lowercase letters are all custom topics.
            --Attribute delivery and attribute reporting response
            mqtt_client:subscribe("$thing/down/property/" .. qcloud_pid .. "/" .. qcloud_dev)
            --Incident reporting response
            mqtt_client:subscribe("$thing/event/property/" .. qcloud_pid .. "/" .. qcloud_dev)
            --App calls device behavior
            mqtt_client:subscribe("$thing/action/property/" .. qcloud_pid .. "/" .. qcloud_dev)
            --OTA downlink
            mqtt_client:subscribe("$ota/update/" .. qcloud_pid .. "/" .. qcloud_dev)
            --RRPC message downlink
            mqtt_client:subscribe("$rrpc/rxd/" .. qcloud_pid .. "/" .. qcloud_dev .. "/+")
            --Broadcast message downlink
            mqtt_client:subscribe("$broadcast/rxd/" .. qcloud_pid .. "/" .. qcloud_dev .. "/+")
            upload_stat(true)
        elseif event == "recv" then --Data sent by the server
            log.info("mqtt", "downlink", "topic", data, "payload", payload)
            on_downlink(data, payload)
        elseif event == "sent" then --Events after publish is successful
            log.info("mqtt", "sent", "pkgid", data)
        end
    end)

    --After initiating the connection, the mqtt library will automatically maintain the link. If the connection is disconnected, it will automatically reconnect by default.
    mqttc:connect()
    sys.waitUntil("mqtt_conack")
    log.info("mqtt连接成功")
    sys.timerLoopStart(upload_stat, 6 * 1000)
    while true do
        sys.wait(60*1000)
    end
    mqttc:close()
    mqttc = nil
end)

sys.taskInit(function()
    while 1 do
        sys.wait(3600 * 1000) --Check once an hour
        local fixed, time_fixed = libgnss.isFix()
        if not fixed then
            exec_agnss()
        end
    end
end)

--Low voltage alarm
sys.taskInit(function()
    while 1 do
        local vbat = adc.get(adc.CH_VBAT)
        --log.info("vbat", vbat)
        if vbat < 3400 then
            gpio.set(LED_VBAT, 1)
            sys.wait(100)
            gpio.set(LED_VBAT, 0)
            sys.wait(900)
            --TODO report low voltage event
        else
            sys.wait(1000)
        end
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
