--- Module function: lolademo
--@Modules lora
--@author Dozingfiretruck
--@release 2021.06.17

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "lorademo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

local rtos_bsp = rtos.bsp()

-- spi_id,pin_cs,pin_reset,pin_busy,pin_dio1
local function lora_pin()     
    if rtos_bsp == "AIR101" then
        return 0,pin.PB04,pin.PB00,pin.PB01,pin.PB06
    elseif rtos_bsp == "AIR103" then
        return 0,pin.PB04,pin.PB00,pin.PB01,pin.PB06
    elseif rtos_bsp == "AIR105" then
        return 5,pin.PC14,pin.PE08,pin.PE09,pin.PE06
    elseif rtos_bsp == "ESP32C3" then
        return 2,7,6,11,5
    elseif rtos_bsp == "ESP32S3" then
        return 2,14,15,13,12
    elseif rtos_bsp == "EC618" then
        return 0,8,1,18,19
    elseif rtos_bsp == "EC718P" then
        return 0,8,1,31,32
    else
        log.info("main", "bsp not support")
        return
    end
end

local spi_id,pin_cs,pin_reset,pin_busy,pin_dio1 = lora_pin() 

sys.subscribe("LORA_TX_DONE", function()
    lora.recive(1000)
end)

sys.subscribe("LORA_RX_DONE", function(data, size)
    log.info("LORA_RX_DONE: ", data, size)
    lora.send("PING")
end)

sys.subscribe("LORA_RX_TIMEOUT", function()
    lora.recive(1000)
end)

sys.taskInit(function()
    lora.init("llcc68",{id = spi_id,cs = pin_cs,res = pin_reset,busy = pin_busy,dio1 = pin_dio1})
    
    lora.set_channel(433000000)
    lora.set_txconfig("llcc68",
    {mode=1,power=22,fdev=0,bandwidth=0,datarate=9,coderate=4,preambleLen=8,
        fixLen=false,crcOn=true,freqHopOn=0,hopPeriod=0,iqInverted=false,timeout=3000}
    )
    lora.set_rxconfig("llcc68",
    {mode=1,bandwidth=0,datarate=9,coderate=4,bandwidthAfc=0,preambleLen=8,symbTimeout=0,fixLen=false,
        payloadLen=0,crcOn=true,freqHopOn=0,hopPeriod=0,iqInverted=false,rxContinuous=false}
    )

    lora.send("PING")
    while 1 do
        sys.wait(1000)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!


