--[[
@module zh07
@summary ZH07 激光粉尘传感器
@version 1.0
@date    2023.03.09
@author  BaiShiyu
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
sys = require("sys")
local zh07 = require "zh07"
local uartid = 1 --Select different uartid according to the actual device

sys.taskInit(function ()
    local result = zh07.init(uartid)
    if not result then return end

    while true do
        sys.wait(1000)
        log.info(string.format("pm1.0  %sμg/m³", zh07.getPM_1()))
        log.info(string.format("pm2.5  %sμg/m³", zh07.getPM_2_5()))
        log.info(string.format("pm10   %sμg/m³", zh07.getPM_10()))
    end
end)
]]


local zh07 = {}
--local sys = require "sys"

local uartid    --Select different uartid according to the actual device
local rbuff

--[[
zh07初始化
@api zh07.init(uart_id)
@number uart_id uartid
@return bool   成功返回true
@usage
zh07.init(1)
]]
function zh07.init(uart_id)
    uartid = uart_id
    --initialization
    local result = uart.setup(
        uartid,--serial port id
        9600,--baud rate
        8,--data bits
        1--Stop bit
    )

    if result ~= 0 then
        log.info("zh07 init_fail")
        return false
    end

    --Receiving data will trigger a callback, where "receive" is a fixed value
    uart.on(uartid, "receive", function(id, len)
        local s = ""
        s = uart.read(id, len)
        if #s == 0 then return end

        local hexStr, hexLen = s:toHex()
        log.info("ZH07", "receive", hexStr, hexLen)

        if string.sub(hexStr,1,2) == "42" and hexLen == 64 then
            rbuff = s
        end
    end)

    log.info("zh07 init_ok")
    return true
end


--[[
获取zh07 PM1.0数据
@api zh07.getPM_1()
@return number PM1.0数据
@usage
local zh07_pm1 = zh07.getPM_1()
log.info(string.format("pm1.0  %sμg/m³", zh07_pm1))
]]
function zh07.getPM_1()
    if not rbuff then return 0 end

    return rbuff:byte(11)*256 + rbuff:byte(12)
end

--[[
获取zh07 PM2.5数据
@api zh07.getPM_2_5()
@return number PM2.5数据
@usage
local zh07_pm25 = zh07.getPM_2_5()
log.info(string.format("pm2.5  %sμg/m³", zh07_pm25))
]]
function zh07.getPM_2_5()
    if not rbuff then return 0 end

    return rbuff:byte(13)*256 + rbuff:byte(14)
end

--[[
获取zh07 PM10数据
@api zh07.getPM_10()
@return number PM10数据
@usage
local zh07_pm10 = zh07.getPM_10()
log.info(string.format("pm10  %sμg/m³", zh07_pm10))
]]
function zh07.getPM_10()
    if not rbuff then return 0 end

    return rbuff:byte(15)*256 + rbuff:byte(16)
end

return zh07

