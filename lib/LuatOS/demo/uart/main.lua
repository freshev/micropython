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

local uartid = 1 --Select different uartid according to the actual device

--initialization
uart.setup(
    uartid,--serial port id
    115200,--baud rate
    8,--data bits
    1--Stop bit
)


--Send data cyclically
sys.timerLoopStart(uart.write,1000, uartid, "test")
--Receiving data will trigger a callback, where "receive" is a fixed value
uart.on(uartid, "receive", function(id, len)
    local s = ""
    repeat
        s = uart.read(id, 128)
        if #s > 0 then --#s is to take the length of the string
            --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
            --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
            log.info("uart", "receive", id, #s, s)
            --log.info("uart", "receive", id, #s, s:toHex())
        end
        --If you use ESP32C3/ESP32S3 firmware compiled before 2024.5.13, restoring the following code can work normally
        --if #s == len then
        --     break
        -- end
    until s == ""
end)

--Not all devices support the sent event
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
