
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "sntpdemo"
VERSION = "1.0.0"

--[[
本demo需要mqtt库, 大部分能联网的设备都具有这个库
mqtt也是内置库, 无需require
]]

--sys library is standard
_G.sys = require("sys")
--[[Special note, using the mqtt library requires the following statements]]
_G.sysplus = require("sysplus")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--Unified networking functions
sys.taskInit(function()
    local device_id = mcu.unique_id():toHex()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to automatic network distribution
        --LED = gpio.setup(12, 0, gpio.PULLUP)
        wlan.init()
        wlan.setMode(wlan.STATION) --This is also the mode by default, and you can do it without calling it.
        device_id = wlan.getMac()
        wlan.connect(ssid, password, 1)
    elseif mobile then
        --Air780E/Air600E series
        --mobile.simid(2) -- Automatically switch SIM cards
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        device_id = mobile.imei()
    elseif w5500 then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
    elseif socket then
        --The adapted socket library is also OK
        --No other operations, just give a comment
    else
        --For other bsps that I don’t know, let me give you some hints in a loop.
        while 1 do
            sys.wait(1000)
            log.info("bsp", "本bsp可能未适配网络层, 请查证")
        end
    end
    --By default, it waits until the connection is successful
    sys.waitUntil("IP_READY")
    sys.publish("net_ready", device_id)
end)

sys.taskInit(function()
    --Waiting for Internet connection
    local ret, device_id = sys.waitUntil("net_ready")
    sys.wait(1000)
    --For Cat.1 Moduless and mobile/telecom cards, the base station time is usually issued, so sntp is not necessary. However, China Unicom cards are usually not issued, so sntp is required.
    --Corresponding to ESP32 series Moduless, the firmware will also execute sntp by default, so manually calling sntp is also optional.
    --sntp has built-in several commonly used NTP servers, and also supports self-selected servers.
    while 1 do
        --Use the built-in ntp server address, including Ali ntp
        log.info("开始执行SNTP")
        socket.sntp()
        --Custom ntp address
        -- socket.sntp("ntp.aliyun.com")
        --socket.sntp({"baidu.com", "abc.com", "ntp.air32.cn"})
        --It usually only takes a few hundred milliseconds to succeed
        local ret = sys.waitUntil("NTP_UPDATE", 5000)
        if ret then
            --The following is a demonstration of getting/printing time, pay attention to time zone issues
            log.info("sntp", "时间同步成功", "本地时间", os.date())
            log.info("sntp", "时间同步成功", "UTC时间", os.date("!%c"))
            log.info("sntp", "时间同步成功", "RTC时钟(UTC时间)", json.encode(rtc.get()))
            --os.time(rtc.get()) requires a version after 2023.07.21, because the naming difference of the month is mon/month
            --log.info("sntp", "Time synchronization successful", "utc timestamp", os.time(rtc.get()))
            log.info("sntp", "时间同步成功", "本地时间戳", os.time())
            local t = os.date("*t")
            log.info("sntp", "时间同步成功", "本地时间os.date() json格式", json.encode(t))
            log.info("sntp", "时间同步成功", "本地时间os.date(os.time())", os.time(t))
            --log.info("sntp", "Time synchronization successful", "Local time", os.time())
            --For normal use, once an hour is enough, or even once a day.
            -- sys.wait(3600000) 
            --For demonstration purposes here, every 5 seconds is used.
            sys.wait(5000)
        else
            log.info("sntp", "时间同步失败")
            sys.wait(60000) --Try again in 1 minute
        end

        --Timestamp, accurate to milliseconds. Added on 2023.11.15
        --Note that the timestamp will be more accurate if sntp has been successfully completed at least 2 times.
        --If sntp is completed only once, the timestamp will be slower than the standard time by the length of network delay (10~500ms).
        if socket.ntptm then
            local tm = socket.ntptm()
            log.info("tm数据", json.encode(tm))
            log.info("时间戳", string.format("%u.%03d", tm.tsec, tm.tms))
            sys.wait(5000)
        end
    end
end)

sys.subscribe("NTP_ERROR", function()
    log.info("socket", "sntp error")
    -- socket.sntp()
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
