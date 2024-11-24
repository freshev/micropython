
PROJECT = "memdemo"
VERSION = "1.0.0"

sys = require("sys")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

sys.taskInit(function()
    local count = 1
    while 1 do
        sys.wait(1000)
        log.info("luatos", "hi", count, os.date())
        --memory
        log.info("lua", rtos.meminfo())
        --sysmemory
        log.info("sys", rtos.meminfo("sys"))
        count = count + 1
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
