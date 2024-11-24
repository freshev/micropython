
--[[
@module tm1650
@summary tm1650 数码管和按键扫描芯片
@version 1.0
@date    2023.09.07
@author  lulipro
@usage
--Notice:
--1. The digital tube driven by TM1650 should use common cathode digital tube.
--2. TM1650 can also drive LED. If it is LED, the LED should be connected to the same circuit inside the common cathode digital tube.
--3. Support key scanning. In this mode, the DP/KP pin of tm1650 is an interrupt output pin.
--=========Key scan example==========
local function tm1650_kcb(key_code)
    log.info('tm1650 user cb,key code=',string.format("%02X",key_code))
end
sys.taskInit(function()
    tm1650.init(pin.PB06,pin.PB07,tm1650.MODE_KEY_INPUT,pin.PB08,tm1650_kcb)
    while 1 do
        sys.wait(2000)
    end
end)
--=========Nigital tube display example============
sys.taskInit(function()
    --Common cathode segment code table, numbers from 0 to 9
    local NUM_TABLE_AX = {
        [0]=0x3f,[1]=0x06,[2]=0x5b,[3]=0x4f,[4]=0x66,
        [5]=0x6d,[6]=0x7d,[7]=0x07,[8]=0x7f,[9]=0x6f
    };   
    tm1650.init(pin.PB06,pin.PB07,tm1650.MODE_LED_OUTPUT)
    while 1 do
        for i = tm1650.DIG1, tm1650.DIG4, 1 do
            tm1650.print(i,NUM_TABLE_AX[6])
            sys.wait(500)
        end
        sys.wait(1000)
        for i = tm1650.BRIGHT1, tm1650.BRIGHT8, 1 do
            tm1650.setBright(i)
            sys.wait(500)
        end
        for i = 1, 8, 1 do
            sys.wait(500)
            tm1650.close()    
            sys.wait(500)
            tm1650.open()
        end
        sys.wait(2000)
        tm1650.clear()
    end
end)
]]

local tm1650 = {}

local sys = require "sys"

--Digital tube bit selection definition
tm1650.DIG1 = 0
tm1650.DIG2 = 1
tm1650.DIG3 = 2
tm1650.DIG4 = 3

--Eight-level brightness constant definition
tm1650.BRIGHT1 = 1
tm1650.BRIGHT2 = 2
tm1650.BRIGHT3 = 3
tm1650.BRIGHT4 = 4
tm1650.BRIGHT5 = 5
tm1650.BRIGHT6 = 6
tm1650.BRIGHT7 = 7
tm1650.BRIGHT8 = 8   --At level 8 brightness, the register binary bit is 000


--Working mode definition
tm1650.MODE_LED_OUTPUT = 0x71   --Digital tube LED drive mode (7 levels of brightness, 8-segment mode, open display)
tm1650.MODE_KEY_INPUT  = 0x79   --Key scan mode



local TM1650_SCL
local TM1650_SDA
local TM1650_SDA_PIN      --SDA pin
local TM1650_IRQ_PIN      --Interrupt pin number used for key detection
local TM1650_KEY_CALLBACK --Button callback function

local bright          = tm1650.BRIGHT7   --brightness variable
local is_display_on   = 1                --Show switch flag variable

local function tm1650_writeReg(reg,value)
    local dat
    local mbit
    --=====start========
    TM1650_SCL(1)
    TM1650_SDA(1)
    TM1650_SDA(0)
    TM1650_SCL(0)

    --======Write register address==========
    dat = reg
    for i = 0, 7, 1 do
        TM1650_SCL(0)
        mbit = (((dat<<i)&0x80) ~= 0) and 1 or 0
        TM1650_SDA(mbit)
        TM1650_SCL(1)
        TM1650_SCL(0)
    end
    TM1650_SDA(1)  --Release SDA
    TM1650_SCL(0)
    TM1650_SCL(1)
    TM1650_SCL(0)

    --======Write data==========
    dat = value
    for i = 0, 7, 1 do
        TM1650_SCL(0)
        mbit = (((dat<<i)&0x80) ~= 0) and 1 or 0
        TM1650_SDA(mbit)
        TM1650_SCL(1)
        TM1650_SCL(0)
    end
    TM1650_SDA(1)  --Release SDA
    TM1650_SCL(0)
    TM1650_SCL(1)
    TM1650_SCL(0)

    --=======stop=======
    TM1650_SCL(0)
    TM1650_SDA(0)
    TM1650_SCL(1)
    TM1650_SDA(1)
end


local function tm1650_readReg(reg)
    local dat
    local mbit
    --=====start========
    TM1650_SCL(1)
    TM1650_SDA(1)
    TM1650_SDA(0)
    TM1650_SCL(0)

    --======Write register address==========
    dat = reg
    for i = 0, 7, 1 do
        TM1650_SCL(0)
        mbit = (((dat<<i)&0x80) ~= 0) and 1 or 0
        TM1650_SDA(mbit)
        TM1650_SCL(1)
        TM1650_SCL(0)
    end
    TM1650_SDA(1)  --Release SDA
    TM1650_SCL(0)
    TM1650_SCL(1)
    TM1650_SCL(0)

    --=======Read data==========
    dat = 0
    gpio.setup(TM1650_SDA_PIN,nil,gpio.PULLUP)  --SDA input mode
    for i = 0, 7, 1 do
        TM1650_SCL(0)
        dat = (dat << 1)
        TM1650_SCL(1)
        if gpio.HIGH ==  gpio.get(TM1650_SDA_PIN) then
            dat = ( dat | 0x01)
        end 
        TM1650_SCL(0)
    end

    TM1650_SCL(0)
    TM1650_SCL(1)
    TM1650_SCL(0)

    TM1650_SDA = gpio.setup(TM1650_SDA_PIN,1)  --SDA resets to output mode
    --=======stop=======
    TM1650_SCL(0)
    TM1650_SDA(0)
    TM1650_SCL(1)
    TM1650_SDA(1)

    return dat
end


--[[
TM1650初始化，根据mode参数可以设置为数码管显示或者按键扫描模式
@api tm1650.init(scl_pin,sda_pin,mode,irq_pin,key_cb)
@number scl_pin，定义了时钟线驱动引脚
@number sda_pin，定义了数据线驱动引脚
@number mode，定义了工作模式，tm1650.MODE_LED_OUTPUT，数码管LED驱动模式；tm1650.MODE_KEY_INPUT，按键检测模式
@number irq_pin，定义按键中断引脚
@function key_cb，按键用户回调函数，此函数有一个number类型参数，为按下的按键的按键代码
@usage
tm1650.init(pin.PB06,pin.PB07,tm1650.MODE_LED_OUTPUT) --Digital tube display mode
tm1650.init(pin.PB06,pin.PB07,tm1650.MODE_KEY_INPUT,pin.PB08,tm1650_kcb)  --Key scan mode
]]
function tm1650.init(scl_pin,sda_pin,mode,irq_pin,key_cb)

    TM1650_SCL =  gpio.setup(scl_pin, 1)
    TM1650_SDA =  gpio.setup(sda_pin, 1)
    TM1650_SDA_PIN = sda_pin

    if mode == tm1650.MODE_KEY_INPUT then
        TM1650_IRQ_PIN      = irq_pin
        TM1650_KEY_CALLBACK = key_cb

        --Configure for key scan mode
        tm1650_writeReg(0x48,tm1650.MODE_KEY_INPUT)

        gpio.setup(
            TM1650_IRQ_PIN,       --interrupt gpio number
            function (val)        --Interrupt processing function, when triggered by falling edge
                local key_code = tm1650_readReg(0x49)  --Read key code
                if TM1650_KEY_CALLBACK then
                    TM1650_KEY_CALLBACK(key_code)
                end
                --log.info('tm1650 key get',string.format("%02X",key_code)) --Output key value
            end,
            gpio.PULLUP,    --Enable pull-up
            gpio.FALLING   --Falling edge trigger
        )
    else
        --Configured as digital tube LED driving mode, 7 levels of brightness, open display
        tm1650_writeReg(0x48,tm1650.MODE_LED_OUTPUT)
        tm1650.clear()
    end

end

--[[
设置TM1650的显示亮度，此操作不影响显存中的数据
@api tm1650.setBright(bri)
@number 亮度参数，取值为tm1650.BRIGHT1~tm1650.BRIGHT8
@usage
tm1650.setBright(tm1650.BRIGHT8)
]]
function tm1650.setBright(bri)
    if bri>tm1650.BRIGHT8 then bri = tm1650.BRIGHT8 end
    if bri<tm1650.BRIGHT1 then bri = tm1650.BRIGHT1 end
    bright = bri

    local bright_bits = bright
    if bright == tm1650.BRIGHT8 then bright_bits=0 end  --The register binary for brightness 8 is 000
    
    tm1650_writeReg(0x48,((bright_bits<<4)|is_display_on))
end

--[[
打开TM1650的显示，此操作不影响显存中的数据
@api tm1650.open()
@usage
tm1650.open()
]]
function tm1650.open()
    local bright_bits = bright
    if bright == tm1650.BRIGHT8 then bright_bits=0 end  --The register binary for brightness 8 is 000
    is_display_on=1

    tm1650_writeReg(0x48,((bright_bits<<4)|is_display_on))
end

--[[
关闭TM1650的显示，此操作不影响显存中的数据
@api tm1650.close()
@usage
tm1650.close()
]]
function tm1650.close()
    local bright_bits = bright
    if bright == tm1650.BRIGHT8 then bright_bits=0 end  --The register binary for brightness 8 is 000
    is_display_on=0

    tm1650_writeReg(0x48,((bright_bits<<4)|is_display_on))
end

--[[
向TM1650的一个指定的位对应的显存发送指定的段数据进行显示
@api tm1650.print(dig,seg_data)
@number dig，定义位选参数，取值为tm1650.DIG1~tm1650.DIG4
@number seg_data，定义段数据参数
@usage
tm1650.print(tm1650.DIG1,0x3f)
]]
function tm1650.print(dig,seg_data)
    if (dig>=tm1650.DIG1) and (dig<=tm1650.DIG4) then
        tm1650_writeReg(dig*2+0x68,seg_data)
    end
end

--[[
清除TM1650的所有位对应的显存数据，即全部刷写为0
@api tm1650.clear()
@usage
tm1650.clear()
]]
function tm1650.clear()
    for i = tm1650.DIG1, tm1650.DIG4, 1 do
        tm1650.print(i,0)
    end
end

return tm1650
