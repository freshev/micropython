--LuaTools requires two pieces of information, PROJECT and VERSION

PROJECT = "rf_test"

VERSION = "1.0.0"

PRODUCT_KEY = "1111"

--Introduce necessary library files (written in Lua), internal libraries do not require require

_G.sys = require("sys")

local uartid = uart.VUART_0 --Fixed id of USB virtual serial port

--initialization
local result = uart.setup(
    uartid,--serial port id
    115200,--The baud rate actually doesn't matter, it's a pure virtual serial port
    8,--data bits
    1--Stop bit
)


--Receiving data will trigger a callback, where "receive" is a fixed value
uart.on(uartid, "receive", function(id, len)
    local s = ""
    repeat
        s = uart.read(id, len)
        if s and #s > 0 then --#s is to take the length of the string
           log.info("uart", "receive", id, #s, s:toHex())
		   mobile.nstInput(s)
        end
    until s == ""
	mobile.nstInput(nil)
end)


mobile.nstOnOff(true,uart.VUART_0)
--It always ends with this sentence

sys.run()

--Do not add any statements after sys.run()!!!!!
