--[[
@module am2320
@summary am2320 温湿度传感器
@version 1.0
@date    2023.07.31
@author  wendal
@demo    am2320
@usage
--Usage examples
local am2320 = require "am2320"
sys.taskInit(function()
    local i2c_id = 0
    i2c.setup(i2c_id, i2c.FAST)
    while 1 do
        local t, h = am2320.read(i2c_id)
        log.info("am2320", "温度", t, "湿度", h)
        sys.wait(1000)
    end
end)
]]


local am2320 = {}

--[[
读取温湿度数据
@api am2320.read(i2c_id)
@int i2c总线的id, 默认是0, 需要按实际接线来填, 例如0/1/2/3
@return number 温度,单位摄氏度,若读取失败会返回nil
@return number 相对湿度,单位1%,若读取失败会返回nil
]]
function am2320.read(i2c_id)
    if not i2c_id then
        i2c_id = 0
    end
    local i2cslaveaddr = 0x5C --The 8bit address is 0xb8 and the 7bit address is 0x5C
    i2c.send(i2c_id, i2cslaveaddr, 0x03)
    --Query function code: 0x03 Query register first address: 0 Length: 4
    i2c.send(i2c_id, i2cslaveaddr, {0x03, 0x00, 0x04})
    local _, ismain = coroutine.running()
    if ismain then
        timer.mdelay(2)
    else
        sys.wait(2)
    end
    local data = i2c.recv(i2c_id, i2cslaveaddr, 8)

    --8-bit data format returned by the sensor:
    --1       2       3       4       5       6       7       8
    --0x03    0x04    0x03    0x39     0x01    0x15    0xE1    0XFE
    --Function code Data length Humidity high bit Humidity data Temperature high bit Temperature low bit CRC low CRC high

    if data == nil or #data ~= 8 then
        return
    end
    --log.info("AM2320", "buf data:", buf)
    --log.info("AM2320", "HEX data:", data:toHex())

    local _, crc = pack.unpack(data, '<H', 7)
    data = data:sub(1, 6)
    if crc == crypto.crc16_modbus(data, 6) then
        local _, hum, tmp = pack.unpack(string.sub(data, 3, -1), '>H2')
        --Positive and negative temperature treatment
        if tmp >= 0x8000 then
            tmp = 0x8000 - tmp
        end
        tmp, hum = tmp / 10, hum / 10
        --log.info("AM2320", "data(tmp hum):", tmp, hum)
        return tmp, hum
    end
end

return am2320
