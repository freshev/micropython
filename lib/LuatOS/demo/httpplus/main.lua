
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "httpdemo"
VERSION = "1.0.0"

--[[
本demo需要socket库, 大部分能联网的设备都具有这个库
socket是内置库, 无需require
]]

--sys library is standard
_G.sys = require("sys")
httpplus = require "httpplus"


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


sys.taskInit(function()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to esptouch distribution network
        --LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        wlan.setMode(wlan.STATION)
        wlan.connect(ssid, password, 1)
        local result, data = sys.waitUntil("IP_READY", 30000)
        log.info("wlan", "IP_READY", result, data)
        device_id = wlan.getMac()
    elseif rtos.bsp() == "AIR105" then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
        sys.wait(1000)
        --TODO Get mac address as device_id
    elseif mobile then
        --Air780E/Air600E series
        --mobile.simid(2)
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
        log.info("ipv6", mobile.ipv6(true))
        sys.waitUntil("IP_READY", 30000)
    elseif http then
        sys.waitUntil("IP_READY")
    else
        while 1 do
            sys.wait(1000)
            log.info("http", "当前固件未包含http库")
        end
    end
    log.info("已联网")
    sys.publish("net_ready")
end)

function test_httpplus()
    sys.waitUntil("net_ready")
    --Debug switch
    httpplus.debug = true

    -- socket.sslLog(3)
    --local code, resp =httpplus.request({method="POST", url="https://abc:qq@whoami.k8s.air32.cn/goupupup"})
    --log.info("http", code, resp)

    --Expected return 302
    --local code, resp = httpplus.request({method="POST", url="https://air32.cn/goupupup"})
    --log.info("http", code, resp)

    --local code, resp = httpplus.request({method="POST", url="https://httpbin.air32.cn/post", files={abcd="/luadb/libfastlz.a"}})
    --log.info("http", code, resp)

    --local code, resp = httpplus.request({method="POST", url="https://httpbin.air32.cn/anything", forms={abcd="12345"}})
    --log.info("http", code, resp)

    --local code, resp = httpplus.request({method="POST", url="https://httpbin.air32.cn/post", files={abcd="/luadb/abc.txt"}})
    --log.info("http", code, resp)

    --Simple GET request
    local code, resp = httpplus.request({url="https://httpbin.air32.cn/"})
    log.info("http", code, resp)
    
    --Simple POST request
    --local code, resp = httpplus.request({url="https://httpbin.air32.cn/post", body="123456", method="POST"})
    --log.info("http", code, resp)
    
    --File upload
    --local code, resp = httpplus.request({url="https://httpbin.air32.cn/post", files={myfile="/luadb/abc.txt"}})
    --log.info("http", code, resp)
    
    --GET request with custom header
    --local code, resp = httpplus.request({url="https://httpbin.air32.cn/get", headers={Auth="12312234"}})
    --log.info("http", code, resp)
    
    --GET request with authentication information
    --local code, resp = httpplus.request({url="https://wendal:123@httpbin.air32.cn/get", headers={Auth="12312234"}})
    --log.info("http", code, resp)
    
    --PUT request
    --local code, resp = httpplus.request({url="https://httpbin.air32.cn/put", method="PUT", body="123"})
    --log.info("http", code, resp)

    --Form POST
    --local code, resp = httpplus.request({url="https://httpbin.air32.cn/post", forms={abc="123"}})
    --log.info("http", code, resp)

    --Response body chucked encoding test
    local code, resp = httpplus.request({url="https://httpbin.air32.cn/stream/1"})
    log.info("http", code, resp)
    if code == 200 then
        log.info("http", "headers", json.encode(resp.headers))
        local body = resp.body:query()
        log.info("http", "body", body:toHex())
        log.info("http", "body", body)
        log.info("http", "body", json.decode(body:trim()))
        --log.info("http", "body", json.decode(resp.body))
    end
end

sys.taskInit(test_httpplus)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
