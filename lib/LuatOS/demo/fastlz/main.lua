
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "fastlzdemo"
VERSION = "1.0.0"

sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

sys.taskInit(function()
    sys.wait(1000)
    --compressed string
    local tmp = io.readFile("/luadb/fastlz.h") or "q309pura;dsnf;asdouyf89q03fonaewofhaeop;fhiqp02398ryhai;ofinap983fyua0weo;ifhj3p908fhaes;iofaw789prhfaeiwop;fhaesp98fadsjklfhasklfsjask;flhadsfk"
    local L1 = fastlz.compress(tmp)
    local dstr = fastlz.uncompress(L1)
    log.info("fastlz", "压缩等级1", #tmp, #L1, #dstr)
    L1 = nil
    dstr = nil
    local L2 = fastlz.compress(tmp, 2)
    local dstr = fastlz.uncompress(L2)
    log.info("fastlz", "压缩等级2", #tmp, #L2, #dstr)
    L1 = nil
    dstr = nil
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
