--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "usb_uart"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

log.info("main", "usb uart demo")

--The virtual serial port of Air105 requires driver installation, but Air780E/Air600E does not.
--USB driver download https://doc.openluat.com/wiki/21?wiki_page_id=2070
--The USB driver is consistent with the USB driver of Hezhou Cat.1

--Currently only Air105/Air780E/Air600E can run this demo

if usbapp then --Air105 needs to initialize the usb virtual serial port
    usbapp.start(0)
end


local uartid = uart.VUART_0 --Fixed id of USB virtual serial port

--initialization
local result = uart.setup(
    uartid,--serial port id
    115200,--baud rate
    8,--data bits
    1--Stop bit
)


--Receiving data will trigger a callback, where "receive" is a fixed value
uart.on(uartid, "receive", function(id, len)
    local s = ""
    repeat
        --s = uart.read(id, 1024)
        s = uart.read(id, len)
        if s and #s > 0 then --#s is to take the length of the string
            --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
            --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
            log.info("uart", "receive", id, #s, s)
            --log.info("uart", "receive", id, #s, s:toHex())
        end
    until s == ""
end)

--Not all devices support the sent event
uart.on(uartid, "sent", function(id)
    log.info("uart", "sent", id)
end)

sys.taskInit(function()
    local timer
    while 1 do
        local result, uart_id,data = sys.waitUntil("USB_UART_INC", 30000)
        if result and uart_id == uart.VUART_0 and data == 1 then
            --Send data cyclically
            timer = sys.timerLoopStart(uart.write,1000, uartid, "test")
            while 1 do
                local result, uart_id,data = sys.waitUntil("USB_UART_INC", 30000)
                if result and uart_id == uart.VUART_0 and data == 2 then
                    sys.timerStop(timer)
                    break
                end
            end
        end
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
