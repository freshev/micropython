
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "libgnssdemo"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

--[[ 
demo适用于air530z, 演示挂载在uart 2的情况, 如果挂载在其他端口, 修改gps_uart_id
]]

local gps_uart_id = 2

uart.on(gps_uart_id, "recv", function(id, len)
    local data = uart.read(gps_uart_id, 1024)
     if data then
        libgnss.parse(data)
    end
end)

--The default baud rate of Air530Z is 9600, and it is automatically switched once
sys.taskInit(function()
    uart.setup(gps_uart_id, 9600)
    uart.write(gps_uart_id, "$PCAS01,5*19\r\n")
    sys.wait(200)
    uart.close(gps_uart_id)
    uart.setup(gps_uart_id, 115200)
end)

sys.timerLoopStart(function()
    log.info("GPS", libgnss.getIntLocation())
    local rmc = libgnss.getRmc()
    log.info("rmc", json.encode(rmc))
    --log.info("rmc", rmc.lat, rmc.lng, rmc.year, rmc.month, rmc.day, rmc.hour, rmc.min, rmc.sec)
    rtc.set({year=rmc.year,mon=rmc.month,day=rmc.day,hour=rmc.hour,min=rmc.min,sec=rmc.sec})
end, 3000) --Print every two seconds

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!