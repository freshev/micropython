--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "wifidemo"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
require("sysplus")


--function meminfo()
--log.info("lua", rtos.meminfo())
--log.info("sys", rtos.meminfo("sys"))
-- end

--Initialize the LED lights. The two left and right LEDs on the development board are gpio12/gpio13 respectively.
local LEDA= gpio.setup(12, 0, gpio.PULLUP)
local LEDB= gpio.setup(13, 0, gpio.PULLUP)

sys.taskInit(function()
    sys.wait(1000)
    wlan.init()
    --Change it to your own ssid and password
    --wlan.connect("myssid", "mypassword", 1)
    wlan.connect("luatos1234", "12341234", 1)
    log.info("wlan", "wait for IP_READY")
    
    while not wlan.ready() do
        local ret, ip = sys.waitUntil("IP_READY", 30000)
        --After wlan is connected, the ip address will be printed here.
        log.info("ip", ret, ip)
        if ip then
            _G.wlan_ip = ip
        end
    end
    log.info("wlan", "ready !!", wlan.getMac())
    sys.wait(1000)
    httpsrv.start(80, function(fd, method, uri, headers, body)
        log.info("httpsrv", method, uri, json.encode(headers), body)
        -- meminfo()
        if uri == "/led/1" then
            LEDA(1)
            return 200, {}, "ok"
        elseif uri == "/led/0" then
            LEDA(0)
            return 200, {}, "ok"
        end
        return 404, {}, "Not Found" .. uri
    end)
    log.info("web", "pls open url http://" .. _G.wlan_ip .. "/")
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
