--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "max30102demo"
VERSION = "1.0.0"

--log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
local sys = require "sys"

--sys.timerLoopStart(function ()
--log.info("mem.lua", rtos.meminfo())
--log.info("mem.sys", rtos.meminfo("sys"))
--end, 3000)


_G.sysplus = require("sysplus")

function pinx()
    local bsp = rtos.bsp()
    if bsp:startsWith("ESP32") then
        return 0, 2
    elseif bsp == "AIR105" then
        return 0, pin.PC05
    elseif bsp == "AIR101" or bsp == "AIR103" or bsp == "AIR601" then
        return 0, 10
    else
        return 0, 1
    end
end

local i2cid, irq_pin = pinx()
local i2c_speed = i2c.FAST
sys.taskInit(function()
    log.info("初始化i2c")
    i2c.setup(i2cid, i2c_speed)
    log.info("初始化max30102")
    max30102.init(i2cid, irq_pin)
    -- max30102.get().wait()
    -- max30102.shutdown()
    while 1 do
        log.info("尝试读取")
        local ret,HR,SpO2 = max30102.get().wait()
        if ret then
            log.info("max30102", HR,SpO2)
        else
            log.info("max30102", "false")
        end
        sys.wait(5000)
    end
end)




--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
