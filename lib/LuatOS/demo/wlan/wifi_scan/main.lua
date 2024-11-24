--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "wifidemo"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
require("sysplus")

--After the wifi scan is successful, there will be a WLAN_SCAN_DONE message, just read it
sys.subscribe("WLAN_SCAN_DONE", function ()
    local results = wlan.scanResult()
    log.info("scan", "results", #results)
    for k,v in pairs(results) do
        log.info("scan", v["ssid"], v["rssi"], (v["bssid"]:toHex()))
    end
end)

sys.taskInit(function()
    sys.wait(1000)
    wlan.init()
    while 1 do
        wlan.scan()
        sys.wait(15000) --Note, especially for the Air780 series, this time can only be longer and not shorter.
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
