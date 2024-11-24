
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "lbsLocdemo"
VERSION = "1.0.0"

--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.

--[[Note: The PRODUCT_KEY here is for demonstration only and is not guaranteed to be usable all the time. In mass production projects, you must use the project productKey you created in iot.openluat.com]]
PRODUCT_KEY = ""

--[[This demo requires the lbsLoc library and libnet library. The libraries are located in script\libs and require]]
local lbsLoc = require("lbsLoc")

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the lbsLoc library requires the following statements]]
_G.sysplus = require("sysplus")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


--Function: Callback function after obtaining the latitude and longitude corresponding to the base station
--Parameters: -- result: number type, 0 means success, 1 means the network environment is not ready yet, 2 means failure to connect to the server, 3 means failure to send data, 4 means the receiving server response timed out, 5 means the server failed to return the query; when it is 0, Only the next 5 parameters are meaningful
		--lat: string type, latitude, 3 digits for the integer part and 7 digits for the decimal part, for example, 031.2425864
		--lng: string type, longitude, 3 digits for the integer part and 7 digits for the decimal part, for example, 121.4736522
        --addr: currently meaningless
        --time: string type or nil, the time returned by the server, 6 bytes, year, month, day, hour, minute and second, need to be converted to hexadecimal for reading
            --The first byte: year minus 2000, for example, 2017, it is 0x11
            --The second byte: month, for example, July is 0x07, December is 0x0C
            --The third byte: day, for example, the 11th is 0x0B
            --The fourth byte: hour, for example, when 18, it is 0x12
            --The fifth byte: points, for example, 59 points is 0x3B
            --The sixth byte: seconds, for example, 48 seconds is 0x30
        --locType: numble type or nil, positioning type, 0 indicates successful base station positioning, 255 indicates successful WIFI positioning
local function getLocCb(result, lat, lng, addr, time, locType)
    log.info("testLbsLoc.getLocCb", result, lat, lng)
    --Obtaining latitude and longitude successfully
    if result == 0 then
        log.info("服务器返回的时间", time:toHex())
        log.info("定位类型,基站定位成功返回0", locType)
    end
    --Broadcast to other tasks that need positioning data
    --sys.publish("lbsloc_result", result, lat, lng)
end

sys.taskInit(function()
    sys.waitUntil("IP_READY", 30000)
    while 1 do
        mobile.reqCellInfo(15)
        sys.waitUntil("CELL_INFO_UPDATE", 3000)
        lbsLoc.request(getLocCb)
        sys.wait(60000)
    end
end)

----The following is base station + wifi hybrid positioning
--Note that the free version of base station + wifi hybrid positioning will only return base station positioning results in most cases.
--Please contact sales for paid version
--sys.subscribe("WLAN_SCAN_DONE", function ()
--local results = wlan.scanResult()
--log.info("scan", "results", #results)
--if #results > 0 then
--local reqWifi = {}
--for k,v in pairs(results) do
--log.info("scan", v["ssid"], v["rssi"], v["bssid"]:toHex())
--local bssid = v["bssid"]:toHex()
--bssid = string.format ("%s:%s:%s:%s:%s:%s", bssid:sub(1,2), bssid:sub(3,4), bssid:sub(5,6), bssid:sub(7,8), bssid:sub(9,10), bssid:sub(11,12))
--             reqWifi[bssid]=v["rssi"]
--         end
--         lbsLoc.request(getLocCb,nil,nil,nil,nil,nil,nil,reqWifi)
--     else
--lbsLoc.request(getLocCb) -- No wifi data, normal positioning
--     end
-- end)

-- sys.taskInit(function()
--sys.waitUntil("IP_READY", 30000)
--     wlan.init()
--while 1 do
--         mobile.reqCellInfo(15)
--sys.waitUntil("CELL_INFO_UPDATE", 3000)
--         wlan.scan()
--         sys.wait(60000)
--     end
-- end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
