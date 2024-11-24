
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "hmetademo"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


sys.taskInit(function()
    while hmeta do
        --hmeta identifies the underlying Modules type
        --Different Moduless can use the same bsp, but depending on the package, the specific Modules can still be identified based on internal data
        log.info("hmeta", hmeta.model(), hmeta.hwver and hmeta.hwver())
        log.info("bsp",   rtos.bsp())
        sys.wait(3000)
    end
    log.info("这个bsp不支持hmeta库哦")
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
