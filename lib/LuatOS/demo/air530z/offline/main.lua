
_G.sys = require("sys")
require "sysplus"
--[[
接Air530Z-BD, 本demo演示的是不带AGPS的, 带AGPS的请参考demo/air530z/agps (还没写完)

提醒:
1. Air530Z-BD的串口默认是9600波特率, 单北斗
2. Air530Z 是北斗+GPS双模, 默认是9600波特率
3. UART的TX/RX要交叉接, 否则无法正常工作
]]

local gps_uart_id = 2

sys.taskInit(function()
    libgnss.clear() --Clear data and initialize
    sys.wait(100)
    --Try 9600 baud rate first, and switch the baud rate
    uart.setup(gps_uart_id, 9600)
    uart.write(gps_uart_id, "$PCAS01,5*19\r\n")
    sys.wait(200)

    --Press 115200 to start reading data
    uart.setup(gps_uart_id, 115200)
    --Debug log, optional
    -- libgnss.debug(true)
    libgnss.bind(gps_uart_id)
end)

sys.taskInit(function()
    while 1 do
        sys.wait(1000)
        log.info("RMC", json.encode(libgnss.getRmc(2) or {}, "7f"))
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

        --Print all satellites
        --local gsv = libgnss.getGsv() or {sats={}}
        --for i, v in ipairs(gsv.sats) do
        --log.info("sat", i, v.nr, v.snr, v.azimuth, v.elevation)
        -- end
    end
end)

sys.run()
