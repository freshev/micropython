--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "wifidemo"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
require("sysplus")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--OTA tasks
function ota_task()
    sys.taskInit(function()
        local dst_path = "/update.bin"
        os.remove(dst_path)
        local url = "http://site0.cn/api/esp32/ota?mac=" .. wlan.getMac()
        local code = http.request("GET", url, nil, nil, {dst=dst_path}).wait()
        if code and code == 200 then
            log.info("ota", "OTA 下载完成, 3秒后重启")
            sys.wait(3000)
            rtos.reboot()
        end
        log.info("ota", "服务器返回非200,就是不需要升级", code)
        os.remove(dst_path)
    end)
end

sys.taskInit(function()
    sys.wait(100)
    wlan.init()
    sys.wait(100)
    wlan.connect("luatos1234", "123456890")
    log.info("wlan", "wait for IP_READY", wlan.getMac())
    sys.waitUntil("IP_READY", 30000)

    --After connecting to the Internet, execute an OTA first
    ota_task()
    --Then perform OTA every 6 hours
    sys.timerLoopStart(ota_task, 6*3600*1000)
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
