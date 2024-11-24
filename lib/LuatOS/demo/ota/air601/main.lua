--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "otademo"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
require("sysplus")

--[[
提示:
1. 本demo是演示ota的, 只升级脚本, 如果还需要底层一起升级, 参考demo/fota
2. demo/fota 需要大量flash空间作为fota分区, 所以能启用的库会很少,请酌情使用
3. ota文件是放在文件系统的,所以不能超过40k, 且不能少于1k
4. 服务器上的ota文件路径无所谓, 本地下载路径必须是 /update.bin
]]

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

sys.timerLoopStart(function()
    log.info("当前版本号", _G.VERSION)
end, 1000)

--OTA tasks
function ota_task()
    sys.taskInit(function()
        local dst_path = "/update.bin"
        os.remove(dst_path) --Be sure to remove old files first
        --Here is the URL used for demonstration. Please replace it with your own in actual projects.
        --Path rules are customized and do not necessarily require this specification.
        --The current version is used as the path here to facilitate demonstration and avoid repeated upgrades.
        local url = "http://upload.air32.cn/ota/air601/" .. _G.PROJECT .. "/" .. _G.VERSION .. ".ota?mac=" .. wlan.getMac()
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
    wlan.connect("luatos1234", "12341234")
    log.info("wlan", "wait for IP_READY", wlan.getMac())
    sys.waitUntil("IP_READY", 30000)

    sys.wait(500)
    --After connecting to the Internet, execute an OTA first
    ota_task()
    --Then perform OTA every 6 hours
    sys.timerLoopStart(ota_task, 6*3600*1000)
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
