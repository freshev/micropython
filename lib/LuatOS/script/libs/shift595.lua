--[[
@module shift595
@summary shift595 74HC595芯片
@version 1.0
@date    2023.08.30
@author  lulipro
@usage
--Notice:
--1. The sclk shift clock pin and dat data pin must be provided during initialization. rclk can be selected according to application requirements.
--2. AIR101 official core board, the bottom layer is LuatOS-SoC_V0017_AIR101.soc. After testing, the serial clock frequency of this script library is 18KHz.
--Usage examples:
--Hardware Modules: Dual 595 driven common anode 4-bit digital tube
local shift595 = require("shift595")
sys.taskInit(function() 

    shift595.init(pin.PB08,pin.PB09,pin.PB10)  -- sclk,dat,rclk
    
    while 1 do
        local wei = 1
        for i = 0, 3, 1 do
            shift595.out(0x82,shift595.MSB)--Send segment data, then bit select data
            shift595.out(wei,shift595.MSB)--Send segment data, then bit select data
            shift595.latch() --latch
            wei = wei<<1
            sys.wait(500)
        end
        sys.wait(1000)
    end
end
)
]]

local shift595 = {}

local sys = require "sys"

shift595.MSB=0     --In byte serial output, the highest bit is sent first
shift595.LSB=1     --In byte serial output, the lowest bit is sent first

local SHIFT595_SCLK     --Serial shift clock pin
local SHIFT595_DAT      --serial data pin
local SHIFT595_RCLK     --Latch signal clock pin


--[[
75hc595芯片初始化
@api shift595.init(sclk,dat,rclk)
@number sclk,定义驱动595串行时钟信号的引脚
@number dat,定义驱动595串行数据的引脚
@number rclk,定义驱动595锁存信号的引脚，可选
@usage
shift595.init(pin.PB08,pin.PB09,pin.PB10)  -- sclk,dat,rclk
]]
function shift595.init(sclk,dat,rclk)
    SHIFT595_SCLK = gpio.setup(sclk, 1)
    SHIFT595_DAT  = gpio.setup(dat, 1)
    
    if rclk then
        SHIFT595_RCLK = gpio.setup(rclk, 1)
    else
        SHIFT595_RCLK = nil
    end
end


--[[
串行输出一个字节到74hc595芯片的移位寄存器中
@api shift595.out(dat,endian)
@number dat,发送的字节数据
@number endian,指定发送字节数据时的大小端模式，有shift595.MSB和shift595.LSB两种参数可选。默认shift595.MSB
@usage
shift595.out(0x82,shift595.MSB)
shift595.out(0x82)  --The default is shift595.MSB, which is equivalent to the above
]]
function shift595.out(dat,endian)
    local mbit
    for i = 0, 7, 1 do
        SHIFT595_SCLK(0)
        if endian == shift595.LSB then
            mbit = ((dat>>i)&0x01~=0) and 1 or 0
        else
            mbit = ((dat<<i)&0x80~=0) and 1 or 0
        end
        SHIFT595_DAT(mbit)
        SHIFT595_SCLK(1)
    end
end


--[[
给74hc595芯片的RCLK线一个高脉冲，使得移位寄存器中的数据转移到锁存器中，当OE使能时，数据就输出到QA~QH引脚上。如果初始化时没用到rclk引脚则此函数调用无效。
@api shift595.latch()
@usage
shift595.latch()
]]
function shift595.latch()
    if SHIFT595_RCLK then
        SHIFT595_RCLK(0)
        SHIFT595_RCLK(1)
    end
end

return shift595
