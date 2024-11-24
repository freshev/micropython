
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "lbsLoc2demo"
VERSION = "1.0.0"


--sys library is standard
sys = require("sys")

local lbsLoc2 = require("lbsLoc2")

--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

sys.taskInit(function()
    sys.waitUntil("IP_READY", 30000)
    -- mobile.reqCellInfo(60)
    -- sys.wait(1000)
    while mobile do --Without the mobile library, there would be no base station positioning
        mobile.reqCellInfo(15)
        sys.waitUntil("CELL_INFO_UPDATE", 3000)
        local lat, lng, t = lbsLoc2.request(5000)
        --local lat, lng, t = lbsLoc2.request(5000, "bs.openluat.com")
        log.info("lbsLoc2", lat, lng, (json.encode(t or {})))
        sys.wait(60000)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
