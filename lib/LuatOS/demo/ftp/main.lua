
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ftpdemo"
VERSION = "1.0.0"

--[[
本demo需要ftp库, 大部分能联网的设备都具有这个库
ftp也是内置库, 无需require
]]

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the ftp library requires the following statements]]
_G.sysplus = require("sysplus")

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
        --w5500.init(spi.SPI_2, 24000000, pin.PB03, pin.PC00, pin.PC03)
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        log.info("auto mac", w5500.getMac():toHex())
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
        sys.waitUntil("IP_READY", 30000)
    end

    ----Print the supported cipher suites. Generally speaking, the firmware already contains 99% of the common cipher suites.
    --if crypto.cipher_suites then
    --log.info("cipher", "suites", json.encode(crypto.cipher_suites()))
    -- end
    while true do
        sys.wait(1000)
        log.info("ftp 启动")
        print(ftp.login(nil,"121.43.224.154",21,"ftp_user","3QujbiMG").wait())
    
        print(ftp.command("NOOP").wait())
        print(ftp.command("SYST").wait())

        print(ftp.command("TYPE I").wait())
        print(ftp.command("PWD").wait())
        print(ftp.command("MKD QWER").wait())
        print(ftp.command("CWD /QWER").wait())

        print(ftp.command("CDUP").wait())
        print(ftp.command("RMD QWER").wait())

        print(ftp.command("LIST").wait())

        --io.writeFile("/1222.txt", "23noianfdiasfhnpqw39fhawe;fuibnnpw3fheaios;fna;osfhisao;fadsfl")
        -- print(ftp.push("/1222.txt","/12222.txt").wait())
        
        print(ftp.pull("/122224.txt","/122224.txt").wait())

        local f = io.open("/122224.txt", "r")
        if f then
            local data = f:read("*a")
            f:close()
            log.info("fs", "writed data", data)
        else
            log.info("fs", "open file for read failed")
        end

        print(ftp.command("DELE /12222.txt").wait())
        print(ftp.push("/122224.txt","/12222.txt").wait())
        print(ftp.close().wait())
        log.info("meminfo", rtos.meminfo("sys"))
        sys.wait(15000)
    end


end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
