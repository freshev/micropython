--[[
w5500集成演示
]]

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "w5500demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")
sysplus = require("sysplus")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--Networking function
sys.taskInit(function()
    sys.wait(100)
    if w5500 == nil then
        while 1 do
            log.info("w5500", "当前固件未包含w5500库")
            sys.wait(1000)
        end
    end
    -----------------------------
    --w5500 Ethernet
    ----------------------------
    --Wiring tips:
    --First find the SPI port, SCK clock, MISO, MOSI and connect them
    --The power supply must be sufficient, especially since the w5500 Modules is powered by 3.3v, it is best to have an external power supply.
    --The following default selected GPIOs are not mandatory and can be replaced with other GPIOs.
    local rtos_bsp = rtos.bsp()
    log.info("setup w5500 for", rtos_bsp)
    if rtos_bsp:startsWith("ESP32") then
        --ESP32C3, GPIO5 is connected to SCS, GPIO6 is connected to IRQ/INT, GPIO8 is connected to RST
        w5500.init(2, 20000000, 5, 6, 8) 
    elseif rtos_bsp:startsWith("EC618") then
        --EC618 series, such as Air780E/Air600E/Air700E
        --GPIO8 is connected to SCS, GPIO1 is connected to IRQ/INT, GPIO22 is connected to RST
        w5500.init(0, 25600000, 8, 1, 22) 
    elseif rtos_bsp:startsWith("EC718") then
        --EC718P series, such as Air780EP/Air780EPV
        --GPIO8 is connected to SCS, GPIO29 is connected to IRQ/INT, GPIO30 is connected to RST
        w5500.init(0, 25600000, 8, 29, 30)
    elseif rtos_bsp == "AIR101" or rtos_bsp == "AIR103" or rtos_bsp == "AIR601" then
        --PA1 is connected to SCS, PB01 is connected to IRQ/INT, PA7 is connected to RST
        w5500.init(0, 20000000, pin.PA01, pin.PB01, pin.PA07)
    elseif rtos_bsp == "AIR105" then
        --PC14 is connected to SCS, PC01 is connected to IRQ/INT, PC00 is connected to RST
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
    else
        --For other bsps that I don’t know, let me give you some hints in a loop.
        while 1 do
            sys.wait(1000)
            log.info("bsp", "本bsp可能未适配w5500, 请查证")
        end
    end
    w5500.config() --The default is DHCP mode
    --w5500.config("192.168.1.29", "255.255.255.0", "192.168.1.1") --Static IP mode
    --w5500.config("192.168.1.122", "255.255.255.0", "192.168.1.1", string.fromHex("102a3b4c5d6e"))
    w5500.bind(socket.ETH0)
    --Tips: You need to connect to the Internet cable, otherwise there may not be any logs printed.
    --By default, it waits until the connection is successful
    sys.waitUntil("IP_READY")
    sys.publish("net_ready")
end)

--Demo task
local function sockettest()
    --Waiting for Internet connection
    sys.waitUntil("net_ready")

    socket.sntp(nil, socket.ETH0)
    sys.wait(500)

    --Pay attention to the adapter parameter here. The bsp where you are located may have multiple adapters. For example, Air780E itself also has a 4G network adapter.
    --So here you need to specify which network adapter to use to access
    --In the same way, the socket/mqtt/websocket/ftp library has similar configuration items.
    local opts = {}
    opts["adapter"] = socket.ETH0
    while 1 do
        log.info("发起http请求")
        local code, headers, body = http.request("GET", "http://httpbin.air32.cn/get", nil, nil, opts).wait()
        log.info("http", code, body)
        sys.wait(5000)
    end
end

sys.taskInit(sockettest)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!

