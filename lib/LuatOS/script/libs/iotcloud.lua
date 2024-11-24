--[[
@module iotcloud
@summary iotcloud 云平台库 (已支持: 腾讯云 阿里云 onenet 华为云 涂鸦云 百度云 Tlink云 其他也会支持,有用到的提issue会加速支持)  
@version 1.0
@date    2023.06.19
@author  Dozingfiretruck
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.

    --Tencent Cloud
    --Dynamic registration
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx" ,product_secret = "xxx"})

    --Key verification
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx",device_name = "123456789",key = "xxx=="})
    --Certificate verification
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx",device_name = "123456789"},{tls={client_cert=io.readFile("/luadb/client_cert.crt")}})


    --Alibaba Cloud
    --Dynamic registration (no pre-registration required) (one type, one password)
    --iotcloudc = iotcloud.new(iotcloud.ALIYUN,{produt_id = "xxx",product_secret = "xxx"})

    --Key verification (pre-registration) (one machine, one password)
    --iotcloudc = iotcloud.new(iotcloud.ALIYUN,{produt_id = "xxx",device_name = "xxx",key = "xxx"})
    --Certificate verification (pre-registration)
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx",device_name = "xxx"},{tls={client_cert=io.readFile("/luadb/client_cert.crt")}})

    --ONENET Cloud
    --Dynamic registration
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",userid = "xxx",userkey = "xxx"})
    --One type, one secret
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",product_secret = "xxx"})
    --One machine, one secret
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",device_name = "xxx",key = "xxx"})

    --Huawei Cloud
    --Dynamic registration (no pre-registration required)
    -- iotcloudc = iotcloud.new(iotcloud.HUAWEI,{produt_id = "xxx",project_id = "xxx",endpoint = "xxx",
    --                         iam_username="xxx",iam_password="xxx",iam_domain="xxx"})
    --Key verification (pre-registration)
    -- iotcloudc = iotcloud.new(iotcloud.HUAWEI,{produt_id = "xxx",endpoint = "xxx",device_name = "xxx",device_secret = "xxx"})

    ---- Tuya Cloud
    --iotcloudc = iotcloud.new(iotcloud.TUYA,{device_name = "xxx",device_secret = "xxx"})

    --Baidu Cloud
    --iotcloudc = iotcloud.new(iotcloud.BAIDU,{produt_id = "xxx",device_name = "xxx",device_secret = "xxx"})

    --Tlink cloud
    --iotcloudc = iotcloud.new(iotcloud.TLINK,{produt_id = "xxx",product_secret = "xxx",device_name = "xxx"})
    --iotcloudc = iotcloud.new(iotcloud.TLINK,{produt_id = "xxx",product_secret = "xxx",device_name = "xxx"},{tls={client_cert=io.readFile("/luadb/client_cert.crt")}})

]]



local iotcloud = {}
--Cloud platform
--//@const TENCENT string Tencent Cloud
iotcloud.TENCENT            = "tencent"     --Tencent Cloud
--//@const ALIYUN string Alibaba Cloud
iotcloud.ALIYUN             = "aliyun"      --Alibaba Cloud
--//@const ONENET string ONENET云
iotcloud.ONENET             = "onenet"      --ONENET Cloud
--//@const HUAWEI string Huawei Cloud
iotcloud.HUAWEI             = "huawei"      --Huawei Cloud
--//@const TUYA string Tuya Cloud
iotcloud.TUYA               = "tuya"        --graffiti cloud
--//@const BAIDU string Baidu Cloud
iotcloud.BAIDU               = "baidu"      --Baidu Cloud
--//@const TLINK string Tlink云
iotcloud.TLINK               = "tlink"      --Tlink cloud

--Authentication method
local iotcloud_certificate  = "certificate" --Key authentication
local iotcloud_key          = "key"         --Certificate authentication
-- event
--//@const CONNECT string connect to the server
iotcloud.CONNECT            = "connect"     --Connect to the server
--//@const SEND string send message
iotcloud.SEND               = "SEND"        --Send message
--//@const RECEIVE string message received
iotcloud.RECEIVE            = "receive"     --message received
--//@const DISCONNECT string The server connection is disconnected
iotcloud.DISCONNECT         = "disconnect"  --Server connection lost
--//@const OTA string ota message
iotcloud.OTA                = "ota"         --ota news


local cloudc_table = {}     --iotcloudc object table

local cloudc = {}
cloudc.__index = cloudc

--Cloud platform connection success processing function, here you can subscribe to some topics or report versions and other default operations
local function iotcloud_connect(iotcloudc)
    --mqtt_client:subscribe({[topic1]=1,[topic2]=1,[topic3]=1})--Multiple topic subscription
    if iotcloudc.cloud == iotcloud.TENCENT then     --Tencent Cloud
        iotcloudc:subscribe("$ota/update/"..iotcloudc.product_id.."/"..iotcloudc.device_name)    --Subscribe to ota topic
        iotcloudc:publish("$ota/report/"..iotcloudc.product_id.."/"..iotcloudc.device_name,"{\"type\":\"report_version\",\"report\":{\"version\": \"".._G.VERSION.."\"}}")   --Report ota version information
    elseif iotcloudc.cloud == iotcloud.ALIYUN then  --Alibaba Cloud
        iotcloudc:subscribe("/ota/device/upgrade/"..iotcloudc.product_id.."/"..iotcloudc.device_name)    --Subscribe to ota topic
        iotcloudc:publish("/ota/device/inform/"..iotcloudc.product_id.."/"..iotcloudc.device_name,"{\"id\":1,\"params\":{\"version\":\"".._G.VERSION.."\"}}")   --Report ota version information
    elseif iotcloudc.cloud == iotcloud.ONENET then  --China Mobile Cloud
    elseif iotcloudc.cloud == iotcloud.HUAWEI then  --Huawei Cloud
        iotcloudc:subscribe("$oc/devices/"..iotcloudc.device_id.."/sys/events/down")    --Subscribe to ota topic
        iotcloudc:publish("$oc/devices/"..iotcloudc.device_id.."/sys/events/up","{\"services\":[{\"service_id\":\"$ota\",\"event_type\":\"version_report\",\"paras\":{\"fw_version\":\"".._G.VERSION.."\"}}]}")   --Report ota version information
    elseif iotcloudc.cloud == iotcloud.TUYA then  --graffiti cloud
    elseif iotcloudc.cloud == iotcloud.TLINK then  --Tlink cloud
        iotcloudc:subscribe(iotcloudc.device_name.."/+")    --Subscribe to topic
        
    end
end

local function http_downloald_callback(content_len,body_len,iotcloudc)
    -- print("http_downloald_callback-------------------",content_len,body_len)
    if iotcloudc.cloud == iotcloud.TENCENT then
        if body_len == 0 then
            --Start upgrading type: message type state: status is burning
            iotcloudc:publish("$ota/report/"..iotcloudc.product_id.."/"..iotcloudc.device_name,"{\"type\":\"report_progress\",\"report\":{\"progress\":{\"state\": \"burning\",\"result_code\": \"0\",\"result_msg\": \"\"}},\"version\": \""..iotcloudc.ota_version.."\"}")
        else
            --Download progress type: message type state: status is downloading percent: current download progress, percentage
            iotcloudc:publish("$ota/report/"..iotcloudc.product_id.."/"..iotcloudc.device_name,"{\"type\":\"report_progress\",\"report\":{\"progress\":{\"state\": \"downloading\",\"percent\": \""..body_len*100//content_len.."\",\"result_code\": \"0\",\"result_msg\": \"\"}},\"version\": \""..iotcloudc.ota_version.."\"}")
        end
    elseif iotcloudc.cloud == iotcloud.HUAWEI then
        iotcloudc:publish("$oc/devices/"..iotcloudc.device_id.."/sys/events/up","{\"services\":[{\"service_id\":\"$ota\",\"event_type\":\"upgrade_progress_report\",\"paras\":{\"result_code\":\"0\",\"version\":\"".._G.VERSION.."\",\"progress\":\""..(body_len*100//content_len - 1).."\"}}]}")   --Report ota version information
    end
end 

local function iotcloud_ota_download(iotcloudc,ota_payload,config)
    local ota_url = nil
    local ota_headers = nil
    local body_ota = nil
    config.callback = http_downloald_callback
    config.userdata = iotcloudc

    if iotcloudc.cloud == iotcloud.TENCENT then
        ota_url = ota_payload.url
    elseif iotcloudc.cloud == iotcloud.ALIYUN then
        ota_url = ota_payload.data.url
    elseif iotcloudc.cloud == iotcloud.HUAWEI then
        ota_url = ota_payload.services[1].paras.url
        ota_headers = {["Content-Type"]="application/json;charset=UTF-8",["Authorization"]="Bearer "..ota_payload.services[1].paras.access_token}
    end

    local code, headers, body = http.request("GET", ota_url, ota_headers, body_ota, config).wait()
    --log.info("ota download", code, headers, body) -- only returns code and headers
    if code == 200 or code == 206 then
        if iotcloudc.cloud == iotcloud.TENCENT then  --This is Tencent Cloud
            --Upgrade successful type: message type state: status is completed
            iotcloudc:publish("$ota/report/"..iotcloudc.product_id.."/"..iotcloudc.device_name,"{\"type\":\"report_progress\",\"report\":{\"progress\":{\"state\": \"done\",\"result_code\": \"0\",\"result_msg\": \"\"}},\"version\": \""..iotcloudc.ota_version.."\"}")
        elseif iotcloudc.cloud == iotcloud.ALIYUN then --This is Alibaba Cloud
            
        elseif iotcloudc.cloud == iotcloud.HUAWEI then
            iotcloudc:publish("$oc/devices/"..iotcloudc.device_id.."/sys/events/up","{\"services\":[{\"service_id\":\"$ota\",\"event_type\":\"upgrade_progress_report\",\"paras\":{\"result_code\":\"0\",\"version\":\""..iotcloudc.ota_version.."\",\"progress\":\"100\"}}]}")   --Report ota version information
        end
    else
        if iotcloudc.cloud == iotcloud.TENCENT then  --This is Tencent Cloud
            --Upgrade failure type: message type state: status is failed result_code: error code, -1: download timeout; -2: file does not exist; -3: signature expired; -4: MD5 does not match; -5: firmware update failed result_msg: error message
            iotcloudc:publish("$ota/report/"..iotcloudc.product_id.."/"..iotcloudc.device_name,"{\"type\":\"report_progress\",\"report\":{\"progress\":{\"state\": \"fail\",\"result_code\": \"-5\",\"result_msg\": \"ota_fail\"}},\"version\": \""..iotcloudc.ota_version.."\"}")
        elseif iotcloudc.cloud == iotcloud.ALIYUN then  --This is Alibaba Cloud
            
        elseif iotcloudc.cloud == iotcloud.HUAWEI then
            iotcloudc:publish("$oc/devices/"..iotcloudc.device_id.."/sys/events/up","{\"services\":[{\"service_id\":\"$ota\",\"event_type\":\"upgrade_progress_report\",\"paras\":{\"result_code\":\"255\"}}]}")   --Report ota version information
        end
    end
    sys.publish("iotcloud", iotcloudc, iotcloud.OTA,code == 200 or code == 206)
end

--iotcloud mqtt callback function
local function iotcloud_mqtt_callback(mqtt_client, event, data, payload)
    local iotcloudc = nil
    --Traverse out iotcloudc
    for k, v in pairs(cloudc_table) do
        if v.mqttc == mqtt_client then
            iotcloudc = v
        end
    end

    local isfota,otadst
    if fota then isfota = true else otadst = "/update.bin" end
    --otadst = "/update.bin" --test

    --print("iotcloud_mqtt_callback",mqtt_client, event, data, payload)
    --User defined code
    if event == "conack" then                               --Connect to the server
        iotcloud_connect(iotcloudc)
        sys.publish("iotcloud",iotcloudc,iotcloud.CONNECT, data, payload)
    elseif event == "recv" then                             --message received
        if iotcloudc.cloud == iotcloud.TENCENT and data == "$ota/update/"..iotcloudc.product_id.."/"..iotcloudc.device_name then --Tencent cloud ota
            local ota_payload = json.decode(payload)
            if ota_payload.type == "update_firmware" then
                iotcloudc.ota_version = ota_payload.version
                sys.taskInit(iotcloud_ota_download,iotcloudc,ota_payload,{fota=isfota,dst=otadst,timeout = 120000})
                
            end
        elseif iotcloudc.cloud == iotcloud.ALIYUN and data == "/ota/device/upgrade/"..iotcloudc.product_id.."/"..iotcloudc.device_name then --Alibaba cloud ota
            local ota_payload = json.decode(payload)
            if ota_payload.message == "success" then
                iotcloudc.ota_version = ota_payload.version
                sys.taskInit(iotcloud_ota_download,iotcloudc,ota_payload,{fota=isfota,dst=otadst,timeout = 120000})
            end
        elseif iotcloudc.cloud == iotcloud.HUAWEI and data == "$oc/devices/"..iotcloudc.device_id.."/sys/events/down" then --Huawei cloud ota
            local ota_payload = json.decode(payload)
            if ota_payload.services[1].event_type == "version_query" then
                iotcloudc:publish("$oc/devices/"..iotcloudc.device_id.."/sys/events/up","{\"services\":[{\"service_id\":\"$ota\",\"event_type\":\"version_report\",\"paras\":{\"fw_version\":\"".._G.VERSION.."\"}}]}")   --Report ota version information
            elseif ota_payload.services[1].event_type == "firmware_upgrade" then
                iotcloudc.ota_version = ota_payload.services[1].paras.version
                sys.taskInit(iotcloud_ota_download,iotcloudc,ota_payload,{fota=isfota,dst=otadst,timeout = 120000})
            end
        else
            sys.publish("iotcloud", iotcloudc, iotcloud.RECEIVE,data,payload)
        end
    elseif event == "sent" then                             --Send message
        sys.publish("iotcloud", iotcloudc, iotcloud.SEND,data,payload)
    elseif event == "disconnect" then                       --Server connection lost
        sys.publish("iotcloud", iotcloudc, iotcloud.DISCONNECT)
    end
end

--Tencent Cloud automatic registration
local function iotcloud_tencent_autoenrol(iotcloudc)
    local deviceName = iotcloudc.device_name
    local nonce = math.random(1,100)
    local timestamp = os.time()
    local data = "deviceName="..deviceName.."&nonce="..nonce.."&productId="..iotcloudc.product_id.."&timestamp="..timestamp
    local hmac_sha1_data = crypto.hmac_sha1(data,iotcloudc.product_secret):lower()
    local signature = crypto.base64_encode(hmac_sha1_data)
    local cloud_body = {
        deviceName=deviceName,
        nonce=nonce,
        productId=iotcloudc.product_id,
        timestamp=timestamp,
        signature=signature,
    }
    local cloud_body_json = json.encode(cloud_body)
    local code, headers, body = http.request("POST","https://ap-guangzhou.gateway.tencentdevices.com/register/dev", 
            {["Content-Type"]="application/json;charset=UTF-8"},
            cloud_body_json
    ).wait()
    if code == 200 then
        local dat, result, errinfo = json.decode(body)
        if result then
            if dat.code==0 then
                local payload = crypto.cipher_decrypt("AES-128-CBC","ZERO",crypto.base64_decode(dat.payload),string.sub(iotcloudc.product_secret,1,16),"0000000000000000")
                local payload = json.decode(payload)
                fskv.set("iotcloud_tencent", payload)
                if payload.encryptionType == 1 then     --Certificate authentication
                    iotcloudc.authentication = iotcloud_certificate
                elseif payload.encryptionType == 2 then --Key authentication
                    iotcloudc.authentication = iotcloud_key
                end
                return true
            else
                log.info("http.post", code, headers, body)
                return false
            end
        end
    else
        log.info("http.post", code, headers, body)
        return false
    end
end

local function iotcloud_aliyun_callback(mqtt_client, event, data, payload)
    --log.info("mqtt", "event", event, mqtt_client, data, payload)
    if data == "/ext/regnwl" then sys.publish("aliyun_autoenrol", payload) end
    if event == "disconnect" then mqtt_client:close() end
end

--Alibaba Cloud automatic registration
local function iotcloud_aliyun_autoenrol(iotcloudc)
    local random = math.random(1,999)
    local data = "deviceName"..iotcloudc.device_name.."productKey"..iotcloudc.product_id.."random"..random
    local mqttClientId = iotcloudc.device_name.."|securemode=-2,authType=regnwl,random="..random..",signmethod=hmacsha1|"
    local mqttUserName = iotcloudc.device_name.."&"..iotcloudc.product_id
    local mqttPassword = crypto.hmac_sha1(data,iotcloudc.product_secret):lower()
    -- print("iotcloud_aliyun_autoenrol",mqttClientId,mqttUserName,mqttPassword)
    aliyun_mqttc = mqtt.create(nil, iotcloudc.product_id..".iot-as-mqtt.cn-shanghai.aliyuncs.com", 443,true)
    aliyun_mqttc:auth(mqttClientId,mqttUserName,mqttPassword)
    aliyun_mqttc:on(iotcloud_aliyun_callback)
    aliyun_mqttc:connect()
    local result, payload = sys.waitUntil("aliyun_autoenrol", 30000)
    --print("aliyun_autoenrol",result, payload)
    if result then
        local payload = json.decode(payload)
        fskv.set("iotcloud_aliyun", payload)
        --print("aliyun_autoenrol payload",payload.clientId, payload.deviceToken)
        return true
    else
        return false
    end
end

--Tencent Cloud parameter configuration logic
local function iotcloud_tencent_config(iotcloudc,iot_config,connect_config)
    iotcloudc.cloud = iotcloud.TENCENT
    iotcloudc.product_id = iot_config.product_id
    if iot_config.product_secret then                       --Product_secret indicates that it is dynamic registration
        iotcloudc.product_secret = iot_config.product_secret
        if not fskv.get("iotcloud_tencent") then 
            if not iotcloud_tencent_autoenrol(iotcloudc) then return false end
        end
        local data = fskv.get("iotcloud_tencent")
        -- print("payload",data.encryptionType,data.psk,data.clientCert,data.clientKey)
        if data.encryptionType == 1 then                    --Certificate authentication
            iotcloudc.ip = 8883
            iotcloudc.isssl = true
            iotcloudc.ca_file = {client_cert = data.clientCert,client_key = data.clientKey}
            iotcloudc.client_id,iotcloudc.user_name = iotauth.qcloud(iotcloudc.product_id,iotcloudc.device_name,"")
        elseif data.encryptionType == 2 then                --Key authentication
            iotcloudc.ip = 1883
            iot_config.key = data.psk
            iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password = iotauth.qcloud(iotcloudc.product_id,iotcloudc.device_name,iot_config.key,iot_config.method)
        end
    else                                                    --Otherwise, it is non-dynamic registration.
        if iot_config.key then                              --Key authentication
            iotcloudc.ip = 1883
            iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password = iotauth.qcloud(iotcloudc.product_id,iotcloudc.device_name,iot_config.key,iot_config.method)
        elseif connect_config.tls then                      --Certificate authentication
            iotcloudc.ip = 8883
            iotcloudc.isssl = true
            iotcloudc.ca_file = {client_cert = connect_config.tls.client_cert}
            iotcloudc.client_id,iotcloudc.user_name = iotauth.qcloud(iotcloudc.product_id,iotcloudc.device_name,"")
        else                                                --No key certificate
            return false
        end
    end
    if connect_config then
        iotcloudc.host = connect_config.host or iotcloudc.product_id..".iotcloud.tencentdevices.com"
        if connect_config.ip then iotcloudc.ip = connect_config.ip end
    else
        iotcloudc.host = iotcloudc.product_id..".iotcloud.tencentdevices.com"
    end
    return true
end

--Alibaba Cloud parameter configuration logic
local function iotcloud_aliyun_config(iotcloudc,iot_config,connect_config)
    iotcloudc.cloud = iotcloud.ALIYUN
    iotcloudc.product_id = iot_config.product_id
    if iot_config.product_secret then                       --Product_secret indicates that it is dynamic registration
        iotcloudc.product_secret = iot_config.product_secret
        if not fskv.get("iotcloud_aliyun") then 
            if not iotcloud_aliyun_autoenrol(iotcloudc) then return false end
        end
        local data = fskv.get("iotcloud_aliyun")
        --print("aliyun_autoenrol payload",data.clientId, data.deviceToken)
        iotcloudc.client_id = data.clientId.."|securemode=-2,authType=connwl|"
        iotcloudc.user_name = iotcloudc.device_name.."&"..iotcloudc.product_id
        iotcloudc.password = data.deviceToken
        iotcloudc.ip = 1883
    else                                                    --Otherwise, it is non-dynamic registration.
        if iot_config.key then                              --Key authentication
            iotcloudc.ip = 1883
            iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password = iotauth.aliyun(iotcloudc.product_id,iotcloudc.device_name,iot_config.key,iot_config.method)
        --elseif connect_config.tls then -- Certificate authentication
        --iotcloudc.ip = 443
        --iotcloudc.isssl = true
        --iotcloudc.ca_file = {client_cert = connect_config.tls.client_cert}
        --iotcloudc.client_id,iotcloudc.user_name = iotauth.aliyun(iotcloudc.product_id,iotcloudc.device_name,"",iot_config.method,nil,true)
        else                                                --No key certificate
            return false
        end
    end
    if connect_config then
        iotcloudc.host = connect_config.host or iotcloudc.product_id..".iot-as-mqtt.cn-shanghai.aliyuncs.com"
        if connect_config.ip then iotcloudc.ip = connect_config.ip end
    else
        iotcloudc.host = iotcloudc.product_id..".iot-as-mqtt.cn-shanghai.aliyuncs.com"
    end
    return true
end

--China Mobile Cloud automatic registration
local function iotcloud_onenet_autoenrol(iotcloudc)
    local version = '2022-05-01'
    local res = "userid/"..iotcloudc.userid
    local et = '32472115200'
    local method = 'SHA256'
    local key = crypto.base64_decode(iotcloudc.userkey)
    local StringForSignature  = et .. '\n' .. method .. '\n' .. res ..'\n' .. version
    local sign1 = crypto.hmac_sha256(StringForSignature,key)
    local sign2 = sign1:fromHex()
    local sign = crypto.base64_encode(sign2)
    sign = string.urlEncode(sign)
    res = string.urlEncode(res)
    local token = string.format('version=%s&res=%s&et=%s&method=%s&sign=%s',version, res, et, method, sign)
    local code, headers, body = http.request("POST","https://iot-api.heclouds.com/device/create", 
            {["Content-Type"]="application/json;charset=UTF-8",["authorization"]=token},
            "{\"product_id\":\""..iotcloudc.product_id.."\",\"device_name\":\""..iotcloudc.device_name.."\"}"
    ).wait()
    if code == 200 then
        local dat, result, errinfo = json.decode(body)
        if result then
            if dat.code==0 then
                fskv.set("iotcloud_onenet", dat.data.sec_key)
                return true
            else
                log.info("http.post", code, headers, body)
                return false
            end
        end
    else
        log.info("http.post", code, headers, body)
        return false
    end
end

--China Mobile Cloud parameter configuration logic
local function iotcloud_onenet_config(iotcloudc,iot_config,connect_config)
    iotcloudc.cloud = iotcloud.ONENET
    iotcloudc.product_id = iot_config.product_id
    iotcloudc.host  = "mqtts.heclouds.com"
    iotcloudc.ip    = 1883
    if iot_config.product_secret then                       --One type, one secret
        iotcloudc.product_secret = iot_config.product_secret
        local version = '2018-10-31'
        local res = "products/"..iotcloudc.product_id
        local et = '32472115200'
        local method = 'sha256'
        local key = crypto.base64_decode(iotcloudc.product_secret)
        local StringForSignature  = et .. '\n' .. method .. '\n' .. res ..'\n' .. version
        local sign1 = crypto.hmac_sha256(StringForSignature,key)
        local sign2 = sign1:fromHex()
        local sign = crypto.base64_encode(sign2)
        sign = string.urlEncode(sign)
        res = string.urlEncode(res)
        local token = string.format('version=%s&res=%s&et=%s&method=%s&sign=%s',version, res, et, method, sign)
        iotcloudc.client_id = iotcloudc.device_name
        iotcloudc.user_name = iotcloudc.product_id
        iotcloudc.password = token
    elseif iot_config.key then                              --One machine, one secret
        iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password = iotauth.onenet(iotcloudc.product_id,iotcloudc.device_name,iot_config.key)
    elseif iot_config.userid and iot_config.userkey then    --Dynamic registration
        iotcloudc.userid = iot_config.userid
        iotcloudc.userkey = iot_config.userkey
        if not fskv.get("iotcloud_onenet") then 
            if not iotcloud_onenet_autoenrol(iotcloudc) then return false end
        end
        local data = fskv.get("iotcloud_onenet")
        --print("fskv.get data",data)
        iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password = iotauth.iotda(iotcloudc.product_id,iotcloudc.device_name,data)
    end
    return true
end

--Huawei Cloud automatic registration
local function iotcloud_huawei_autoenrol(iotcloudc)
    local token_code, token_headers, token_body = http.request("POST","https://iam."..iotcloudc.region..".myhuaweicloud.com/v3/auth/tokens", 
            {["Content-Type"]="application/json;charset=UTF-8"},
            "{\"auth\":{\"identity\":{\"methods\":[\"password\"],\"password\":{\"user\":{\"domain\":{\"name\":\""..iotcloudc.iam_domain.."\"},\"name\":\""..iotcloudc.iam_username.."\",\"password\":\""..iotcloudc.iam_password.."\"}}},\"scope\":{\"project\":{\"name\":\""..iotcloudc.region.."\"}}}}"
    ).wait()
    if token_code ~= 201 then
        log.error("iotcloud_huawei_autoenrol",token_body)
        return false
    end

    local http_url = "https://"..iotcloudc.endpoint..".iotda."..iotcloudc.region..".myhuaweicloud.com/v5/iot/"..iotcloudc.project_id.."/devices"
    local code, headers, body = http.request("POST",http_url, 
            {["Content-Type"]="application/json;charset=UTF-8",["X-Auth-Token"]=token_headers["X-Subject-Token"]},
            "{\"node_id\": \""..iotcloudc.device_name.."\",\"product_id\": \""..iotcloudc.product_id.."\"}"
    ).wait()
    --print("iotcloud_huawei_autoenrol", code, headers, body)
    if code == 201 then
        local dat, result, errinfo = json.decode(body)
        if result then
            fskv.set("iotcloud_huawei", dat.auth_info.secret)
            return true
        end
    else
        log.error("iotcloud_huawei_autoenrol", code, headers, body)
        return false
    end
end

local function iotcloud_huawei_config(iotcloudc,iot_config,connect_config)
    iotcloudc.cloud = iotcloud.HUAWEI
    iotcloudc.region = iot_config.region or "cn-north-4"
    iotcloudc.endpoint = iot_config.endpoint
    iotcloudc.product_id = iot_config.product_id
    iotcloudc.project_id = iot_config.project_id
    iotcloudc.iam_username = iot_config.iam_username
    iotcloudc.iam_password = iot_config.iam_password
    iotcloudc.iam_domain = iot_config.iam_domain
    iotcloudc.device_id = iotcloudc.product_id.."_"..iotcloudc.device_name
    iotcloudc.device_secret = iot_config.device_secret
    iotcloudc.ip = 1883

    if iotcloudc.endpoint then
        iotcloudc.host = iotcloudc.endpoint..".iot-mqtts."..iotcloudc.region..".myhuaweicloud.com"
    else
        log.error("iotcloud","huawei","endpoint is nil")
        return false
    end
    --One type, one password (automatic registration) will eventually obtain the device key
    if iotcloudc.product_id and iotcloudc.project_id and iotcloudc.iam_username and iotcloudc.iam_password and iotcloudc.iam_domain then
        if not fskv.get("iotcloud_huawei") then 
            if not iotcloud_huawei_autoenrol(iotcloudc) then return false end
        end
        iotcloudc.device_secret = fskv.get("iotcloud_huawei")
    end

    if iotcloudc.device_secret then                         --One machine, one secret
        iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password = iotauth.iotda(iotcloudc.device_id,iotcloudc.device_secret)
    else
        return false
    end
    return true
end

--Tuya Cloud parameter configuration logic
local function iotcloud_tuya_config(iotcloudc,iot_config,connect_config)
    iotcloudc.cloud = iotcloud.TUYA
    iotcloudc.host = "m1.tuyacn.com"
    iotcloudc.ip = 8883
    iotcloudc.isssl = true
    iotcloudc.device_secret = iot_config.device_secret
    if iotcloudc.device_secret then
        iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password = iotauth.tuya(iotcloudc.device_name,iotcloudc.device_secret)
    else
        return false
    end
    return true
end

--Baidu cloud parameter configuration logic
local function iotcloud_baidu_config(iotcloudc,iot_config,connect_config)
    iotcloudc.cloud = iotcloud.BAIDU
    iotcloudc.product_id = iot_config.product_id
    iotcloudc.region = iot_config.region or "gz"
    iotcloudc.host = iotcloudc.product_id..".iot."..iotcloudc.region..".baidubce.com"
    iotcloudc.ip = 1883
    --iotcloudc.isssl = true
    iotcloudc.device_secret = iot_config.device_secret
    if iotcloudc.device_secret then
        iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password = iotauth.baidu(iotcloudc.product_id,iotcloudc.device_name,iotcloudc.device_secret)
    else
        return false
    end
    return true
end

--TLINK cloud parameter configuration logic
local function iotcloud_tlink_config(iotcloudc,iot_config,connect_config)
    iotcloudc.cloud = iotcloud.TLINK
    iotcloudc.host = "mq.tlink.io"
    iotcloudc.ip = 1883
    iotcloudc.client_id = iotcloudc.device_name
    iotcloudc.user_name = iot_config.product_id
    iotcloudc.password = iot_config.product_secret
    if connect_config.tls then                      --Certificate authentication
        iotcloudc.ip = 8883
        iotcloudc.isssl = true
        iotcloudc.ca_file = {client_cert = connect_config.tls.client_cert}
    end
    return true
end

--[[
创建云平台对象
@api iotcloud.new(cloud,iot_config,connect_config)
@string 云平台 iotcloud.TENCENT:腾讯云 iotcloud.ALIYUN:阿里云 iotcloud.ONENET:中国移动云 iotcloud.HUAWEI:华为云 iotcloud.TUYA:涂鸦云
@table iot云平台配置, device_name:可选，默认为imei否则为unique_id iot_config.product_id:产品id(阿里云则为产品key) iot_config.product_secret:产品密钥,有此项则为动态注册 iot_config.key:设备秘钥,有此项则为秘钥连接  userid:用户ID,onenet专用,动态注册使用  userkey:用户Accesskey,onenet专用,动态注册使用
@table mqtt配置, host:可选,默认为平台默认host ip:可选,默认为平台默认ip tls:加密,若有此项一般为产品认证 keepalive:心跳时间,单位s 可选,默认240
@return table 云平台对象
@usage

    --Tencent Cloud
    --Dynamic registration
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx" ,product_secret = "xxx"})

    --Key verification
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx",device_name = "123456789",key = "xxx=="})
    --Certificate verification
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx",device_name = "123456789"},{tls={client_cert=io.readFile("/luadb/client_cert.crt")}})


    --Alibaba Cloud
    --Dynamic registration (no pre-registration required) (one type, one password)
    --iotcloudc = iotcloud.new(iotcloud.ALIYUN,{produt_id = "xxx",product_secret = "xxx"})

    --Key verification (pre-registration) (one machine, one password)
    --iotcloudc = iotcloud.new(iotcloud.ALIYUN,{produt_id = "xxx",device_name = "xxx",key = "xxx"})
    --Certificate verification (pre-registration)
    --iotcloudc = iotcloud.new(iotcloud.TENCENT,{produt_id = "xxx",device_name = "xxx"},{tls={client_cert=io.readFile("/luadb/client_cert.crt")}})

    --ONENET Cloud
    --Dynamic registration
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",userid = "xxx",userkey = "xxx"})
    --One type, one secret
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",product_secret = "xxx"})
    --One machine, one secret
    --iotcloudc = iotcloud.new(iotcloud.ONENET,{produt_id = "xxx",device_name = "xxx",key = "xxx"})

    --Huawei Cloud
    --Dynamic registration (no pre-registration required)
    -- iotcloudc = iotcloud.new(iotcloud.HUAWEI,{produt_id = "xxx",project_id = "xxx",endpoint = "xxx",
    --                         iam_username="xxx",iam_password="xxx",iam_domain="xxx"})
    --Key verification (pre-registration)
    -- iotcloudc = iotcloud.new(iotcloud.HUAWEI,{produt_id = "xxx",endpoint = "xxx",device_name = "xxx",device_secret = "xxx"})

    ---- Tuya Cloud
    --iotcloudc = iotcloud.new(iotcloud.TUYA,{device_name = "xxx",device_secret = "xxx"})

    --Baidu Cloud
    --iotcloudc = iotcloud.new(iotcloud.BAIDU,{produt_id = "xxx",device_name = "xxx",device_secret = "xxx"})

    --Tlink cloud
    --iotcloudc = iotcloud.new(iotcloud.TLINK,{produt_id = "xxx",product_secret = "xxx",device_name = "xxx"})
    --iotcloudc = iotcloud.new(iotcloud.TLINK,{produt_id = "xxx",product_secret = "xxx",device_name = "xxx"},{tls={client_cert=io.readFile("/luadb/client_cert.crt")}})

]]
function iotcloud.new(cloud,iot_config,connect_config)
    if not connect_config then connect_config = {} end
    local mqtt_ssl = nil
    local iotcloudc = setmetatable({
        cloud = nil,                                        --Cloud platform
        host = nil,                                         -- host
        ip = nil,                                           -- ip
        mqttc = nil,                                        --mqtt object
        device_name = nil,                                  --Device name (usually device id)
        product_id = nil,                                    --product id
        product_secret = nil,                               --product key
        device_id = nil,                                    --Device id (usually device name)
        device_secret = nil,                                --Device key
        region = nil,                                       --cloud area
        client_id = nil,                                    --mqtt client id
        user_name = nil,                                    --mqtt username
        password = nil,                                     --mqtt password
        authentication = nil,                               --Authentication method: key authentication/certificate authentication
        isssl = nil,                                        --Whether to encrypt
        ca_file = nil,                                      --Certificate
        ota_version = nil,                                  --ota time target version
        userid = nil,                                       --onenet API only
        userkey = nil,                                      --onenet API only
        iam_username = nil,                                 --Huawei Cloud API dedicated IAM user name
        iam_password = nil,                                 --Huawei Cloud API dedicated Huawei Cloud password
        iam_domain = nil,                                   --Huawei Cloud API dedicated account name
        endpoint = nil,                                     --Huawei Cloud API only
        project_id = nil,                                   --Huawei Cloud API only
        
    }, cloudc)
    if fskv then fskv.init() else return false end
    if  iot_config.produt_id then
        iot_config.product_id = iot_config.produt_id
    end
    if iot_config.device_name then                          --If set, use the specified device_name
        iotcloudc.device_name = iot_config.device_name
    elseif mobile then                                      --No priority is set to use imei
        iotcloudc.device_name = mobile.imei()
    else                                                    --Use unique_id without imei
        iotcloudc.device_name = mcu.unique_id():toHex()
    end
    if cloud == iotcloud.TENCENT or cloud == "qcloud" then  --This is Tencent Cloud
        if not iotcloud_tencent_config(iotcloudc,iot_config,connect_config) then return false end
    elseif cloud == iotcloud.ALIYUN then
        if not iotcloud_aliyun_config(iotcloudc,iot_config,connect_config) then return false end
    elseif cloud == iotcloud.ONENET then
        if not iotcloud_onenet_config(iotcloudc,iot_config,connect_config) then return false end
    elseif cloud == iotcloud.HUAWEI then
        if not iotcloud_huawei_config(iotcloudc,iot_config,connect_config) then return false end
    elseif cloud == iotcloud.TUYA then
        if not iotcloud_tuya_config(iotcloudc,iot_config,connect_config) then return false end
    elseif cloud == iotcloud.BAIDU then
        if not iotcloud_baidu_config(iotcloudc,iot_config,connect_config) then return false end
    elseif cloud == iotcloud.TLINK then
        if not iotcloud_tlink_config(iotcloudc,iot_config,connect_config) then return false end
    else
        log.error("iotcloud","cloud not support",cloud)
        return false
    end
    -- print("iotauth.mqtt",iotcloudc.host,iotcloudc.ip,iotcloudc.isssl)
    -- print("iotauth.auth",iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password)
    if iotcloudc.ca_file then
        iotcloudc.ca_file.verify = 1  
        mqtt_ssl = iotcloudc.ca_file
    elseif iotcloudc.isssl then
        mqtt_ssl = iotcloudc.isssl
    end
    iotcloudc.mqttc = mqtt.create(nil, iotcloudc.host, iotcloudc.ip, mqtt_ssl)
    -- iotcloudc.mqttc:debug(true)
    iotcloudc.mqttc:auth(iotcloudc.client_id,iotcloudc.user_name,iotcloudc.password)
    iotcloudc.mqttc:keepalive(connect_config.keepalive or 240)
    iotcloudc.mqttc:autoreconn(true, 3000)                  --Automatic reconnection mechanism
    iotcloudc.mqttc:on(iotcloud_mqtt_callback)              --mqtt callback
    table.insert(cloudc_table,iotcloudc)                    --Add to table record
    return iotcloudc,error_code                             --Error return pending
end 

--[[
云平台连接
@api cloudc:connect()
@usage
iotcloudc:connect()
]]
function cloudc:connect()
    self.mqttc:connect()
end

--[[
云平台断开
@api cloudc:disconnect()
@usage
iotcloudc:disconnect()
]]
function cloudc:disconnect()
    self.mqttc:disconnect()
end

--[[
云平台订阅
@api cloudc:subscribe(topic, qos)
@string/table 主题
@number topic为string时生效 0/1/2 默认0
]]
function cloudc:subscribe(topic, qos)
    self.mqttc:subscribe(topic, qos)
end

--[[
云平台取消订阅
@api cloudc:unsubscribe(topic)
@string/table 主题
]]
function cloudc:unsubscribe(topic)
    self.mqttc:unsubscribe(topic)
end

--[[
云平台发布
@api cloudc:publish(topic,data,qos,retain)
@string/table 主题
@string 消息,必填,但长度可以是0
@number 消息级别 0/1 默认0
@number 是否存档, 0/1,默认0
]]
function cloudc:publish(topic,data,qos,retain)
    self.mqttc:publish(topic,data,qos,retain)
end

--[[
云平台关闭
@api cloudc:close()
@usage
iotcloudc:close()
]]
function cloudc:close()
    self.mqttc:close()
    for k, v in pairs(cloudc_table) do
        if v.mqttc == self.mqttc then
            table.remove(cloudc_table,k)
        end
    end
end

return iotcloud
