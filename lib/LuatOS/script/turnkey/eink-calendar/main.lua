PROJECT = "wifidemo"
VERSION = "1.0.0"

--Test support hardware: ESP32C3
--Test firmware version: LuatOS-SoC_V0003_ESP32C3[_USB].soc

local sys = require "sys"
require("sysplus")


--Compatible with V1001 firmware
if http == nil and http2 then
    http = http2
end

--Things that need to be filled in by yourself
--wifi information
local wifiName,wifiPassword = "Xiaomi_AX6000","Air123456"
--For region ID, please go to https://api.luatos.org/luatos-calendar/v1/check-city/ to check the ID of your location
local location = "101020100"
--For weather interface information, you need to apply for it yourself. For specific parameters, please refer to the description on the page https://api.luatos.org/
local appid,appsecret = "27548549","3wdKWuRZ"

local function connectWifi()
    log.info("wlan", "wlan_init:", wlan.init())

    wlan.setMode(wlan.STATION)
    wlan.connect(wifiName,wifiPassword,1)

    --Waiting to connect to the router, the IP has not been obtained yet
    result, _ = sys.waitUntil("WLAN_STA_CONNECTED")
    log.info("wlan", "WLAN_STA_CONNECTED", result)
    --When you successfully obtain the IP, it means you are connected to the LAN.
    result, data = sys.waitUntil("IP_READY")
    log.info("wlan", "IP_READY", result, data)
end

local function requestHttp()
    local code, headers, body = http.request("GET","http://apicn.luatos.org:23328/luatos-calendar/v1?mac=111&battery=10&location="..location.."&appid="..appid.."&appsecret="..appsecret).wait()
    if code == 200 then
        return body
    else
        log.info("http get failed",code, headers, body)
        sys.wait(500)
        return ""
    end
end

function refresh()
    log.info("refresh","start!")
    local data
    for i=1,5 do--Retry up to five times
        collectgarbage("collect")
        data = requestHttp()
        collectgarbage("collect")
        if #data > 100 then
            break
        end
        log.info("load fail","retry!")
    end
    if #data < 100 then
        log.info("load fail","exit!")
        return
    end
    collectgarbage("collect")
    eink.model(eink.MODEL_1in54)
    log.info("eink.setup",eink.setup(0, 2,11,10,6,7))
    eink.setWin(200, 200, 2)
    eink.clear(1)
    log.info("eink", "end setup")
    eink.drawXbm(0, 0, 200, 200, data)
    --Refresh the screen
    eink.show()
    eink.sleep()
    log.info("refresh","done")
end


sys.taskInit(function()
    --Connect to wifi first
    connectWifi()
    while true do
        refresh()
        sys.wait(3600*1000)--Refresh it once an hour
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
