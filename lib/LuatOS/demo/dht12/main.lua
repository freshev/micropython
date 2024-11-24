
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "dht12"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

-- https://datasheet.lcsc.com/szlcsc/DHT12-Digital-temperature-and-humidity-sensor_C83989.pdf
--Air302 has only one i2c, id=0
--If you cannot read the data, please try the following operations
--1. Swap SCL and SDA
--2. Make sure that both SCL and SDA are pulled up to VCC (3.3v), 1k~10k

---- Initialize and open I2C operation DHT12
--local function read_dht12(id)

--local data = i2c.readReg(id, 0x5C, 0, 5)
--if not data then
--log.info("i2c", "read reg fail")
--         return
--     end


--log.info("DHT12 HEX data: ", data:toHex())
---- are humidity integer, humidity decimal, temperature integer, temperature and humidity respectively.
--local _, h_H, h_L, t_H, t_L,crc = pack.unpack(data, 'b5')
--log.info("DHT12 data: ", h_H, h_L, t_H, t_L)
---- Calculate the checksum. The sum of the first 4 digits should equal the value of the last digit.
--if (((h_H + h_L + t_H + t_L) & 0xFF )) ~= crc then
--log.info("DHT12", "check crc fail")
--return "0.0", "0.0"
--     end
---- It is necessary to consider the situation where the temperature is lower than 0 degrees. The 0th bit of t_L is the sign bit.
--local t_L2 = tonumber(t_L)
--if t_L2 > 127 then
--return h_H .. ".".. h_L, "-" .. t_H .. "." .. tostring(t_L2 - 128)
--     else
--return h_H .. ".".. h_L, t_H .. "." .. t_L
--     end
-- end

sys.taskInit(function()
    local id = 0--i2c id, please change it as needed
    while 1 do
        sys.wait(5000) --Read once every 5 seconds
        i2c.setup(id, i2c.SLOW)
        --log.info("dht12", read_dht12(0)) -- If you want to read in the traditional way, please uncomment the read_dht12 method
        log.info("dht12", i2c.readDHT12(id))
        i2c.close(id)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
