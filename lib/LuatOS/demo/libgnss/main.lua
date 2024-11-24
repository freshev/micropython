--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "gnss"
VERSION = "1.0.0"

--[[
本demo是演示定位数据处理的. script/turnkey目录有更完整的应用.
本demo需要V1103及以上的固件!!
注意: 室内无信号!! 无法定位!!! 到室外去, 起码天线要在室外.
窗口只有少许信号, 无法保证定位成功

这个demo需要的流量很多,可以注释这行
sys.publish("mqtt_pub", "/gnss/" .. mobile.imei() .. "/up/nmea", data, 1)
]]

--sys library is standard
local sys = require("sys")
require("sysplus")

local gps_uart_id = 2
local mqttc = nil

libgnss.clear() --Clear data and initialize

uart.setup(gps_uart_id, 115200)

function exec_agnss()
    if http then
        --AGNSS has been tuned
        while 1 do
            local code, headers, body = http.request("GET", "http://download.openluat.com/9501-xingli/HXXT_GPS_BDS_AGNSS_DATA.dat").wait()
            --local code, headers, body = http.request("GET", "http://nutzam.com/6228.bin").wait()
            log.info("gnss", "AGNSS", code, body and #body or 0)
            if code == 200 and body and #body > 1024 then
                --uart.write(gps_uart_id, "$reset,0,h01\r\n")
                -- sys.wait(200)
                --uart.write(gps_uart_id, body)
                for offset=1,#body,512 do
                    log.info("gnss", "AGNSS", "write >>>", #body:sub(offset, offset + 511))
                    uart.write(gps_uart_id, body:sub(offset, offset + 511))
                    --sys.waitUntil("UART2_SEND", 100)
                    sys.wait(100) --Waiting for 100ms will be more successful
                end
                --sys.waitUntil("UART2_SEND", 1000)
                io.writeFile("/6228.bin", body)
                break
            end
            sys.wait(60*1000)
        end
    end
    sys.wait(20)
    -- "$AIDTIME,year,month,day,hour,minute,second,millisecond"
    local date = os.date("!*t")
    if date.year > 2022 then
        local str = string.format("$AIDTIME,%d,%d,%d,%d,%d,%d,000", 
                         date["year"], date["month"], date["day"], date["hour"], date["min"], date["sec"])
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

function upload_stat()
    if mqttc == nil or not mqttc:ready() then return end
    local stat = {
        csq = mobile.csq(),
        rssi = mobile.rssi(),
        rsrq = mobile.rsrq(),
        rsrp = mobile.rsrp(),
        --iccid = mobile.iccid(),
        snr = mobile.snr()
    }
    sys.publish("mqtt_pub", "/gnss/" .. mobile.imei() .. "/up/stat", (json.encode(stat)), 1)
end

sys.timerLoopStart(upload_stat, 60*1000)

sys.taskInit(function()
    --The default baud rate of the GPS of the Air780EG engineering sample is 9600, and the mass production version is 115200. The following is the temporary code
    log.info("GPS", "start")
    pm.power(pm.GPS, true)
    --Bind uart, the bottom layer automatically processes GNSS data
    --The second parameter is forwarded to the virtual UART to facilitate analysis by the host computer.
    libgnss.bind(gps_uart_id, uart.VUART_0)
    libgnss.on("raw", function(data)
        --Not reported by default, open it yourself if necessary
        data = data:split("\r\n")
        if data == nil then
            return
        end
        for k, v in pairs(data) do
            if v and v:startsWith("$GNRMC") then
                sys.publish("mqtt_pub", "/gnss/" .. mobile.imei() .. "/up/nmea", v, 0)
            end
        end
    end)
    sys.wait(200) --It takes time for the GPNSS chip to start up
    --Debug log, optional
    libgnss.debug(true)
    --Show serial port configuration
    --uart.write(gps_uart_id, "$CFGPRT,1\r\n")
    -- sys.wait(20)
    --Add displayed sentences
    uart.write(gps_uart_id, "$CFGMSG,0,1,1\r\n") -- GLL
    sys.wait(20)
    uart.write(gps_uart_id, "$CFGMSG,0,5,1\r\n") -- VTG
    sys.wait(20)
    uart.write(gps_uart_id, "$CFGMSG,0,6,1\r\n") -- ZDA
    sys.wait(20)
    --After successful positioning, use GNSS time to set RTC, which is temporarily unavailable.
    -- libgnss.rtcAuto(true)
    exec_agnss()
end)

sys.taskInit(function()
    while 1 do
        sys.wait(5000)
        --6228CI, query product information, optional
        --uart.write(gps_uart_id, "$PDTINFO,*62\r\n")
        --uart.write(gps_uart_id, "$AIDINFO\r\n")
        -- sys.wait(100)
        
        --uart.write(gps_uart_id, "$CFGSYS\r\n")
        --uart.write(gps_uart_id, "$CFGMSG,6,4\r\n")
        log.info("RMC", json.encode(libgnss.getRmc(2) or {}))
        --log.info("GGA", libgnss.getGga(3))
        --log.info("GLL", json.encode(libgnss.getGll(2) or {}))
        --log.info("GSA", json.encode(libgnss.getGsa(2) or {}))
        --log.info("GSV", json.encode(libgnss.getGsv(2) or {}))
        --log.info("VTG", json.encode(libgnss.getVtg(2) or {}))
        --log.info("ZDA", json.encode(libgnss.getZda(2) or {}))
        --log.info("date", os.date())
        log.info("sys", rtos.meminfo("sys"))
        log.info("lua", rtos.meminfo("lua"))
    end
end)

--Subscribe to GNSS status encoding
sys.subscribe("GNSS_STATE", function(event, ticks)
    --The event value is
    --FIXED Positioning successful
    --LOSE positioning lost
    --Ticks is the time when the event occurs and can generally be ignored.
    log.info("gnss", "state", event, ticks)
    if event == "FIXED" then
        local locStr = libgnss.locStr()
        log.info("gnss", "locStr", locStr)
        if locStr then
            io.writeFile("/gnssloc", locStr)
        end
    end
end)

--mqtt upload task
sys.taskInit(function()
	sys.waitUntil("IP_READY", 15000)

    mqttc = mqtt.create(nil, "lbsmqtt.airm2m.com", 1886)  --mqtt client creation

    mqttc:auth(mobile.imei(), mobile.imei(), mobile.muid()) --mqtt triplet configuration
    log.info("mqtt", mobile.imei(), mobile.imei(), mobile.muid())
    mqttc:keepalive(30) --Default value 240s
    mqttc:autoreconn(true, 3000) --Automatic reconnection mechanism

    mqttc:on(function(mqtt_client, event, data, payload)  --mqtt callback registration
        --User-defined code, processed by event
        --log.info("mqtt", "event", event, mqtt_client, data, payload)
        if event == "conack" then --mqtt message after successfully completing authentication
            sys.publish("mqtt_conack") --Topics with lowercase letters are all custom topics.
            --Subscription is not required, but is usually available
            mqtt_client:subscribe("/gnss/" .. mobile.imei() .. "/down/#")
        elseif event == "recv" then --Data sent by the server
            log.info("mqtt", "downlink", "topic", data, "payload", payload)
            --Continue to add custom business processing logic here
        elseif event == "sent" then --Events after publish is successful
            log.info("mqtt", "sent", "pkgid", data)
        end
    end)

    --After initiating the connection, the mqtt library will automatically maintain the link. If the connection is disconnected, it will automatically reconnect by default.
    mqttc:connect()
	sys.waitUntil("mqtt_conack")
    log.info("mqtt连接成功")
    sys.timerStart(upload_stat, 1000) --Actively upload once after one second
    while true do
        --Business presentation. Waiting for data to be reported from other tasks
        --The mqtt_pub string here is customized and has no direct connection with the mqtt library
        --If you do not need to close the mqtt connection asynchronously, the code in while can be replaced with sys.wait(30000)
        local ret, topic, data, qos = sys.waitUntil("mqtt_pub", 30000)
        if ret then
            if topic == "close" then break end
            log.info("mqtt", "publish", "topic", topic)
            mqttc:publish(topic, data, qos)
        end
    end
    mqttc:close()
    mqttc = nil
end)

sys.taskInit(function()
    while 1 do
        sys.wait(3600*1000) --Check once an hour
        local fixed, time_fixed = libgnss.isFix()
        if not fixed then
            exec_agnss()
        end
    end
end)

sys.timerLoopStart(function()
    upload_stat()
end, 60000)

--sys.subscribe("NTP_UPDATE", function()
--if not libgnss.isFix() then
---- "$AIDTIME,year,month,day,hour,minute,second,millisecond"
--local date = os.date("!*t")
--local str = string.format("$AIDTIME,%d,%d,%d,%d,%d,%d,000",
--date["year"], date["month"], date["day"], date["hour"], date["min"], date["sec"])
--log.info("gnss", str)
--uart.write(gps_uart_id, str .. "\r\n")
--     end
-- end)

--if socket and socket.sntp then
--sys.subscribe("IP_READY", function()
--         socket.sntp()
--     end)
-- end

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
