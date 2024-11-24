--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "uart_irq"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

log.info("main", "uart demo")

local uart_id = 1

local uartid = 2 --Select different uartid according to the actual device
local _uart485Pin = 18
gpio.setup(_uart485Pin, 0, nil, nil, 4) 
----gpio.setup(18, 0, nil, nil, 4)
--initialization
--485 automatic switching, select GPIO10 as the transceiver conversion pin

uart.setup(uart_id, 9600, 8, 1, uart.NONE)
uart.setup(uartid, 9600, 8, 1, uart.NONE, uart.LSB, 1024, _uart485Pin, 0, 2000)
--uart.setup(uartid, 9600, 8, 1, uart.NONE)
  
--Send data cyclically
sys.timerLoopStart(uart.write,1000, uart_id, "test")
sys.timerLoopStart(uart.write,1000, uartid, "test")
--Receiving data will trigger a callback, where "receive" is a fixed value



sys.taskInit(function ()
    uart.on(uart_id, "receive", function(id, len)
        --if uart_id ~= id then
        --     return
        -- end
        local s = ""
        repeat
            --s = uart.read(id, 1024)
            s = uart.read(id, len)
            if #s > 0 then --#s is to take the length of the string
                --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
                --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
                log.info("uart", "receive", id, #s, s)
                --log.info("uart", "receive", id, #s, s:toHex())
            end
            if #s == len then
                break
            end
            
        until s == ""
    end)
end)

sys.taskInit(function ()
    
    uart.on(uartid, "receive", function(id, len)
        --if uartid ~= id then
        --     return
        -- end
        local s = ""
        repeat
            --s = uart.read(id, 1024)
            s = uart.read(id, len)
            if #s > 0 then --#s is to take the length of the string
                --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
                --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
                log.info("uart", "receive", id, #s, s)
                --log.info("uart", "receive", id, #s, s:toHex())
            end
            if #s == len then
                break
            end
            
        until s == ""
    end)
end)


--Not all devices support the sent event
uart.on(uart_id, "sent", function(id)
    log.info("uart", "sent", id)
end)
uart.on(uartid, "sent", function(id)
    log.info("uart", "sent", id)
end)
-- sys.taskInit(function()
--while 1 do
--         sys.wait(500)
--     end
-- end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
