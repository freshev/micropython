--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "lcdsegdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require "sys"

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(15000)--Initialize watchdog set to 15s
    sys.timerLoopStart(wdt.feed, 10000)--Feed the watchdog once every 10 seconds
end


--initializelcdseg
if lcdseg.setup(lcdseg.BIAS_ONETHIRD, lcdseg.DUTY_ONEFOURTH, 33, 4, 60,0xff,0xffffffff) then
    lcdseg.enable(1)

    sys.taskInit(function ()
        while 1 do
            for i=0,3 do
                for j=1,31 do
                    lcdseg.seg_set(i, j, 1)
                    sys.wait(10)
                end
            end
            for i=0,3 do
                for j=1,31 do
                    lcdseg.seg_set(i, j, 0)
                    sys.wait(10)
                end
            end
        end
    end)
end



--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
