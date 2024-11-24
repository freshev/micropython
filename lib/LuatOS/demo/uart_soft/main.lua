--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "soft_uart"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

log.info("main", "soft uart demo")

local function resouce()     
    local rtos_bsp = rtos.bsp()
    if rtos_bsp == "AIR101" then
        return nil,nil,nil,nil,nil,nil,nil
    elseif rtos_bsp == "AIR103" then
        return nil,nil,nil,nil,nil,nil,nil
    elseif rtos_bsp == "AIR105" then
        return pin.PA07,0,pin.PA06,1,115200,-20,-10
    elseif rtos_bsp == "ESP32C3" then
        return nil,nil,nil,nil,nil,nil,nil
    elseif rtos_bsp == "ESP32S3" then
        return nil,nil,nil,nil,nil,nil,nil
    elseif rtos_bsp == "EC618" then
        return 17,0,1,2,19200,0,-1
	elseif string.find(rtos_bsp,"EC718")then
        return 2,0,3,2,9600,0,0
    else
        log.info("main", "bsp not support")
        return
    end
end

local tx_pin,tx_timer,rx_pin,rx_timer,br,tx_adjust,rx_adjust = resouce() 

local uartid = uart.createSoft(tx_pin,tx_timer,rx_pin,rx_timer,tx_adjust,rx_adjust)
--initialization
local result = uart.setup(
    uartid,--serial port id
    br,--The software serial port baud rate has different limits according to the software and hardware configuration of the platform.
    8,--data bits
    1,--Stop bit
    uart.ODD
)


--Send data cyclically
--sys.timerLoopStart(uart.write,1000, uartid, "test")
--Receiving data will trigger a callback, where "receive" is a fixed value
uart.on(uartid, "receive", function(id, len)
    local s = ""
    repeat
        --s = uart.read(id, 1024)
        s = uart.read(id, len)
        if #s > 0 then --#s is to take the length of the string
            --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
            --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
            log.info("uart", "receive", id, #s, s, s:toHex())
            uart.write(id, s)
        end
    until s == ""
end)

--Not all devices support the sent event
uart.on(uartid, "sent", function(id)
    log.info("uart", "sent", id)
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
