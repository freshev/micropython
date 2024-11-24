--[[
@module aht10
@summary aht10 温湿度传感器
@version 1.0
@date    2022.03.10
@author  Dozingfiretruck
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
local aht10 = require "aht10"
i2cid = 0
i2c_speed = i2c.FAST
sys.taskInit(function()
    i2c.setup(i2cid,i2c_speed)
    aht10.init(i2cid)--Initialization, pass in i2c_id
    while 1 do
        local aht10_data = aht10.get_data()
        log.info("aht10_data", "aht10_data.RH:"..(aht10_data.RH*100).."%","aht10_data.T"..(aht10_data.T).."℃")
        sys.wait(1000)
    end
end)
]]


local aht10 = {}
local sys = require "sys"
local i2cid

local AHT10_ADDRESS_ADR_LOW       =   0x38

---The address used by the device
local AHT10_INIT                  =   0xE1 --initialization command
local AHT10_MEASURE               =   0xAC --Trigger measurement command
local AHT10_SOFT_RESET            =   0xBA --Soft reset command, the time required for soft reset does not exceed 20 milliseconds.

local AHT10_STATE                 =   0x71 --status word.

--[[
aht10初始化
@api aht10.init(i2c_id)
@number 所在的i2c总线id
@return bool   成功返回true
@usage
aht10.init(0)
]]
function aht10.init(i2c_id)
    i2cid = i2c_id
    sys.wait(40)--40 milliseconds to wait for device to stabilize
    i2c.send(i2cid, AHT10_ADDRESS_ADR_LOW, AHT10_SOFT_RESET)--soft reset
    sys.wait(20)
    i2c.send(i2cid, AHT10_ADDRESS_ADR_LOW, AHT10_STATE)
    local data = i2c.recv(i2cid, AHT10_ADDRESS_ADR_LOW, 1)
    local _,state = pack.unpack(data, "b")
    if not state then
        log.info("aht10", "not found")
        return
    end
    if bit.isclear(state,3) then
        i2c.send(i2cid, AHT10_ADDRESS_ADR_LOW, {AHT10_INIT,0x08,0x00})--initialization
    end
    sys.wait(20)
    log.info("aht10 init_ok")
    return true
end

--Get raw data
local function aht10_get_raw_data()
    local raw_data={Srh=nil,St=nil}
    i2c.send(i2cid, AHT10_ADDRESS_ADR_LOW, {AHT10_MEASURE, 0x33, 0x00})
    sys.wait(80)--Wait more than 80 milliseconds
    i2c.send(i2cid, AHT10_ADDRESS_ADR_LOW, AHT10_STATE)
    local data = i2c.recv(i2cid, AHT10_ADDRESS_ADR_LOW, 1)
    local _,state = pack.unpack(data, "b")
    --if bit.isclear(state,7) then
        local data = i2c.recv(i2cid, AHT10_ADDRESS_ADR_LOW, 6)
        local _, data1, data2, data3, data4, data5, data6 = pack.unpack(data, "b6")
        raw_data.Srh = bit.bor(bit.bor(bit.rshift(data4, 4), bit.lshift(data3, 4)),bit.lshift(data2, 12))
        raw_data.St = bit.bor(bit.bor(bit.lshift(bit.band(data4, 0x0f), 16), bit.lshift(data5, 8)), data6)
    -- end
    return raw_data or 0
end

--[[
获取aht10数据
@api aht10.get_data()
@return table aht10数据
@usage
local aht10_data = aht10.get_data()
log.info("aht10_data", "aht10_data.RH:"..(aht10_data.RH*100).."%","aht10_data.T"..(aht10_data.T).."℃")
]]
function aht10.get_data()
    local aht10_data={RH=nil,T=nil}
    local raw_data = aht10_get_raw_data()
    aht10_data.RH = raw_data.Srh/1048576
    aht10_data.T = raw_data.St/1048576*200-50
    return aht10_data or 0
end

return aht10


