
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "pmdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Note: This demo uses luatools to download!!!
--Note: This demo uses luatools to download!!!
--Note: This demo uses luatools to download!!!

sys.taskInit(function()
    while 1 do
        sys.wait(3000)
        log.info("pm", "休眠60秒", "GPIO下降沿唤醒，键盘唤醒和RTC闹钟唤醒")
        --air105 only supports id=0. The actual precision is seconds, but the parameter requirement is milliseconds.
        --So the following call id=0, timeout=60*1000
        pm.dtimerStart(0, 60000)
        --air105air105 only supports pm.DEEP, which is a pause mode. It does not reset after waking up. The code continues to run and can be interrupted by falling edges, RTC interrupts, and hardware keyboard interrupts.
        pm.request(pm.DEEP)
        sys.wait(100)
        pm.request(pm.IDLE)
        --air105 does not reset after waking up, the code continues to run, the following code is executed after waking up
        log.info("pm", "系统被唤醒", "代码继续执行")
        sys.publish("SYS_WAKEUP")
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
