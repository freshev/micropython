
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "i2c 24c02 demo"
VERSION = "1.0.0"

--sys library is standard
sys = require("sys")

--1010 000x
--7bit address, excluding the last read and write bit
local addr = 0x50
--Change the number according to the actual chip
local i2cid = 0

sys.taskInit(function()
    log.info("i2c initial",i2c.setup(i2cid))
    while true do
        --first way
        i2c.send(i2cid, addr, string.char(0x01).."1234abcd")
        sys.wait(100)
        i2c.send(i2cid, addr, string.char(0x01))
        local data = i2c.recv(i2cid, addr, 8)
        log.info("i2c", "data1",data:toHex(),data)

        --Second way
        i2c.writeReg(i2cid, addr, 0x01, "abcd1234")
        sys.wait(100)
        local data = i2c.readReg(i2cid, addr, 0x01, 8)
        log.info("i2c", "data2",data:toHex(),data)
        sys.wait(1000)
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
