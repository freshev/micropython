local sys = require "sys"

 --Unified networking functions
sys.taskInit(function()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking, ESP32 series all support
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to automatic network distribution
        wlan.init()
        wlan.setMode(wlan.STATION) --This is also the mode by default, and you can do it without calling it.
        wlan.connect(ssid, password, 1)
    elseif mobile then
        --Air780E/Air600E series
        --mobile.simid(2) -- Automatically switch SIM cards
        --LED = gpio.setup(27, 0, gpio.PULLUP)
        --device_id = mobile.imei()
    elseif w5500 then
        --w5500 Ethernet, currently only supported by Air105
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
        --LED = gpio.setup(62, 0, gpio.PULLUP)
    elseif socket or mqtt then
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
    sys.publish("net_ready")
end)
