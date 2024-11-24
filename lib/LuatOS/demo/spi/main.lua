
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "w25q_spi_demo"
VERSION = "1.0.1"

sys = require("sys")

--SPI number, please modify it according to the actual situation!
local spiId = 0
--CS is lame, please modify it as needed!
local cs = 17
local cspin = gpio.setup(cs, 1)

--Send and receive data
local function sendRecv(data,len)
    local r = ""
    cspin(0)
    if data then spi.send(spiId,data) end
    if len then r = spi.recv(spiId,len) end
    cspin(1)
    return r
end


sys.taskInit(function()

    local result = spi.setup(
        spiId,--serial port id
        nil,
        0,--CPHA
        0,--CPOL
        8,--data width
        100000--,--frequency
        --spi.MSB,--high and low order, optional, high-end first by default
        --spi.master,--master mode optional, default master
        --spi.full--full duplex optional, default full duplex
    )
    print("open",result)
    if result ~= 0 then--The return value is 0, indicating that the opening is successful.
        print("spi open error",result)
        return
    end

    --Check chip model
    local chip = sendRecv(string.char(0x9f),3)
    if chip == string.char(0xef,0x40,0x16) then
        log.info("spi", "chip id read ok 0xef,0x40,0x16")
    else
        log.info("spi", "chip id read error")
        for i=1,#chip do
            print(chip:byte(i))
        end
        return
    end

    local data = "test data 123456"

    --enable write
    sendRecv(string.char(0x06))

    --Write page data to address 0x000001
    sendRecv(string.char(0x02,0x00,0x00,0x01)..data)
    log.info("spi","write",data)

    sys.wait(500)--Wait for the write operation to complete

    --Read data
    local r = sendRecv(string.char(0x03,0x00,0x00,0x01),data:len())
    log.info("spi","read",r)

    --disable write
    sendRecv(string.char(0x04))

    spi.close(spiId)
end)

--It always ends with this sentence
sys.run()
