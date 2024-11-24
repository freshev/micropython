--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "web_audio"
VERSION = "1.0.0"

--[[
    如果没有外挂spi flash，就选用后缀是不带外挂flash的固件；
    如果外挂了spi flash，就选用后缀是带外挂flash的固件。
    区别：不带外部flash的版本，因为内存空间小，删除了大部分功能，仅供测试web_audio使用
	    带外部flash的版本功能齐全，和官方发布的固件功能相同。
        固件地址：https://gitee.com/openLuat/LuatOS/attach_files/1342571/download
]]

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the mqtt library requires the following statements]]
_G.sysplus = require("sysplus")
require "audio_play"
--Modify the following parameters according to your own server
local mqtt_host = "lbsmqtt.airm2m.com"
local mqtt_port = 1883
local mqtt_isssl = false
local client_id = mcu.unique_id():toHex()
local user_name = "username"
local password = "password"

local mqttc = nil

--************************Cloud Audio MQTT Payload Message Type************************ *********************
--0---4-byte big-endian text length--GBK encoding of the text--1---4-byte big-endian audio URL length--GBK encoding of the URL
--0 means that it is followed by ordinary text. 1 means audio follows. Any combination of the two. The length is the length of the subsequent code
--************************Cloud Audio MQTT Payload Message Type************************ *********************
--************************HTTP audio download processing************************ ****************

--- Convert gb2312 encoding to utf8 encoding
--@string gb2312s gb2312 encoded data
--@return string data,utf8 encoded data
--@usage local data = common.gb2312ToUtf8(gb2312s)
function gb2312ToUtf8(gb2312s)
    local cd = iconv.open("ucs2", "gb2312")
    local ucs2s = cd:iconv(gb2312s)
    cd = iconv.open("utf8", "ucs2")
    return cd:iconv(ucs2s)
end

sys.subscribe("payload_msg", function()
    sys.taskInit(function()
        local result, data = sys.waitUntil("payload_msg")
        payload_table, payload_table_len = {}, 0
        local pbuff = zbuff.create(10240)
        local plen = pbuff:write(data)
        for i = 0, plen - 1, 1 do
            if pbuff[i] == 0 or pbuff[i] == 1 then
                if pbuff[i + 1] == 0 and pbuff[i + 2] == 0 and pbuff[i + 3] == 0 then
                    log.info("111")
                    payload_table[payload_table_len] = pbuff[i]
                    payload_table_len = payload_table_len + 1
                    payload_table[payload_table_len] = pbuff[i + 4]
                    payload_table_len = payload_table_len + 1
                    local a = pbuff[i + 4]
                    local s = pbuff:toStr(i + 5, a)
                    log.info("s", s)
                    payload_table[payload_table_len] = s
                    log.info("payload_table[payload_table_len]",
                             payload_table[payload_table_len])
                    payload_table_len = payload_table_len + 1
                end
            end
            --log.info("Test", pbuff[i], i)
        end
        log.info("payload_table_len", payload_table_len)
        for i = 0, payload_table_len, 3 do
            if i ~= 0 then sys.waitUntil("audio_end") end
            if payload_table[i] == 0 then --ttsplay
                local ttstext = payload_table[i + 2]
                log.info("音频", gb2312ToUtf8(ttstext))
                sys.publish("TTS_msg", gb2312ToUtf8(ttstext))
            elseif payload_table[i] == 1 then --Audio file playback
                local url = tostring(payload_table[i + 2])
                log.info("url", url)
                sys.publish("HTTP_msg", url)
            end

        end
    end)
end)

sys.taskInit(function()
    sys.wait(1000)
    LED = gpio.setup(27, 0, gpio.PULLUP)
    device_id = mobile.imei()
    local result= sys.waitUntil("IP_READY", 30000)
    if not result then
        log.info("网络连接失败")
    end
    mqttc = mqtt.create(nil, mqtt_host, mqtt_port, mqtt_isssl, ca_file)
    mqttc:auth(client_id, user_name, password) --client_id is required, the rest are optional
    mqttc:keepalive(30) --Default value 240s
    mqttc:autoreconn(true, 3000) --Automatic reconnection mechanism

    mqttc:on(function(mqtt_client, event, data, payload)
        --User defined code
        log.info("mqtt", "event", event, mqtt_client, data, payload)
        if event == "conack" then
            sys.publish("mqtt_conack")
            mqtt_client:subscribe("test20220929/" .. device_id)
        elseif event == "recv" then
            log.info("mqtt", "downlink", "topic", data, "payload",
                     payload:toHex())
            sys.publish("payload_msg", payload)
            -- mqttmsg(payload)
        elseif event == "sent" then
            log.info("mqtt", "sent", "pkgid", data)
        --elseif event == "disconnect" then
        ---- When there is no automatic reconnection, restart mqttc as needed
        --log.info("mqtt link disconnected")
        ---- mqtt_client:connect()
        end
    end)
    mqttc:connect()
    sys.wait(1000)
    local error = mqttc:ready()
    if not error then
        log.info("mqtt 连接失败")
    else
        log.info("mqtt 连接成功")
    end
    sys.waitUntil("mqtt_conack")
    while true do
        --mqttc automatically handles reconnection
        local ret, topic, data, qos = sys.waitUntil("mqtt_pub", 30000)
    end
    mqttc:close()
    mqttc = nil
end)

sys.taskInit(function()
    while 1 do
        --The most common HTTP GET request
        local result, data = sys.waitUntil("HTTP_msg")
        if result then
            log.info("data", data)
            local code, headers, body = http.request("GET", data, {}, "",
                                                     {dst = "/audio.mp3"})
                                            .wait()
            log.info("http", code, json.encode(headers or {}))
            if code == 200 then
                sys.wait(1000)
                log.info("fsize", fs.fsize("/audio.mp3"))
                sys.publish("Audio_msg")
            else
                log.info("http下载失败")
            end
        end
    end
end)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
