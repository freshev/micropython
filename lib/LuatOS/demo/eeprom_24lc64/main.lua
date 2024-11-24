--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "eeprom_demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000) --Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000) --Feed the watchdog once every 3 seconds
end

--[[
    示例里将A0、A1、A2、WP引脚都接地
]]

local eeprom = require "eeprom"

local EEPROM_DEVICE_ADDRESS             =       0x50                    --i2c device address (7bit)
local EEPROM_RegAddr                    =       "\x00\x01"              --EEPROM address to write/read
local dataToWrite                       =       "\xC3\x23\xB1"          --Sample data to write
local DATA_LEN                          =       3                       --Read the length of data

i2cid       =   0
i2cSpeed    =   i2c.FAST

sys.taskInit(function()
    local code = i2c.setup(i2cid,i2cSpeed)
    while true do
        local res_write = eeprom.writebyte(i2cid,EEPROM_DEVICE_ADDRESS,EEPROM_RegAddr,dataToWrite)
        log.info("transfer write: ",res_write)
        sys.wait(1000)
        local result, rxdata = eeprom.readbyte(i2cid,EEPROM_DEVICE_ADDRESS,EEPROM_RegAddr,DATA_LEN)
        log.info("transfer read: ",result,rxdata:toHex())
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
