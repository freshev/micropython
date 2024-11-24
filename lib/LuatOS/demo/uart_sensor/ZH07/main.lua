--UART1 supports 600 1200 2400 4800 9600 baud rate and still receives data in sleep mode without losing data.
--At other baud rates, wake up through the RX of UART1 after sleeping. Note that all continuous data will be lost when waking up, so it needs to be sent twice. After the first byte is sent, there will be a prompt, and then the data is sent.
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ZH07"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
local zh07 = require "zh07"

local uartid = 1 --Select different uartid according to the actual device

--The following is the configuration of simulated uart
--local tx_pin = 11 -- tx pin
--local rx_pin = 9 -- pin of rx
--local uartid = uart.createSoft(tx_pin,0,rx_pin,2)

sys.taskInit(function ()
    local result = zh07.init(uartid)
    if not result then return end

    while true do
        sys.wait(1000)
        log.info(string.format("pm1.0  %sμg/m³", zh07.getPM_1()))
        log.info(string.format("pm2.5  %sμg/m³", zh07.getPM_2_5()))
        log.info(string.format("pm10   %sμg/m³", zh07.getPM_10()))
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
