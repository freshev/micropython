
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "sht30demo"
VERSION = "1.0.0"

--sys library is standard
sys = require("sys")

--wiring
--[[
SHT30 --- Air302
SDA   -   I2C_SDA
SCL   -   I2C_SCL
VCC   -   VDDIO
GND   -   GND
]]

--Tip, the I2C silk screen on the boss may be reversed. If the reading fails, please replace the SDA and SLA.

--Start a task and query SHT20 data regularly
sys.taskInit(function()

    --Default i2c address of sht30
    local addr = 0x44
    --Modify according to actual situation
    local id = 0

    log.info("i2c", "initial",i2c.setup(id))

    while true do
        --first way
        i2c.send(id, addr, string.char(0x2C, 0x06))
        sys.wait(5) -- 5ms
        local data = i2c.recv(id, addr, 6)
        log.info("sht30", data:toHex())
        if #data == 6  then
            local _,tval,ccrc,hval,hcrc = pack.unpack(data, ">HbHb")
            --local cTemp = ((((data:byte(1) * 256.0) + data:byte(2)) * 175) / 65535.0) - 45
            --local fTemp = (cTemp * 1.8) + 32
            --local humidity = ((((data:byte(4) * 256.0) + data:byte(5)) * 100) / 65535.0)
            local cTemp = ((tval * 175) / 65535.0) - 45
            --local fTemp = (cTemp * 1.8) + 32
            local humidity = ((hval * 100) / 65535.0)
            log.info("sht30", cTemp, humidity)
        end
        sys.wait(2000)
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
