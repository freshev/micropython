
--libgnss library initialization
libgnss.clear() --Clear data and initialize

--LED and ADC initialization
LED_GNSS = 24
gpio.setup(LED_GNSS, 0) --GNSS positioning success light

local gnss = require("uc6228")

sys.taskInit(function()
    log.debug("提醒", "室内无GNSS信号,定位不会成功, 要到空旷的室外,起码要看得到天空")
    gnss.setup({
        uart_id=2,
        uart_forward = uart.VUART_0, --Forward to the virtual serial port to facilitate docking with GnssToolKit3
        debug=true
    })
    pm.power(pm.GPS, true)
    gnss.start()
    gnss.agps()
end)

sys.taskInit(function()
    while 1 do
        sys.wait(5000)
        --log.info("RMC", json.encode(libgnss.getRmc(2) or {}, "7f"))
        --log.info("INT", libgnss.getIntLocation())
        --log.info("GGA", libgnss.getGga(3))
        --log.info("GLL", json.encode(libgnss.getGll(2) or {}, "7f"))
        --log.info("GSA", json.encode(libgnss.getGsa(1) or {}, "7f"))
        --log.info("GSV", json.encode(libgnss.getGsv(2) or {}, "7f"))
        --log.info("VTG", json.encode(libgnss.getVtg(2) or {}, "7f"))
        --log.info("ZDA", json.encode(libgnss.getZda(2) or {}, "7f"))
        --log.info("date", os.date())
        --log.info("sys", rtos.meminfo("sys"))
        --log.info("lua", rtos.meminfo("lua"))
    end
end)

--Subscribe to GNSS status encoding
sys.subscribe("GNSS_STATE", function(event, ticks)
    --The event value is
    --FIXED Positioning successful
    --LOSE positioning lost
    --Ticks is the time when the event occurs and can generally be ignored.
    local onoff = libgnss.isFix() and 1 or 0
    log.info("GNSS", "LED", onoff)
    gpio.set(LED_GNSS, onoff)
end)

