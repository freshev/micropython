
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "helloworld"
VERSION = "1.0.0"

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")

log.info("main", "hello world")

print(_VERSION)

sys.timerLoopStart(function()
    print("hi, LuatOS")
    print("mem.lua", rtos.meminfo())
    print("mem.sys", rtos.meminfo("sys"))
end, 3000)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
