--[[
@module lm75
@summary lm75 温度传感器 支持lm75a lm75b
@version 1.0
@date    2022.04.12
@author  Dozingfiretruck
@usage
--Usage examples
local lm75 = require "lm75"
i2cid = 0
i2c_speed = i2c.FAST
sys.taskInit(function()
    i2c.setup(i2cid,i2c_speed)
    lm75.init(i2cid)--Initialization, pass in i2c_id
    while 1 do
        local lm75_data = lm75.get_data()
        if lm75_data then
            log.info("lm75_data", lm75_data.."℃")
        end
        sys.wait(1000)
    end
end)
]]


local lm75 = {}
local sys = require "sys"
local i2cid

local LM75_ADDRESS_ADR         =   0x48

---The address used by the device

local LM75_CONF                =   0x01 --configuration register
local LM75_TEMP                =   0x00 --Temperature register
local LM75_TOS                 =   0x03 --Thermal Shutdown Threshold Register
local LM75_THYST               =   0x02 --hysteresis register


--[[
lm75_data 初始化
@api lm75_data.init(i2c_id)
@number 所在的i2c总线id
@return bool   成功返回true
@usage
lm75_data.init(0)
]]
function lm75.init(i2c_id)
    i2cid = i2c_id
    return true
end

--[[
获取 lm75 数据
@api lm75.get_data()
@return table lm75 数据
@usage
local lm75_data = lm75.get_data()
if lm75_data then
    log.info("lm75_data", lm75_data.."℃")
end

]]
function lm75.get_data()
    local temp
    i2c.send(i2cid, LM75_ADDRESS_ADR,LM75_TEMP)
    local _,data = pack.unpack(i2c.recv(i2cid, LM75_ADDRESS_ADR, 2),">h")
    if data then
        if bit.isclear(bit.rshift(data,5), 10) then
            temp = bit.rshift(data,5)*0.125
        else
            temp = -(bit.bxor(bit.rshift(data,5),0x3F8)+1)*0.125
        end
    end
    return temp
end

return lm75


