--- Module function: 780E connected to ctwing platform
--@Modules ctwing_iot
--@author Zhai Science Research
--@license MIT
--@copyright OpenLuat.com
--@release 2023.4.13
local sys = require "sys"
--[[Special note, using the mqtt library requires the following statements]]
_G.sysplus = require("sysplus")

--------------Information that needs to be changed--------------------------
--Device ID
local device_id ="15601013001"
--Account name
local user_name="001"
--Feature string
local password ="8Jtoo3rc9RRYgtaWv5QXxvF15-tzlmgNzk2O6cQeg_o"
--theme
local ctwing_iot_subscribetopic = {
    ["signal_report"]=0 ,["test"]=0,["z1223"]=1 --This demo takes z1223 as an example
}
--------------The above is modified based on personal registration information-------------
local mqtt_client
--MQTT connection status
local mqtt_connected = false

local function ctwing_iot()
    local mobile_signal
    sys.waitUntil("IP_READY_IND",30000)
    mobile_signal=mobile.status()
    log.info("SIM SIGNAL",mobile_signal)
    --Create an MQTT client
    log.info("MQTT CONNECTTING...")
    mqtt_client = mqtt.create( nil ,"mqtt.ctwing.cn", 1883)
    mqtt_client:auth(device_id,user_name,password)--Triple configuration
    mqtt_client:keepalive(240)--Set heartbeat packet interval
    mqtt_client:autoreconn(true, 3000) --Automatic reconnection mechanism
    mqtt_client:on(function(mqtt_client, event, data, payload)                --[[
                event可能出现的值有
                conack --Server authentication is completed, the mqtt connection has been established, and data can be subscribed and published. There is no additional data.
                recv   --The data is received and sent by the server. The data is the topic value (string), and the payload is the business data (string). Metas is metadata (table), which is generally not processed.
                        --metas contains the following content
                        --qos value range 0,1,2
                        --retain value range 0,1
                        --dup value range 0,1
                sent   --After the sending is completed, qos0 will notify immediately, qos1/qos2 will call back when the server responds, and data is the message id.
                disconnect --The server is disconnected, there is a network problem, or the server kicks the client, for example, the clientId is repeated, or the business data is not reported after timeout.
                ]]
                --User defined code
                log.info("mqtt", "event", event, mqtt_client, data, payload)
                if event == "conack" then--
                    log.info("MQTT CONNECTTED")
                    sys.publish("mqtt_conack")
                    mqtt_client:subscribe(ctwing_iot_subscribetopic)--Topic subscription
                    log.info("Successfully subscribed to mqtt")

                elseif event == "recv" then
                    log.info("mqtt", "downlink", "topic", data, "payload", payload)
                    sys.publish("mqtt_payload", data, payload)

                elseif event == "sent" then --Send success signal asynchronously
                    log.info("mqtt", "sent", "pkgid", data)

                end
                end)
                --Automatically handles reconnection unless it closes itself
    mqtt_client:connect()
    sys.waitUntil("mqtt_conack")
    while true do
                    --Demonstrates waiting for reporting information sent by other tasks
                local ret, topic, data, qos = sys.waitUntil("mqtt_pub", 300000)
                local date="test date"--test data
                local qos=1
                if ret then
                    --Provides a way to close this while loop. Comment out if not needed.
                    if topic == "close" then break end
                    mqtt_client:publish("z1223",date,qos)--QOS0 does not have puback, QOS1 has puback
                    log.info("发送成功")
                end
                    --If there are no other tasks to report, you can write an empty wait
                    --sys.wait(60000000)
    end

    mqtt_client:close()
    mqtt_client = nil
end

sys.taskInit(ctwing_iot)
sys.taskInit(function()
    while true do
        sys.wait(60000)
        if mqtt_client and mqtt_client:ready() then
            sys.publish("mqtt_pub",ctwing_iot_subscribetopic.z1223,1)
        end
    end
end)


--return ctwing
