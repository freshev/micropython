--[[
@module netLed
@summary netLed 网络状态指示灯
@version 1.0
@date    2023.02.21
@author  DingHeng
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
local netLed = require ("netLed")

local LEDA = gpio.setup(27,1,gpio.PULLUP) --LED pin determines the end of assignment
sys.taskInit(function()
--breathing light
sys.wait(5080)--Delay 5 seconds to wait for network registration
log.info("mobile.status()", mobile.status())
  while true do
        if mobile.status() == 1 then--Registered
            sys.wait(688)
            netLed.setupBreateLed(LEDA)
        end
   end
end)
]]
 
netLed = {}
--Reference sys library
local sys = require("sys")


local simError        --SIM card status: true means abnormal, false or nil means normal
local flyMode         --Whether it is in airplane mode: true means yes, false or nil means no
local gprsAttached    --Whether to attach to the GPRS network, true means yes, false or nil means no
local socketConnected --Whether there is a socket connected to the background, true means yes, false or nil means no

--[[
网络指示灯表示的工作状态
NULL：功能关闭状态
FLYMODE：飞行模式
SIMERR：未检测到SIM卡或者SIM卡锁pin码等SIM卡异常
IDLE：未注册GPRS网络
GPRS：已附着GPRS数据网络
SCK：socket已连接上后台
]]
local ledState = "NULL"
local ON,OFF = 1,2
--Configured lighting and off duration under various working conditions (unit: milliseconds)
local ledBlinkTime =
{
    NULL = {0,0xFFFF},  --Always extinguished
    FLYMODE = {0,0xFFFF},  --Always extinguished
    SIMERR = {300,5700},  --On for 300 milliseconds, off for 5700 milliseconds
    IDLE = {300,3700},  --On for 300 milliseconds, off for 3700 milliseconds
    GPRS = {300,700},  --On for 300 milliseconds, off for 700 milliseconds
    SCK = {100,100},  --On for 100 milliseconds, off for 100 milliseconds
}


local ledSwitch = false   --Network indicator switch, true means on, false or nil means off
local LEDPIN = 27         --Network indicator default PIN pin (GPIO27)
local lteSwitch = false   --LTE indicator switch, true means on, false or nil means off
local LTEPIN = 26         --LTE indicator light default PIN pin (GPIO26)


--[[
更新网络指示灯表示的工作状态
@api netLed.setState
@return nil 无返回值
@usage
netLed.setState()
]]
 function netLed.setState()
    log.info("netLed.setState",ledSwitch,ledState,flyMode,simError,gprsAttached,socketConnected)
    if ledSwitch then
        local newState = "IDLE"
        if flyMode then
            newState = "FLYMODE"
        elseif simError then
            newState = "SIMERR"
        elseif socketConnected then
            newState = "SCK"
        elseif gprsAttached then
            newState = "GPRS"
        end
        --The indicator light status changes
        if newState~=ledState then
            ledState = newState
            sys.publish("NET_LED_UPDATE")
        end
    end
end

--[[
网络指示灯模块的运行任务
@api netLed.taskLed(ledPinSetFunc)
@return nil 无返回值
@usage 
local LEDA = gpio.setup(27,1,gpio.PULLUP) --LED pin determines the end of assignment
netLed.taskLed(LEDA)
]]
function netLed.taskLed(ledPinSetFunc)
    while true do
        --log.info("netLed.taskLed",ledPinSetFunc,ledSwitch,ledState)
        if ledSwitch then
            local onTime,offTime = ledBlinkTime[ledState][ON],ledBlinkTime[ledState][OFF]
            if onTime>0 then
                ledPinSetFunc(1)
                if not sys.waitUntil("NET_LED_UPDATE", onTime) then
                    if offTime>0 then
                        ledPinSetFunc(0)
                        sys.waitUntil("NET_LED_UPDATE", offTime)
                    end
                end
            else if offTime>0 then
                    ledPinSetFunc(0)
                    sys.waitUntil("NET_LED_UPDATE", offTime)
                end
            end            
        else
            ledPinSetFunc(0)
            break
        end
    end
end

--[[
LTE指示灯模块的运行任务
@api netLed.taskLte(ledPinSetFunc)
@return nil 无返回值
@usage 
local LEDA = gpio.setup(27,1,gpio.PULLUP) --LED pin determines the end of assignment
netLed.taskLte(LEDA)
]]
 function netLed.taskLte(ledPinSetFunc)
    while true do
        local _,arg = sys.waitUntil("LTE_LED_UPDATE")
        if lteSwitch then
            ledPinSetFunc(arg and 1 or 0)
        end
    end
end

--[[
配置网络指示灯和LTE指示灯并且立即执行配置后的动作
@api netLed.setup(flag,ledpin,ltepin)
@bool flag 是否打开网络指示灯和LTE指示灯功能,true为打开,false为关闭
@number ledPin 控制网络指示灯闪烁的GPIO引脚,例如pio.P0_1表示GPIO1
@number ltePin 控制LTE指示灯闪烁的GPIO引脚,例如pio.P0_4表示GPIO4
@return nil 无返回值
@usage 
netLed.setup(true,27,0)
]]
function netLed.setup(flag,ledPin,ltePin)
    --log.info("netLed.setup",flag,pin,ledSwitch)    
    local oldSwitch = ledSwitch
    if flag~=ledSwitch then
        ledSwitch = flag
        sys.publish("NET_LED_UPDATE")
    end
    if flag and not oldSwitch then
        sys.taskInit(netLed.taskLed, gpio.setup(ledPin or LEDPIN, 0))
    end
    if flag~=lteSwitch then
        lteSwitch = flag	
    end  
    if flag and ltePin and not oldSwitch then
        sys.taskInit(netLed.taskLte, gpio.setup(ltePin, 0))  
    end	
end

--[[
配置某种工作状态下指示灯点亮和熄灭的时长（如果用户不配置,使用netLed.lua中ledBlinkTime配置的默认值）
@api netLed.setBlinkTime(state,on,off)
@string state 某种工作状态,仅支持"FLYMODE"、"SIMERR"、"IDLE"、"GSM"、"GPRS"、"SCK"
@number on 指示灯点亮时长,单位毫秒,0xFFFF表示常亮,0表示常灭
@number off 指示灯熄灭时长,单位毫秒,0xFFFF表示常灭,0表示常亮
@return nil 无返回值 
@usage 
netLed.setBlinkTime(("FLYMODE",1000,500) --Indicates that when the flight mode is working, the indicator light flashes as follows: on for 1 second, off for 8.5 seconds.
]]
function netLed.setBlinkTime(state,on,off)
    if not ledBlinkTime[state] then log.error("netLed.setBlinkTime") return end    
    local updated
    if on and ledBlinkTime[state][ON]~=on then
        ledBlinkTime[state][ON] = on
        updated = true
    end
    if off and ledBlinkTime[state][OFF]~=off then
        ledBlinkTime[state][OFF] = off
        updated = true
    end
    --log.info("netLed.setBlinkTime",state,on,off,updated)
    if updated then sys.publish("NET_LED_UPDATE") end
end

--[[ 
呼吸灯
@api netLed.setupBreateLed(ledPin)
@function ledPin 呼吸灯的ledPin(1)用pins.setup注册返回的方法
@return nil 无返回值
@usage 
local netLed = require ("netLed")
local LEDA = gpio.setup(27,1,gpio.PULLUP) --LED pin determines the end of assignment
sys.taskInit(function()
--breathing light
sys.wait(5080)--Delay 5 seconds to wait for network registration
log.info("mobile.status()", mobile.status())
  while true do
        if mobile.status() == 1 then--Registered
            sys.wait(688)
            netLed.setupBreateLed(LEDA)
        end
   end
end)
]]
function netLed.setupBreateLed(ledPin)
--Breathing light status, PWM cycle
    local bLighting, bDarking, LED_PWM = false, true, 18
    if bLighting then
        for i = 1, LED_PWM - 1 do
            ledPin(0)
            sys.wait(i)
            ledPin(1)
            sys.wait(LED_PWM - i)
        end
        bLighting = false
        bDarking = true
        ledPin(0)
        sys.wait(700)
    end
    if bDarking then
        for i = 1, LED_PWM - 1 do
            ledPin(0)
            sys.wait(LED_PWM - i)
            ledPin(1)
            sys.wait(i)
        end
        bLighting = true
        bDarking = false
        ledPin(1)
        sys.wait(700)
    end
end

sys.subscribe("FLYMODE", function(mode) if flyMode~=mode then flyMode=mode netLed.setState() end end)
sys.subscribe("SIM_IND", function(para) if simError~=(para~="RDY") and simError~=(para~="GET_NUMBER") then simError=(para~="RDY") netLed.setState() end log.info("sim status", para) end)
sys.subscribe("IP_LOSE", function() if gprsAttached then gprsAttached=false netLed.setState() end log.info("mobile", "IP_LOSE", (adapter or -1) == socket.LWIP_GP) end)
sys.subscribe("IP_READY", function(ip, adapter) if gprsAttached~=adapter then gprsAttached=adapter netLed.setState() end log.info("mobile", "IP_READY", ip, (adapter or -1) == socket.LWIP_GP) end)
sys.subscribe("SOCKET_ACTIVE", function(active) if socketConnected~=active then socketConnected=active netLed.setState() end end)

return netLed
