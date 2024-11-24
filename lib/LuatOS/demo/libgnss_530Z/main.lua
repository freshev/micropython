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
local lbsLoc = require("lbsLoc")
local sys = require("sys")
require("sysplus")

local gps_uart_id = 2
local mqttc = nil

local lla = {
    lat,
    lng
}

libgnss.clear() --Clear data and initialize

uart.setup(gps_uart_id, 9600)

function exec_agnss()
    if http then
        --AGNSS has been tuned
        while 1 do
            local code, headers, body = http.request("GET", "http://download.openluat.com/9501-xingli/CASIC_data.dat").wait()
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
    --Read previous location information
    local str = io.readFile("/gnssrmc")
    if str then
        --The first is the time information, note that it is UTC time
        --There are many time sources. It is generally recommended to use socket.sntp() to synchronize the system time.
        local dt = os.date("!*t")
        lla = json.decode(str)
        --Then there are auxiliary positioning coordinates
        --Sources come in many ways:
        --1. Obtained from historical positioning data, for example, the previous positioning was saved to the local file system after successful positioning.
        --2. Obtained through base station positioning or wifi positioning
        --3. Obtain approximate coordinates through IP positioning
        --The coordinate system is WGS84, but since it is auxiliary positioning, accuracy is not a key factor.
        local aid = libgnss.casic_aid(dt, lla)
        uart.write(gps_uart_id, aid.."\r\n")
        str = nil
    else
        --TODO initiate base station positioning
        mobile.reqCellInfo(15)
        sys.waitUntil("CELL_INFO_UPDATE", 3000)
        lbsLoc.request(getLocCb)
    end
end

--Function: Callback function after obtaining the latitude and longitude corresponding to the base station
--Parameters: -- result: number type, 0 means success, 1 means the network environment is not ready yet, 2 means failure to connect to the server, 3 means failure to send data, 4 means the receiving server response timed out, 5 means the server failed to return the query; when it is 0, Only the next 5 parameters are meaningful
        --lat: string type, latitude, 3 digits for the integer part and 7 digits for the decimal part, for example, 031.2425864
        --lng: string type, longitude, 3 digits for the integer part and 7 digits for the decimal part, for example, 121.4736522
        --addr: currently meaningless
        --time: string type or nil, the time returned by the server, 6 bytes, year, month, day, hour, minute and second, need to be converted to hexadecimal for reading
            --The first byte: year minus 2000, for example, 2017, it is 0x11
            --The second byte: month, for example, July is 0x07, December is 0x0C
            --The third byte: day, for example, the 11th is 0x0B
            --The fourth byte: hour, for example, when 18, it is 0x12
            --The fifth byte: points, for example, 59 points is 0x3B
            --The sixth byte: seconds, for example, 48 seconds is 0x30
        --locType: numble type or nil, positioning type, 0 indicates successful base station positioning, 255 indicates successful WIFI positioning
function getLocCb(result, lat, lng, addr, time, locType)
    local dt = os.date("!*t")
    local locStr = { lat ,lng }
    locStr.lat = lat
    locStr.lng = lng
    log.info("testLbsLoc",locStr.lat ,locStr.lng)
    --Successfully obtained latitude and longitude, coordinate system WGS84
    if result == 0 then
        local aid = libgnss.casic_aid(dt, locStr)
        uart.write(gps_uart_id, aid.."\r\n")
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
    uart.write(gps_uart_id, "$PCAS03,1,1,1,1,1,1,1,1,0,0,1,1,1*33\r\n") --By default, all name statements are open
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
        local rmc = libgnss.getRmc(2)
        local locStr = { lat ,lng }
        locStr.lat = rmc.lat
        locStr.lng = rmc.lng
        local str = json.encode(locStr, "7f")
        io.writeFile("/gnssrmc", str)
        log.info("gnss", "rmc", str)
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
