
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "osdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

sys.taskInit(function()
    sys.wait(1000)

    --Get local time string
    log.info("本地时间字符串", os.date())
    --Get UTC time string
    log.info("UTC时间字符串", os.date("!%c"))

    --Format local time string
    log.info("本地时间字符串", os.date("%Y-%m-%d %H:%M:%S"))
    --Format UTC time string
    log.info("UTC时间字符串", os.date("!%Y-%m-%d %H:%M:%S"))

    --Format time string
    log.info("自定义时间的字符串", os.date("!%Y-%m-%d %H:%M:%S", os.time({year=2000, mon=1, day=1, hour=0, min=0, sec=0})))
    
    --Get the table of local time
    log.info("本地时间字符串", json.encode(os.date("*t")))
    --Get table of UTC time
    log.info("UTC时间字符串",  json.encode(os.date("!*t")))


    --Timestamp, but the precision under Lua can only be seconds
    log.info("UTC时间戳", os.time())
    log.info("自定义时间戳", os.time({year=2000, mon=1, day=1, hour=0, min=0, sec=0}))
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
