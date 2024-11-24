
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "httpdemo"
VERSION = "1.0.0"

--[[
本demo需要http库, 大部分能联网的设备都具有这个库
http也是内置库, 无需require
]]

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the http library requires the following statements]]
_G.sysplus = require("sysplus")

sys.taskInit(function()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "uiot"
        local password = "12345678"
        log.info("wifi", ssid, password)
        --TODO Change to esptouch distribution network
        LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        wlan.setMode(wlan.STATION)
        wlan.connect(ssid, password, 1)
        local result, data = sys.waitUntil("IP_READY", 30000)
        log.info("wlan", "IP_READY", result, data)
        device_id = wlan.getMac()
    elseif mobile and mobile.imei then
        --Air780E/Air600E series
        --mobile.simid(2)
        LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
        log.info("ipv6", mobile.ipv6(true))
        sys.waitUntil("IP_READY", 30000)
    elseif w5500 then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        LED = gpio.setup(62, 0, gpio.PULLUP)
        sys.wait(1000)
        --TODO Get mac address as device_id
    end

    --Print the supported cipher suites. Generally speaking, the firmware already contains 99% of the common cipher suites.
    if crypto.cipher_suites then
        log.info("cipher", "suites", json.encode(crypto.cipher_suites()))
    end

    -------------------------------------
    -------- HTTP demo code ---------------
    -------------------------------------

    

    while 1 do
        --The most common HTTP GET request
        local code, headers, body = http.request("GET", "https://www.air32.cn/").wait()
        log.info("http.get", "air32.cn", code)
        sys.wait(100)

        --ipv6 test, only supported by EC618 series
        local code, headers, body = http.request("GET", "https://mirrors6.tuna.tsinghua.edu.cn/", nil, nil, {ipv6=true}).wait()
        log.info("http.get", "ipv6", code, json.encode(headers or {}), body and #body or 0)
        sys.wait(100)

        --A rather special external URL to obtain earthquake information
        local url ="https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_hour.geojson"
        local code, headers, body = http.request("GET", url).wait()
        log.info("http.get", "earthquakes", code, json.encode(headers or {}), body and #body or 0)
        sys.wait(100)

        --Extra long URL test
        local url ="https://www.baidu.com/?fdasfaisdolfjadklsjfklasdjflka=fdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflkafdasfaisdolfjadklsjfklasdjflka"
        local code, headers, body = http.request("GET", url).wait()
        log.info("http.get", "longurl", code, json.encode(headers or {}), body and #body or 0)
        sys.wait(100)

        --Alibaba Cloud automatically registers the device verification, which is different from 200
        local url ="https://iot-auth.cn-shanghai.aliyuncs.com/auth/register/device"
        local req_headers = {}
        req_headers["Content-Type"] = "application/x-www-form-urlencoded"
        local req_body = "productKey=he1iZrw123&deviceName=861551056136351&random=2717&sign=DD31BA6E9E087A6DD88E96FD47A7AAA3&signMethod=HmacMD5"
        --req_headers["Content-Length"] = tostring(#req_body)
        local code, headers, body = http.request("POST", url, req_headers, req_body).wait()
        log.info("http.get", "aliyun", code, json.encode(headers or {}), body and #body or 0)
        sys.wait(100)

        --Content-Length:0 case
        local code, headers, body = http.request("GET", "http://air32.cn/test/zero.txt").wait()
        log.info("http.get", "emtry content", code, json.encode(headers or {}), body and #body or 0)
        sys.wait(100)

        sys.wait(600000)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
