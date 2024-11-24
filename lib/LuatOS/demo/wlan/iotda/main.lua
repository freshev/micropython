PROJECT = "gpiodemo"
VERSION = "1.0.0"

--Be sure to add sys.lua!!!!
local sys = require "sys"
require("sysplus")
log.info("main", "iotda demo")


local device_id     = ""    --Change to your own device id
local device_secret = ""    --Change to your own device key

local mqttc = nil

sys.taskInit(function()
    log.info("wlan", "wlan_init:", wlan.init())
    wlan.setMode(wlan.STATION)
    wlan.connect("CMCC_EDU", "88995500", 1)
    local result, data = sys.waitUntil("IP_READY")
    log.info("wlan", "IP_READY", result, data)
    
    local client_id,user_name,password = iotauth.iotda(device_id,device_secret)
    log.info("iotda",client_id,user_name,password)
    
    mqttc = mqtt.create(nil,"a16203e7a0.iot-mqtts.cn-north-4.myhuaweicloud.com", 1883)

    mqttc:auth(client_id,user_name,password)
    mqttc:keepalive(30) --Default value 240s
    mqttc:autoreconn(true, 3000) --Automatic reconnection mechanism

    mqttc:on(function(mqtt_client, event, data, payload)
        --User defined code
        log.info("mqtt", "event", event, mqtt_client, data, payload)
        if event == "conack" then
            sys.publish("mqtt_conack")
            mqtt_client:subscribe("/luatos/123456")
        elseif event == "recv" then
            log.info("mqtt", "downlink", "topic", data, "payload", payload)
        elseif event == "sent" then
            log.info("mqtt", "sent", "pkgid", data)
        end
    end)

    mqttc:connect()
	sys.waitUntil("mqtt_conack")
    while true do
        --mqttc automatically handles reconnection
        local ret, topic, data, qos = sys.waitUntil("mqtt_pub", 30000)
        if ret then
            if topic == "close" then break end
            mqttc:publish(topic, data, qos)
        end
    end
    mqttc:close()
    mqttc = nil
end)

sys.taskInit(function()
	local topic = "/luatos/123456"
	local payload = "123"
	local qos = 1
    local result, data = sys.waitUntil("IP_READY")
    while true do
        sys.wait(5000)
        if mqttc and mqttc:ready() then
            local pkgid = mqttc:publish(topic, payload, qos)
        end
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
