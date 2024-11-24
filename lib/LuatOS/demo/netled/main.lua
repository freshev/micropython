
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "netLeddemo"
VERSION = "1.0.1"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")
local netLed = require("netLed")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


--LED pin determines the end of assignment

local LEDA= gpio.setup(27, 0, gpio.PULLUP)


sys.taskInit(function()
    --Running water lamp program
    sys.wait(5000) --Delay 5 seconds to wait for network registration
    log.info("mobile.status()", mobile.status())
    while true do
        if mobile.status() == 1 then
            sys.wait(600)
            netLed.setupBreateLed(LEDA)
        else
            sys.wait(3000)
            log.info("net fail")
        end
    end
end)

--The user code has ended. It always ends with this sentence.
sys.run()
--Do not add any statements after sys.run()!!!!!
