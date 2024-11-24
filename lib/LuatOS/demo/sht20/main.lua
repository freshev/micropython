
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "sht20demo"
VERSION = "1.0.0"

--sys library is standard
sys = require("sys")

--wiring
--[[
SHT20 --- Module
SDA   -   I2C_SDA
SCL   -   I2C_SCL
VCC   -   VDDIO
GND   -   GND
]]


--Start a task and query SHT20 data regularly
sys.taskInit(function()

    local tmp,hum --raw data
    local temp,hump --true value

    --1010 000x
    local addr = 0x40
    --Modify according to actual situation
    local id = 0

    log.info("i2c", "initial",i2c.setup(id))

    while true do
        --first way
        i2c.send(id, addr, string.char(0xF3))
        sys.wait(100)
        tmp = i2c.recv(id, addr, 2)
        log.info("SHT20", "read tem data", tmp:toHex())

        i2c.send(id, addr, string.char(0xF5))
        sys.wait(100)
        hum = i2c.recv(id, addr, 2)
        log.info("SHT20", "read hum data", hum:toHex())
        local _,tval = pack.unpack(tmp,'>H')
        local _,hval = pack.unpack(hum,'>H')
        if tval and hval then
            temp = (((17572 * tval) >> 16) - 4685)/100
            hump = (((12500 * hval) >> 16) - 600)/100
            log.info("SHT20", "temp,humi",temp,hump)
        end
        sys.wait(1000)
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
