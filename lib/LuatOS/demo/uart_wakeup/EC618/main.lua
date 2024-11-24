--UART1 supports 600 1200 2400 4800 9600 baud rate and still receives data in sleep mode without losing data.
--At other baud rates, wake up through the RX of UART1 after sleeping. Note that all continuous data will be lost when waking up, so it needs to be sent twice. After the first byte is sent, there will be a prompt, and then the data is sent.
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "uart_wakeup"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")


local uartid = 1 --Select different uartid according to the actual device

--initialization
local result = uart.setup(
    uartid,--serial port id
    115200,--baud rate
    8,--data bits
    1--Stop bit
)


--Send data cyclically
--Receiving data will trigger a callback, where "receive" is a fixed value
uart.on(uartid, "receive", function(id, len)
    if len == -1 then
        pm.force(pm.IDLE)
        sys.timerStart(function()
            pm.force(pm.LIGHT)
            uart.write(uartid, "now sleep\r\n")
        end, 5000)
        uart.write(uartid, "uart rx wakeup, after 5 second sleep again\r\n")
        return
    end
    local s = ""
    repeat
        --s = uart.read(id, 1024)
        s = uart.read(id, len)
        if #s > 0 then --#s is to take the length of the string
            --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
            --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
            log.info("uart", "receive", id, #s, s)
            --log.info("uart", "receive", id, #s, s:toHex())
            uart.write(id, s)
        end
    until s == ""
end)

--Not all devices support the sent event
uart.on(uartid, "sent", function(id)
    log.info("uart", "sent", id)
end)

sys.taskInit(function()
    pm.force(pm.LIGHT)
    while 1 do
        sys.wait(20000)
        uart.write(uartid, "20sec, wakeup\r\n")
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
