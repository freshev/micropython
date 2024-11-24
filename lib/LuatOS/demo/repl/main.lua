
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "repl"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

sys.taskInit(function()
    if rtos.bsp() == "EC618" then
        uart.setup(uart.VUART_0)
        uart.on(uart.VUART_0, "recv", function(id, len)
            repeat
                s = uart.read(id, len)
                if s and #s > 0 then --#s is to take the length of the string
                    repl.push(s)
                end
            until s == ""
        end)
    end

end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!

