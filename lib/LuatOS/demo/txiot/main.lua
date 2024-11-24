--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "txiot_demo"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the mqtt library requires the following statements]]
_G.sysplus = require("sysplus")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


--Product ID and product dynamic registration key
-- https://console.cloud.tencent.com/iotexplorer
--To enable "Dynamic Registration Configuration" in project details, both "Dynamic Registration" and "Automatic Device Creation" need to be enabled.
--The following parameters are for testing and must be changed to your own values.
local ProductId = "T7ASGCON0H"
local ProductSecret = "cAnsvBBNquvTWYIihUPoxkp2"
local mqttc
local mqtt_isssl = false

--This demo is purely handwritten. You can also refer to the iotcloud library and its demo/iotcloud for a more consistent cloud service experience.

--[[
函数名：getDeviceName
功能  ：获取设备名称
参数  ：无
返回值：设备名称
]]
local function getDeviceName()
    --By default, the IMEI of the device is used as the device name, and users can modify it according to project needs.
    return mobile.imei()
end



function device_enrol()
    local deviceName = getDeviceName()
    local nonce = math.random(1, 100)
    local timestamp = os.time()
    local data = "deviceName=" .. deviceName .. "&nonce=" .. nonce .. "&productId=" ..
        ProductId .. "&timestamp=" .. timestamp
    local hmac_sha1_data = crypto.hmac_sha1(data, ProductSecret):lower()
    local signature = crypto.base64_encode(hmac_sha1_data)
    local tx_body = {
        productId = ProductId,
        deviceName = deviceName,
        nonce = nonce,
        timestamp = timestamp,
        signature = signature,
    }
    local tx_body_json = json.encode(tx_body)
    local code, headers, body = http.request("POST", "https://ap-guangzhou.gateway.tencentdevices.com/register/dev",
        {
            ["Content-Type"] = "application/json; charset=UTF-8",
            ["X-TC-Version"] = "2019-04-23",
            ["X-TC-Region"] = "ap-guangzhou"
        }, tx_body_json, { timeout = 30000 }).wait()
    log.info("http.post", code, headers, body)
    if code == 200 then
        local m, result, err = json.decode(body)
        log.info(" m,result,err", m, result, err)
        if result == 0 then
            log.info("json解析失败", err)
            device_enrol()
        end
        if m.message == "success" then
            log.info("腾讯云注册设备成功:", body)
            log.info("http.body.message", m.message)
            local result = io.writeFile("/txiot.dat", body)
            log.info("密钥写入结果", result)
        else
            log.info("腾讯云注册设备失败:失败原因", m.message)
        end
    else
        log.info("http请求失败:", body)
    end
end

sys.subscribe("MQTT_SIGN_AUTH", function(clientid, username, password)
    sys.taskInit(function()
        log.info("clientid,username,password", result, clientid, username, password, payload)

        local mqtt_host = ProductId .. ".iotcloud.tencentdevices.com"
        mqttc = mqtt.create(nil, mqtt_host, 1883, mqtt_isssl)
        mqttc:auth(clientid, username, password, false) --client_id is required, the rest are optional
        mqttc:keepalive(300)                            --Default 300s
        mqttc:autoreconn(true, 3000)                    --Automatic reconnection mechanism
        mqttc:on(function(mqtt_client, event, data, payload)
            --User defined code
            log.info("mqtt", "event", event, mqtt_client, data, payload)
            if event == "conack" then
                log.info("mqtt", "sent", "pkgid", data)
                --Connected
                sys.publish("mqtt_conack")
                local txiot_subscribetopic = {
                    ["$thing/down/property/" .. ProductId .. "/" .. getDeviceName()] = 0
                }
                mqtt_client:subscribe(txiot_subscribetopic)
            elseif event == "recv" then
                log.info("mqtt", "downlink", "topic", data, "payload", payload)
                --TODO: Process data.payload according to your needs
                --sys.publish("mqtt_payload", data, payload)
                mqttc:publish("$thing/up/property/" .. ProductId .. "/" .. getDeviceName(),
                    "publish from luat mqtt client",
                    0)
            elseif event == "sent" then
                log.info("mqtt", "sent", "pkgid", data)
            elseif event == "disconnect" then
                log.info("连接失败")
                ----When not automatically reconnecting, restart mqttc as needed
                --     mqtt_client:connect()
            end
        end)
        mqttc:connect()
        sys.waitUntil("mqtt_conack")
        while true do
            --If there are no other tasks to report, you can write an empty wait
            sys.wait(60000000)
        end
    end)
end)


sys.subscribe("MQTT_CERT_AUTH", function() ---Certificate authentication connection
    sys.taskInit(function()
        local clientid = ProductId .. getDeviceName()
        local connid = math.random(10000, 99999)
        log.info("connid类型", type(connid))
        local expiry = "32472115200"
        local username = string.format("%s;12010126;%s;%s", clientid, connid, expiry) --Generate the username part of MQTT in the format ${clientid};${sdkappid};${connid};${expiry}
        local password = 123                                                          --Certificate authentication does not verify password
        log.info("clientid1,username1,password1", clientid, username, password)
        local mqtt_host = ProductId .. ".iotcloud.tencentdevices.com"
        mqttc = mqtt.create(nil, mqtt_host, 8883,
        { server_cert = io.readFile("/luadb/ca.crt"),
        client_cert = io.readFile("/client.crt"),
        client_key = io.readFile("/client.key") })
        mqttc:auth(clientid, username, password) --client_id is required, the rest are optional
        mqttc:keepalive(300)                     --Default value 300s
        mqttc:autoreconn(true, 20000)            --Automatic reconnection mechanism


        mqttc:on(function(mqtt_client, event, data, payload)
            --User defined code
            log.info("mqtt", "event", event, mqtt_client, data, payload)
            if event == "conack" then
                log.info("mqtt", "sent", "pkgid", data)
                --Connected
                sys.publish("mqtt_conack")
                local txiot_subscribetopic = {
                    ["$thing/down/property/" .. ProductId .. "/" .. getDeviceName()] = 0
                }
                mqtt_client:subscribe(txiot_subscribetopic)
            elseif event == "recv" then
                log.info("mqtt", "downlink", "topic", data, "payload", payload)
                --TODO: Process data.payload according to your needs
                --sys.publish("mqtt_payload", data, payload)
                mqttc:publish("$thing/up/property/" .. ProductId .. "/" .. getDeviceName(),
                    "publish from luat mqtt client",
                    0)
            elseif event == "sent" then
                log.info("mqtt", "sent", "pkgid", data)
            elseif event == "disconnect" then
                log.info("连接失败")
                ----When not automatically reconnecting, restart mqttc as needed
                --     mqtt_client:connect()
            end
        end)
        local result = mqttc:connect()
        log.info("connect.result", result)
        sys.waitUntil("mqtt_conack")

        while true do
            --If there are no other tasks to report, you can write an empty wait
            sys.wait(60000000)
        end
    end)
end)

sys.taskInit(function()
    if mobile.status() ~= 1 and not sys.waitUntil("IP_READY", 600000) then
        log.info("网络初始化失败！")
    end
    log.info("io.exists", io.exists("/txiot.dat"))
    if not io.exists("/txiot.dat") then
        device_enrol()
    end
    if not io.exists("/txiot.dat") then
        log.info("txiot", "注册失败")
        return
    end
    local dat, result, err = json.decode(io.readFile("/txiot.dat"))
    log.info("dat,result,err", dat, result, err)
    if result == 0 then
        log.info("json解码失败", err)
        device_enrol() --Parsing failed and re-downloading the file
        local dat, result, err = json.decode(io.readFile("/txiot.dat"))
    end
    local payload = json.decode(crypto.cipher_decrypt("AES-128-CBC", "ZERO", crypto.base64_decode(dat.payload),
        string.sub(ProductSecret, 1, 16), "0000000000000000"))
    log.info("payload[encryptionType]", payload.encryptionType)
    log.info("payload[psk]", payload.psk)
    if payload.encryptionType == 2 then
        local clientid, username, password = iotauth.qcloud(ProductId, getDeviceName(), payload.psk)
        sys.publish("MQTT_SIGN_AUTH", clientid, username, password) --Signature authentication
    elseif payload.encryptionType == 1 then
        log.info("payload date ", payload.encryptionType, payload.psk, payload.clientCert, payload.clientKey)
        io.writeFile("/client.crt", payload.clientCert)
        io.writeFile("/client.key", payload.clientKey)
        sys.publish("MQTT_CERT_AUTH") --Certificate authentication
    end
end)




--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
