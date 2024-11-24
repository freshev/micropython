--[[
@module adxl34x
@summary adxl34x 3轴加速度计 目前支持 adxl345 adxl346
@version 1.0
@date    2022.04.11
@author  Dozingfiretruck
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
local adxl34x = require "adxl34x"
i2cid = 0
i2c_speed = i2c.FAST
sys.taskInit(function()
    i2c.setup(i2cid,i2c_speed)
    adxl34x.init(i2cid)--Initialization, pass in i2c_id
    while 1 do
        local adxl34x_data = adxl34x.get_data()
        log.info("adxl34x_data", "adxl34x_data.x"..(adxl34x_data.x),"adxl34x_data.y"..(adxl34x_data.y),"adxl34x_data.z"..(adxl34x_data.z))
        sys.wait(1000)
    end
end)
]]


local adxl34x = {}
local sys = require "sys"
local i2cid

local ADXL34X_ADDRESS_ADR

local ADXL34X_ADDRESS_ADR_LOW     =   0x53
local ADXL34X_ADDRESS_ADR_HIGH    =   0x1D

local ADXL34X_CHIP_ID_CHECK       =   0x00
local ADXL345_CHIP_ID             =   0xE5
local ADXL346_CHIP_ID             =   0xE6

---The address used by the device

local ADXL34X_THRESH_TAP          =   0x1D --tap threshold
local ADXL34X_OFSX                =   0x1E --X-axis offset
local ADXL34X_OFSY                =   0x1F --Y-axis offset
local ADXL34X_OFSZ                =   0x20 --Z-axis offset
local ADXL34X_DUR                 =   0x21 --tap duration
local ADXL34X_Latent              =   0x22 --tap delay
local ADXL34X_Window              =   0x23 --tap window
local ADXL34X_THRESH_ACT          =   0x24 --activity threshold
local ADXL34X_THRESH_INACT        =   0x25 --resting threshold
local ADXL34X_TIME_INACT          =   0x26 --still time
local ADXL34X_ACT_INACT_CTL       =   0x27 --Axis enable control activity and standstill detection
local ADXL34X_THRESH_FF           =   0x28 --free fall threshold
local ADXL34X_TIME_FF             =   0x29 --free fall time
local ADXL34X_TAP_AXES            =   0x2A --Single/double click axis control
local ADXL34X_ACT_TAP_STATUS      =   0x2B --Click/double click on source
local ADXL34X_BW_RATE             =   0x2C --Data rate and power mode control
local ADXL34X_POWER_CTL           =   0x2D --Power saving feature control
local ADXL34X_INT_ENABLE          =   0x2E --Interrupt enable control
local ADXL34X_INT_MAP             =   0x2F --Interrupt mapping control
local ADXL34X_INT_SOURCE          =   0x30 --Interrupt source
local ADXL34X_DATA_FORMAT         =   0x31 --Data format control
local ADXL34X_DATAX0              =   0x32 --X-axis data 0
local ADXL34X_DATAX1              =   0x33 --X-axis data 1
local ADXL34X_DATAY0              =   0x34 --Y axis data 0
local ADXL34X_DATAY1              =   0x35 --Y axis data 1
local ADXL34X_DATAZ0              =   0x36 --Z axis data 0
local ADXL34X_DATAZ1              =   0x37 --Z axis data 1
local ADXL34X_FIFO_CTL            =   0x38 --FIFO control
local ADXL34X_FIFO_STATUS         =   0x39 --FIFO status

local ADXL346_TAP_SIGN            =   0x3A --Symbols and sources for single/double clicks
local ADXL346_ORIENT_CONF         =   0x3B --directional placement
local ADXL346_Orient              =   0x3C --direction status

--Device ID detection
local function chip_check()
    i2c.send(i2cid, ADXL34X_ADDRESS_ADR_LOW, ADXL34X_CHIP_ID_CHECK)--Read device address
    local revData = i2c.recv(i2cid, ADXL34X_ADDRESS_ADR_LOW, 1)
    if revData:byte() ~= nil then
        ADXL34X_ADDRESS_ADR = ADXL34X_ADDRESS_ADR_LOW
    else
        i2c.send(i2cid, ADXL34X_ADDRESS_ADR_HIGH, ADXL34X_CHIP_ID_CHECK)--Read device address
        sys.wait(50)
        local revData = i2c.recv(i2cid, ADXL34X_ADDRESS_ADR_HIGH, 1)
        if revData:byte() ~= nil then
            ADXL34X_ADDRESS_ADR = ADXL34X_ADDRESS_ADR_HIGH
        else
            log.info("i2c", "Can't find adxl34x device")
            return false
        end
    end
    i2c.send(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_CHIP_ID_CHECK)--Read device address
    sys.wait(50)
    local revData = i2c.recv(i2cid, ADXL34X_ADDRESS_ADR, 1)
    if revData:byte() == ADXL345_CHIP_ID then
        log.info("Device i2c id is: ADXL345")
    elseif revData:byte() == ADXL346_CHIP_ID then
        log.info("Device i2c id is: ADXL346")
    else
        log.info("i2c", "Can't find adxl34x device")
        return false
    end
    return true
end

--[[
adxl34x 初始化
@api adxl34x.init(i2c_id)
@number 所在的i2c总线id
@return bool   成功返回true
@usage
adxl34x.init(0)
]]
function adxl34x.init(i2c_id)
    i2cid = i2c_id
    sys.wait(20)--20 milliseconds to wait for the device to stabilize
    if chip_check() then
        i2c.send(i2cid, ADXL34X_ADDRESS_ADR, {ADXL34X_BW_RATE,0X0D})
        i2c.send(i2cid, ADXL34X_ADDRESS_ADR, {ADXL34X_POWER_CTL,0X08})
        i2c.send(i2cid, ADXL34X_ADDRESS_ADR, {ADXL34X_DATA_FORMAT,0X2B})
        log.info("adxl34x init_ok")
        sys.wait(20)
        return true
    end
    return false
end

--[[
获取 adxl34x 数据
@api adxl34x.get_data()
@return table adxl34x 数据
@usage
local adxl34x_data = adxl34x.get_data()
log.info("adxl34x_data", "adxl34x_data.x"..(adxl34x_data.x),"adxl34x_data.y"..(adxl34x_data.y),"adxl34x_data.z"..(adxl34x_data.z))
]]
function adxl34x.get_data()
    local accel={x=nil,y=nil,z=nil}
    i2c.send(i2cid, ADXL34X_ADDRESS_ADR,ADXL34X_DATAX0)
    _,accel.x = pack.unpack(i2c.recv(i2cid, ADXL34X_ADDRESS_ADR, 2),">h")
    i2c.send(i2cid, ADXL34X_ADDRESS_ADR,ADXL34X_DATAY0)
    _,accel.y = pack.unpack(i2c.recv(i2cid, ADXL34X_ADDRESS_ADR, 2),">h")
    i2c.send(i2cid, ADXL34X_ADDRESS_ADR,ADXL34X_DATAZ0)
    _,accel.z = pack.unpack(i2c.recv(i2cid, ADXL34X_ADDRESS_ADR, 2),">h")
    return accel
end

--[[
获取 adxl34x 中断源
@api adxl34x.get_int_source()
@number 所在的i2c总线id
@usage
adxl34x.get_int_source(i2cid)
]]
function adxl34x.get_int_source(i2cid)
    i2c.readReg(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_INT_SOURCE, 2)
end

--[[
设置 adxl34x 活动和静止阀值
@api adxl34x.set_thresh(i2cid, activity, inactivity, time_inactivity)
@number 所在的i2c总线id
@number 活动阀值
@number 静止阀值
@number 静止时间
@usage
adxl34x.set_thresh(i2cid, string.char(0x05), string.char(0x02), string.char(0x05)) 
log.info("adxl34x_data", "adxl34x_data.x"..(adxl34x_data.x),"adxl34x_data.y"..(adxl34x_data.y),"adxl34x_data.z"..(adxl34x_data.z))
]]
function adxl34x.set_thresh(i2cid, activity, inactivity, time_inactivity)
    i2c.writeReg(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_THRESH_ACT, activity)
    i2c.writeReg(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_THRESH_INACT, inactivity)
    i2c.writeReg(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_THRESH_INACT, time_inactivity)
end

--[[
adxl34x 中断设置
@api adxl34x.set_irqf(i2cid, irqf_map, irqf_act_ctl, irqf_enable)
@number 所在的i2c总线id
@number 中断映射
@number 中断活动控制
@number 中断使能
@usage
adxl34x.set_irqf(i2cid, string.char(0x10), string.char(0xff), string.char(0x10))
]]
function adxl34x.set_irqf(i2cid, irqf_map, irqf_act_ctl, irqf_enable)
    i2c.writeReg(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_INT_ENABLE, string.char(0x00))     --Turn off all interrupts
    i2c.writeReg(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_INT_MAP, irqf_map)
    i2c.writeReg(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_ACT_INACT_CTL, irqf_act_ctl)
    i2c.writeReg(i2cid, ADXL34X_ADDRESS_ADR, ADXL34X_INT_ENABLE, irqf_enable)           --Enable interrupts
end

return adxl34x


