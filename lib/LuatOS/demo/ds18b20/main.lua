
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "dht12"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

sys.taskInit(function()
    local ds18b20_pin = 8
    while 1 do
        sys.wait(1000)
        local val,result = sensor.ds18b20(ds18b20_pin, true)
        log.info("ds18b20", val,result)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
