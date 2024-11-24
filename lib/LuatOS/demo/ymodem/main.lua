--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "uart_ymodem"
VERSION = "1.0.0"
log.style(1)
log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

local uartid = 1 --Select different uartid according to the actual device

--initialization
local result = uart.setup(
    uartid,--serial port id
    115200,--baud rate
    8,--data bits
    1--Stop bit
)
local ymodem_running = false

local rxbuff = zbuff.create(1024 + 32)
local ymodem_handler = ymodem.create("/","save.bin")
local function ymodem_to()
    if not ymodem_running then
        uart.write(uartid, "C")
        ymodem.reset(ymodem_handler)
    end
end


sys.timerLoopStart(ymodem_to,500)

local function ymodem_rx(id,len)
    uart.rx(id,rxbuff)
    log.info(rxbuff:used())
    local result,ack,flag,file_done,all_done = ymodem.receive(ymodem_handler,rxbuff)
    ymodem_running = result
    log.info(ymodem_running,ack,flag,file_done,all_done)
    rxbuff:del()
    if result then
        rxbuff:copy(0, ack,flag)
        uart.tx(id, rxbuff)
    end
    if all_done then
        ymodem_running = false  --Start receiving again
    end
    rxbuff:del()
end
uart.on(uartid, "receive", ymodem_rx)

uart.on(uartid, "sent", function(id)
    log.info("uart", "sent", id)
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
