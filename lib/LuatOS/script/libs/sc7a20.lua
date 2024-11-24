--[[
@module sc7a20
@summary sc7a20 
@version 1.0
@date    2022.04.11
@author  Dozingfiretruck
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
local sc7a20 = require "sc7a20"
i2cid = 0
i2c_speed = i2c.FAST
sys.taskInit(function()
    i2c.setup(i2cid,i2c_speed)
    sc7a20.init(i2cid)--Initialization, pass in i2c_id
    while 1 do
        local sc7a20_data = sc7a20.get_data()
        log.info("sc7a20_data", "sc7a20_data.x"..(sc7a20_data.x),"sc7a20_data.y"..(sc7a20_data.y),"sc7a20_data.z"..(sc7a20_data.z))
        sys.wait(1000)
    end
end)
]]


local sc7a20 = {}
local sys = require "sys"
local i2cid

local SC7A20_ADDRESS_ADR                    --sc7a20 device address

local SC7A20_ADDRESS_ADR_LOW     =   0x18
local SC7A20_ADDRESS_ADR_HIGH    =   0x19
local SC7A20_CHIP_ID_CHECK       =   0x0f   --Device ID detection
local SC7A20_CHIP_ID             =   0x11   --Device ID

local SC7A20_REG_CTRL_REG1       =   0x20   --Control register 1
local SC7A20_REG_CTRL_REG2       =   0x21   --Control register 2
local SC7A20_REG_CTRL_REG3       =   0x22   --Control register 3
local SC7A20_REG_CTRL_REG4       =   0x23   --Control register 4
local SC7A20_REG_CTRL_REG5       =   0x24   --Control register 5
local SC7A20_REG_CTRL_REG6       =   0x25   --Control register 6
local SC7A20_REG_REFERENCE       =   0x26   --reference register
local SC7A20_REG_STATUS_REG      =   0x27   --status register
local SC7A20_REG_OUT_X_L         =   0x28   --X-axis data low byte
local SC7A20_REG_OUT_X_H         =   0x29   --X-axis data high byte
local SC7A20_REG_OUT_Y_L         =   0x2A   --Y axis data low byte
local SC7A20_REG_OUT_Y_H         =   0x2B   --Y axis data high byte
local SC7A20_REG_OUT_Z_L         =   0x2C   --Z axis data low byte
local SC7A20_REG_OUT_Z_H         =   0x2D   --Z axis data high byte
local SC7A20_REG_FIFO_CTRL_REG   =   0x2E   --FIFO control register
local SC7A20_REG_FIFO_SRC_REG    =   0x2F   --FIFO source register
local SC7A20_REG_INT1_CFG        =   0x30   --Interrupt 1 configuration register
local SC7A20_REG_INT1_SRC        =   0x31   --Interrupt 1 source register
local SC7A20_REG_INT1_THS        =   0x32   --Interrupt 1 Threshold Register
local SC7A20_REG_INT1_DURATION   =   0x33   --Interrupt 1 duration register
local SC7A20_REG_INT2_CFG        =   0x34   --Interrupt 2 configuration register
local SC7A20_REG_INT2_SRC        =   0x35   --Interrupt 2 source register
local SC7A20_REG_INT2_THS        =   0x36   --Interrupt 2 Threshold Register
local SC7A20_REG_INT2_DURATION   =   0x37   --Interrupt 2 duration register
local SC7A20_REG_CLICK_CFG       =   0x38   --Click on Configuration Register
local SC7A20_REG_CLICK_SRC       =   0x39   --Click source register
local SC7A20_REG_CLICK_THS       =   0x3A   --Click on Threshold Register
local SC7A20_REG_TIME_LIMIT      =   0x3B   --Click time limit register
local SC7A20_REG_TIME_LATENCY    =   0x3C   --Click Time Delay Register
local SC7A20_REG_TIME_WINDOW     =   0x3D   --Click Time Window Register
local SC7A20_REG_ACT_THS         =   0x3E   --Activity Threshold Register
local SC7A20_REG_ACT_DUR         =   0x3F   --activity duration register


--Device ID detection
local function chip_check()
    i2c.send(i2cid, SC7A20_ADDRESS_ADR_LOW, SC7A20_CHIP_ID_CHECK)--Read device address
    local revData = i2c.recv(i2cid, SC7A20_ADDRESS_ADR_LOW, 1)
    if revData:byte() ~= nil then
        SC7A20_ADDRESS_ADR = SC7A20_ADDRESS_ADR_LOW
    else
        i2c.send(i2cid, SC7A20_ADDRESS_ADR_HIGH, SC7A20_CHIP_ID_CHECK)--Read device address
        sys.wait(50)
        local revData = i2c.recv(i2cid, SC7A20_ADDRESS_ADR_HIGH, 1)
        if revData:byte() ~= nil then
            SC7A20_ADDRESS_ADR = SC7A20_ADDRESS_ADR_HIGH
        else
            log.info("i2c", "Can't find sc7a20 device")
            return false
        end
    end
    i2c.send(i2cid, SC7A20_ADDRESS_ADR, SC7A20_CHIP_ID_CHECK)--Read device address
    sys.wait(50)
    local revData = i2c.recv(i2cid, SC7A20_ADDRESS_ADR, 1)
    if revData:byte() == SC7A20_CHIP_ID then
        log.info("Device i2c id is: SC7A20")
    else
        log.info("i2c", "Can't find sc7a20 device")
        return false
    end
    return true
end


--[[
sc7a20 初始化
@api sc7a20.init(i2c_id)
@number i2c id
@return bool   成功返回true
@usage
sc7a20.init(0)
]]
function sc7a20.init(i2c_id)
    i2cid = i2c_id
    sys.wait(20)--20 milliseconds to wait for the device to stabilize
    if chip_check() then
        i2c.send(i2cid, SC7A20_ADDRESS_ADR, {SC7A20_REG_CTRL_REG1,0X47})
        i2c.send(i2cid, SC7A20_ADDRESS_ADR, {SC7A20_REG_CTRL_REG2,0X00})
        i2c.send(i2cid, SC7A20_ADDRESS_ADR, {SC7A20_REG_CTRL_REG3,0X00})
        i2c.send(i2cid, SC7A20_ADDRESS_ADR, {SC7A20_REG_CTRL_REG4,0X88})
        i2c.send(i2cid, SC7A20_ADDRESS_ADR, {SC7A20_REG_CTRL_REG6,0X00})
        log.info("sc7a20 init_ok")
        sys.wait(20)
        return true
    end
    return false
end

--[[
获取 sc7a20 数据
@api sc7a20.get_data()
@return table sc7a20 数据
@usage
local sc7a20_data = sc7a20.get_data()
log.info("sc7a20_data", "sc7a20_data.x"..(sc7a20_data.x),"sc7a20_data.y"..(sc7a20_data.y),"sc7a20_data.z"..(sc7a20_data.z))
]]
function sc7a20.get_data()
    local accel={x=nil,y=nil,z=nil}
    i2c.send(i2cid, SC7A20_ADDRESS_ADR,SC7A20_REG_OUT_X_L)
    _,accel.x = pack.unpack(i2c.recv(i2cid, SC7A20_ADDRESS_ADR, 2),">h")
    i2c.send(i2cid, SC7A20_ADDRESS_ADR,SC7A20_REG_OUT_Y_L)
    _,accel.y = pack.unpack(i2c.recv(i2cid, SC7A20_ADDRESS_ADR, 2),">h")
    i2c.send(i2cid, SC7A20_ADDRESS_ADR,SC7A20_REG_OUT_Z_L)
    _,accel.z = pack.unpack(i2c.recv(i2cid, SC7A20_ADDRESS_ADR, 2),">h")
    return accel
end


--[[
设置 sc7a20 活动阀值
@api sc7a20.set_thresh (i2cid, activity, time_inactivity)
@number i2c id
@number 活动阀值
@number 活动持续时间
@usage
sc7a20.set_thresh(0, string.char(0x05), string.char(0x05)) 
]]
function sc7a20.set_thresh(i2cid, activity, time_inactivity)
    i2c.writeReg(i2cid, SC7A20_ADDRESS_ADR, SC7A20_REG_ACT_THS, activity)
    i2c.writeReg(i2cid, SC7A20_ADDRESS_ADR, SC7A20_REG_ACT_DUR, time_inactivity)
end


--[[
设置 sc7a20 中断设置
@api sc7a20.set_irqf(i2cid, int, irqf_ths, irqf_duration, irqf_cfg)
@number i2c id
@number 中断脚 传入1及配置INT1脚，传入2及配置INT2脚
@number 中断阀值
@number 中断持续时间
@number 中断配置
@usage
sc7a20.set_irqf(0, 1, string.char(0x05), string.char(0x05), string.char(0x00))
]]
function sc7a20.set_irqf(i2cid, int, irqf_ths, irqf_duration, irqf_cfg)
    if int == 1 then
        i2c.send(i2cid, SC7A20_ADDRESS_ADR, {SC7A20_REG_CTRL_REG3,0X40})        --AOI1 interrupt is mapped to INT1
        i2c.writeReg(i2cid, SC7A20_ADDRESS_ADR, SC7A20_REG_INT1_THS, irqf_ths)
        i2c.writeReg(i2cid, SC7A20_ADDRESS_ADR, SC7A20_REG_INT1_DURATION, irqf_duration)
        i2c.writeReg(i2cid, SC7A20_ADDRESS_ADR, SC7A20_REG_INT1_CFG, irqf_cfg)
    elseif int == 2 then
        i2c.send(i2cid, SC7A20_ADDRESS_ADR, {SC7A20_REG_CTRL_REG6,0X42})        --The AOI2 interrupt is mapped to INT2 and a low level trigger interrupt is configured.
        i2c.writeReg(i2cid, SC7A20_ADDRESS_ADR, SC7A20_REG_INT2_THS, irqf_ths)
        i2c.writeReg(i2cid, SC7A20_ADDRESS_ADR, SC7A20_REG_INT2_DURATION, irqf_duration)
        i2c.writeReg(i2cid, SC7A20_ADDRESS_ADR, SC7A20_REG_INT2_CFG, irqf_cfg)
    else
        log.info("sc7a20", "int Parameter error")
    end
end


return sc7a20


