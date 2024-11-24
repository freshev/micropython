--UART1 supports 600 1200 2400 4800 9600 baud rate and still receives data in sleep mode without losing data.
--At other baud rates, wake up through the RX of UART1 after sleeping. Note that all continuous data will be lost when waking up, so it needs to be sent twice. After the first byte is sent, there will be a prompt, and then the data is sent.
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ZE08G-CH2O"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
local ch2o = require "ze08g_ch2o"

local uartid = 1 --Select different uartid according to the actual device

--The following is the configuration of simulated uart
--local tx_pin = 11 -- tx pin
--local rx_pin = 9 -- pin of rx
--local uartid = uart.createSoft(tx_pin,0,rx_pin,2)

sys.taskInit(function ()
    local result = ch2o.init(uartid)
    if not result then return end
    
    log.info("result:", result)
    while true do
        sys.wait(1000)
        log.info("气体浓度值 PPB：", ch2o.getPPB())
        log.info("百万分比浓度 PPM：", ch2o.getPPM())
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
