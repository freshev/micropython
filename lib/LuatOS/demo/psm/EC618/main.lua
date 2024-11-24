
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "psmdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--For dual-SIM devices, you can set it to automatically select the SIM card.
--However, in this way, the pin where SIM1 is located is forced to be reused as SIM function and cannot be reused as GPIO.
-- mobile.simid(2)
sys.taskInit(function()
    mobile.rtime(2) --When there is no data interaction, RRC is automatically released after 2 seconds.
    mobile.config(mobile.CONF_T3324MAXVALUE,0)
    mobile.config(mobile.CONF_PSM_MODE,1)
    pm.force(pm.HIB)
    pm.power(pm.USB,false)
    gpio.setup(23,nil)
end)



--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
