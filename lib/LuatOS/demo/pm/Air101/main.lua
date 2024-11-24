
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "pmdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Note: This demo uses luatools to download!!!
--Note: This demo uses luatools to download!!!
--Note: This demo uses luatools to download!!!


--Judge and initialize rtc at startup
local t = rtc.get()
log.info("rtc", json.encode(t))
if t["year"] == 1900 then
    --For air101/air103
    --If reset is started, the rtc time must be 1900, just set the time
    --If it is waking up, the rtc time must have been set, and it is definitely not 1900, so there is no need to set it.
    rtc.set({year=2022,mon=9,day=1,hour=0,min=0,sec=0})
end
t = nil --Release the variable t

--Add a hard watchdog to prevent the program from freezing
wdt.init(9000)--Initialize watchdog set to 9s
sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds

local LEDA = gpio.setup(24, 0, gpio.PULLUP) --PB8 output mode
local LEDB = gpio.setup(25, 0, gpio.PULLUP) --PB9 output mode
local LEDC = gpio.setup(26, 0, gpio.PULLUP) --PB10 output mode


--The task started here also flashes to show the sleep state.
sys.taskInit(function()
    local count = 0
    local uid = mcu.unique_id() or ""
    while 1 do
        --Twinkle, twinkle, twinkling
        LEDA(count & 0x01 == 0x01 and 1 or 0)
        LEDB(count & 0x02 == 0x02 and 1 or 0)
        LEDC(count & 0x03 == 0x03 and 1 or 0)
        log.info("gpio", "Go Go Go", uid:toHex(), count)
        sys.wait(1000)
        count = count + 1
    end
end)

sys.taskInit(function()
    --For air101/air103, it will sleep immediately after the request.
    --If there is no sleep, please check the level of the wakeup pin
    --There are two types of hibernation, LIGHT when the RAM does not lose power and HIB when the RAM loses power.

    --The sys.wait here is to prevent it from sleeping immediately after booting.
    --The actual situation is to run the business code and sleep after the conditions are met. The sys.wait here is just to demonstrate the business execution.
    sys.wait(3000)

    --First demonstrate LIGHT mode hibernation, RAM does not lose power, IO is maintained, and the code continues to run after waking up.
    --Set sleep duration
    log.info("pm", "轻休眠15秒", "要么rtc唤醒,要么wakeup唤醒", "RAM不掉电")
    log.info("rtc", json.encode(rtc.get()))
    pm.dtimerStart(0, 15000)
    pm.request(pm.LIGHT)
    --The following statements will be executed after 15000ms under normal circumstances.
    log.info("pm", "轻睡眠唤醒", "代码继续跑")
    --Wait here for 3000ms to simulate business execution, during which the gpio flashing task will continue to run.
    log.info("rtc", json.encode(rtc.get()))
    sys.wait(3000)

    --Test RTC wake-up
    local t = rtc.get()
    if t.sec < 30 then
        log.info("rtc", "轻唤醒测试", "5秒后唤醒")
        t.sec = t.sec + 5
        rtc.timerStart(0, t)
        pm.request(pm.LIGHT)
        log.info("rtc", "轻唤醒测试", "唤醒成功")

        --RTC deep sleep + wake up
        --t.sec = t.sec + 5
        --rtc.timerStart(0, t)
        --log.info("rtc", "Deep wake-up test", "Wake up after 5 seconds")
        -- pm.request(pm.DEEP)
    end

    --Next, we will demonstrate DEEP mode hibernation, RAM power-off, IO failure, and wake-up is equivalent to reset and restart.
    --Because it has been awakened, the dtimer has expired. Reset it.
    log.info("rtc", json.encode(rtc.get()))
    log.info("pm", "深休眠5秒", "要么rtc唤醒,要么wakeup唤醒", "RAM掉电")
    pm.dtimerStart(0, 5000)
    pm.request(pm.DEEP)
    --The following code will not be executed. If it is executed, it is 100% because there is a problem with the wakeup level.
    log.info("pm", "这里的代码不会被执行")
    log.info("rtc", json.encode(rtc.get()))
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
