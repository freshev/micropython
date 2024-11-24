--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "wifidemo"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
require("sysplus")

--[[
本demo演示AP的配网实例
1. 启动后, 会创建一个 luatos_ + mac地址的热点
2. 热点密码是 12341234
3. 热点网关是 192.168.4.1, 同时也是配网网页的ip
4. http://192.168.4.1
]]


if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--Initialize the LED lights. The two left and right LEDs on the development board are gpio12/gpio13 respectively.
local LEDA= gpio.setup(12, 0, gpio.PULLUP)
--local LEDB= gpio.setup(13, 0, gpio.PULLUP)

local scan_result = {}

sys.taskInit(function()
    sys.wait(1000)
    wlan.init()
    sys.wait(300)
    --AP ssid and password
    wlan.createAP("luatos_" .. wlan.getMac(), "12341234")
    log.info("AP", "luatos_" .. wlan.getMac(), "12341234")
    sys.wait(500)
    wlan.scan()
    -- sys.wait(500)
    httpsrv.start(80, function(fd, method, uri, headers, body)
        log.info("httpsrv", method, uri, json.encode(headers), body)
        --/led is the API for controlling lights
        if uri == "/led/1" then
            LEDA(1)
            return 200, {}, "ok"
        elseif uri == "/led/0" then
            LEDA(0)
            return 200, {}, "ok"
        --Scan AP
        elseif uri == "/scan/go" then
            wlan.scan()
            return 200, {}, "ok"
        --The front end obtains the AP list
        elseif uri == "/scan/list" then
            return 200, {["Content-Type"]="applaction/json"}, (json.encode({data=_G.scan_result, ok=true}))
        --The front end has filled in the ssid and password, let’s connect.
        elseif uri == "/connect" then
            if method == "POST" and body and #body > 2 then
                local jdata = json.decode(body)
                if jdata and jdata.ssid then
                    --Enable a timer to connect to the Internet, otherwise this situation may not be completed until the network is completed.
                    sys.timerStart(wlan.connect, 500, jdata.ssid, jdata.passwd)
                    return 200, {}, "ok"
                end
            end
            return 400, {}, "ok"
        --Determine whether the connection is successful based on the IP address
        elseif uri == "/connok" then
            return 200, {["Content-Type"]="applaction/json"}, json.encode({ip=socket.localIP()})
        end
        --In other cases, I just can’t find it.
        return 404, {}, "Not Found" .. uri
    end)
    log.info("web", "pls open url http://192.168.4.1/")
end)

--After the wifi scan is successful, there will be a WLAN_SCAN_DONE message, just read it
sys.subscribe("WLAN_SCAN_DONE", function ()
    local result = wlan.scanResult()
    _G.scan_result = {}
    for k,v in pairs(result) do
        log.info("scan", (v["ssid"] and #v["ssid"] > 0) and v["ssid"] or "[隐藏SSID]", v["rssi"], (v["bssid"]:toHex()))
        if v["ssid"] and #v["ssid"] > 0 then
            table.insert(_G.scan_result, v["ssid"])
        end
    end
    log.info("scan", "aplist", json.encode(_G.scan_result))
end)

sys.subscribe("IP_READY", function()
    --After successful networking, the simulation is reported to the server
    log.info("wlan", "已联网", "通知服务器")
    sys.taskInit(function()
        sys.wait(1000)
        --The following is the simulation implementation of the rtkv library. It is not mandatory to Introduction rtkv here.
        local token = mcu.unique_id():toHex()
        local device = wlan.getMac()
        local params = "device=" .. device .. "&token=" .. token
        params = params .. "&key=ip&value=" .. (socket.localIP())
        local code = http.request("GET", "http://rtkv.air32.cn/api/rtkv/set?" .. params, {timeout=3000}).wait()
        log.info("上报结果", code)
    end)
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
