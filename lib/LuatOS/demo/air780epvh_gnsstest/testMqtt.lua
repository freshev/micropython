
local mqttc = nil

--mqtt upload task
sys.taskInit(function()
    sys.waitUntil("IP_READY", 15000)
    mqttc = mqtt.create(nil, "lbsmqtt.airm2m.com", 1886) --mqtt client creation

    mqttc:auth(mobile.imei(), mobile.imei(), mobile.muid()) --mqtt triplet configuration
    log.info("mqtt", mobile.imei(), mobile.imei(), mobile.muid())
    mqttc:keepalive(30) --Default value 240s
    mqttc:autoreconn(true, 3000) --Automatic reconnection mechanism

    mqttc:on(function(mqtt_client, event, data, payload) --mqtt callback registration
        --User-defined code, processed by event
        --log.info("mqtt", "event", event, mqtt_client, data, payload)
        if event == "conack" then --mqtt message after successfully completing authentication
            sys.publish("mqtt_conack") --Topics with lowercase letters are all custom topics.
            --Subscription is not required, but is usually available
            mqtt_client:subscribe("/gnss/" .. mobile.imei() .. "/down/#")
        elseif event == "recv" then --Data sent by the server
            log.info("mqtt", "downlink", "topic", data, "payload", payload)
            local dl = json.decode(data)
            if dl then
                --Test command
                if dl.cmd then
                    --Write uart directly
                    if dl.cmd == "uart" and dl.data then
                        uart.write(gps_uart_id, dl.data)
                    --Restart command
                    elseif dl.cmd == "reboot" then
                        rtos.reboot()
                    elseif dl.cmd == "stat" then
                        upload_stat()
                    end
                end
            end
        elseif event == "sent" then --Events after publish is successful
            log.info("mqtt", "sent", "pkgid", data)
        end
    end)

    --After initiating the connection, the mqtt library will automatically maintain the link. If the connection is disconnected, it will automatically reconnect by default.
    mqttc:connect()
    -- sys.waitUntil("mqtt_conack")
    --log.info("mqtt connection successful")
    sys.timerStart(upload_stat, 3000) --Actively upload once after one second
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

sys.timerLoopStart(upload_stat, 60000)

sys.taskInit(function()
    local msgs = {}
    while 1 do
        local ret, topic, data, qos = sys.waitUntil("uplink", 30000)
        if ret then
            if topic == "close" then
                break
            end
            log.info("mqtt", "publish", "topic", topic)
            --if #data > 512 then
            --local start = mcu.ticks()
            --local cdata = miniz.compress(data)
            --local endt = mcu.ticks() - start
            --if cdata then
            --log.info("miniz", #data, #cdata, endt)
            --     end
            -- end
            if mqttc:ready() then
                local tmp = msgs
                if #tmp > 0 then
                    log.info("mqtt", "ready, send buff", #tmp)
                    msgs = {}
                    for k, msg in pairs(tmp) do
                        mqttc:publish(msg.topic, msg.data, 0)
                    end
                end
                mqttc:publish(topic, data, qos)
            else
                log.info("mqtt", "not ready, insert into buff")
                if #msgs > 60 then
                    table.remove(msgs, 1)
                end
                table.insert(msgs, {
                    topic = topic,
                    data = data
                })
            end
        end
    end
end)

function upload_stat()
    if mqttc == nil or not mqttc:ready() then return end
    local stat = {
        csq = mobile.csq(),
        rssi = mobile.rssi(),
        rsrq = mobile.rsrq(),
        rsrp = mobile.rsrp(),
        --iccid = mobile.iccid(),
        snr = mobile.snr(),
        vbat = adc.get(adc.CH_VBAT),
        temp = adc.get(adc.CH_CPU),
        memsys = {rtos.meminfo("sys")},
        memlua = {rtos.meminfo()},
        fixed = libgnss.isFix()
    }
    sys.publish("uplink", "/gnss/" .. mobile.imei() .. "/up/stat", (json.encode(stat)), 1)
end

sys.timerLoopStart(upload_stat, 60 * 1000)

