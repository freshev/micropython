
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "irdemo"
VERSION = "1.0.0"

--sys library is standard
sys = require("sys")


sys.taskInit(function()
    while true do
        ir.sendNEC(0, 0x11, 0x22)
        sys.wait(1000)
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
