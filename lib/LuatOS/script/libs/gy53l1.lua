--[[
@module  gy53l1
@summary gy53l1激光测距传感器 
@version 1.0
@date    2023.11.14
@author  dingshuaifei
@usage
测量说明：
测量范围：5-4000mm(可选择短、中、长测量模式)
单次测量：测量一次后需要重新发送单次输出距离数据指令

--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
gy53l1=require"gy53l1"
local uart2=2
sys.taskInit(function()

    sys.wait(2000)
    --initialization
    gy53l1.init(uart2)
    
    --Set the mode, do not set it to the default mode, set the mode with a certain interval
    sys.wait(1000)
    gy53l1.mode(uart2,gy53l1.measuring_short)
    sys.wait(1000)
    gy53l1.mode(uart2,gy53l1.measuring_time_1)

    local data,mode,time
    while true do
        sys.wait(100)
        --Set a single measurement and return a value once.
        --gy53l1.mode(uart2,gy53l1.out_mode_query)

        data,mode,time=gy53l1.get()
        log.info('距离',data,'模式',mode,'时间',time)
    end
end)
]]

gy53l1={}

--received data
local uart_recv_val=""
--packet table
local recv_data={}
--data frame
--recv_data.data=0
--Frame header 1
recv_data.head1=0
--Frame header 2
recv_data.head2=0
--Data type of this frame
recv_data.type=0
--Data volume
recv_data.amount=0
--High 8 bits of data
recv_data.hight=0
--Lower eight bits of data
recv_data.low=0
--Measurement mode
recv_data.mode=0
--Checksum
recv_data.check_sum=0
--distance
local range=0

--------------------------------------------------Selectable measurement mode- --------------------------------------------------

--Default mode: continuous output, medium distance, measurement time 110ms, baud rate 9600

--Output mode setting instructions:
gy53l1.out_mode_coiled=string.char(0xA5,0x45,0xEA)    ---------------Continuously output distance data---1
--[[If it is set as a query command, send the command once and measure once]]
gy53l1.out_mode_query=string.char(0xA5,0x15,0xBA)     ---------------Single output distance data---2

--Save configuration instructions:
gy53l1.save=string.char(0xA5,0x25,0xCA)               ---------------The current configuration is saved when power is turned off; including baud rate (effective after powering on again), measurement mode
                                                       ---------------Form, measurement time, output mode setting
--Measurement mode setting instructions:
gy53l1.measuring_short=string.char(0xA5,0x51,0xF6)    ---------------Short distance measurement mode---1
gy53l1.measuring_middle=string.char(0xA5,0x52,0xF7)   ---------------Mid-distance measurement mode (default)---2
gy53l1.measuring_long=string.char(0xA5,0x53,0xF8)     ---------------Long distance measurement mode---3
--Measurement time setting instructions:
gy53l1.measuring_time_1=string.char(0xA5,0x61,0x06)   ---------------Measurement time 110ms (default)---1
gy53l1.measuring_time_2=string.char(0xA5,0x62,0x07)   ---------------Measurement time 200ms ---2
gy53l1.measuring_time_3=string.char(0xA5,0x63,0x08)   ---------------Measurement time 300ms ---3
gy53l1.measuring_time_4=string.char(0xA5,0x64,0x09)   ---------------Measurement time 55ms ---0
--Baud rate configuration:
gy53l1.ste_baut_1=string.char(0xA5,0xAE,0x53)         ---------------9600 (default) ---1
gy53l1.ste_baut_2=string.char(0xA5,0xAF,0x54)         ---------------115200---2

--example:
--uart.write(2,measuring_short) sets the working mode to short distance
--------------------------------------------------Selectable measurement mode- --------------------------------------------------

--[[
    参数：str 传入串口接收到的string类型的数据
    返回值：失败返回-1
]]
local function data_dispose(str)  
    recv_data.head1=string.byte(str,1)
    recv_data.head2=string.byte(str,2)
    recv_data.type=string.byte(str,3)
    recv_data.amount=string.byte(str,4)
    recv_data.hight=string.byte(str,5)
    recv_data.low=string.byte(str,6)
    recv_data.mode=string.byte(str,7)
    recv_data.check_sum=string.byte(str,8)

    if recv_data.head1 ~= 0x5A then
        log.info('帧头错误')
        return -1
    end
    --Checksum calculation
    local sum=recv_data.head1+recv_data.head2+recv_data.type+ recv_data.amount+recv_data.hight+recv_data.low+recv_data.mode
    sum=sum & 0xff
    if sum ==recv_data.check_sum then
        --Output distance value
        range=(recv_data.hight<<8)|recv_data.low
        --log.info("Distance:mm",range,"Measurement mode:",recv_data.mode & 0x03,"Measurement time:",(recv_data.mode>>2) & 0x03)
    else
        log.info('校验错误')
        return -1
    end
end

--[[
gy53l1初始化
@api gy53l1.init(id)
@number  id 串口id
@return  bool 成功返回true失败返回false
@usage
gy53l1.init(2) 
]]
function gy53l1.init(id)
    --initialization
    local uart_s=uart.setup(id, 9600, 8, 1)
    if uart_s ~=0 then 
        return false
    end

    --Set working mode
    --Receiving data will trigger a callback, where "receive" is a fixed value
    uart.on(id, "receive", function(id, len)
        local s = ""
        repeat
            --s = uart.read(id, 1024)
            s = uart.read(id, len)
            if #s > 0 then --#s is to take the length of the string
                --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
                --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
                --log.info("uart", "receive", id, #s, s)
                data_dispose(s) 
            end
            if #s == len then
                break
            end
        until s == ""
    end)
    return true
end

--[[
gy53l1设置工作模式
@api gy53l1.mode(id,mode)
@number id 串口id
@string mode 可选择配置模式
@return  bool 成功返回true失败返回false
@usage
gy53l1.mode(2,gy53l1.save)--Save current configuration when power off
gy53l1.mode(2,gy53l1.measuring_time_3)--Measurement time 300ms
gy53l1.mode(2,gy53l1.measuring_long)--Measurement distance selection
]]
function gy53l1.mode(id,mode)
    local ret_data=uart.write(id,mode)
    if recv_data ~=0 then
        return true
    else
        return false
    end
end

--[[
gy53l1获取数据
@api gy53l1.get()
@return number data 距离数据
@return number mode 当前测量模式
@return number time 当前测量时间
@usage
local data,mode,timer=gy53l1.get()
log.info("距离",data,"模式",mode,"时间",timer)
]]
function gy53l1.get()
    local data,mode,time= range , recv_data.mode & 0x03 , (recv_data.mode>>2) & 0x03
    return data,mode,time
end

return gy53l1
