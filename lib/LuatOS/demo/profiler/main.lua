
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "memtest"
VERSION = "1.0.0"

--[[
lua内存分析库, 未完成
]]

--sys library is standard
_G.sys = require("sys")

sys.taskInit(function()
    sys.wait(1000)
    collectgarbage()
    collectgarbage()
    sys.wait(1000)
    profiler.start()
    while 1 do
        log.info("sys", rtos.meminfo("sys"))
        log.info("lua", rtos.meminfo("lua"))
        sys.wait(3000)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
