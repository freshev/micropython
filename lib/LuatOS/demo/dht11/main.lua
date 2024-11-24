
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "dht12"
VERSION = "1.0.0"

--sys library is standard
_G.sys = require("sys")

sys.taskInit(function()
    local dht11_pin = 7
    while 1 do
        sys.wait(1000)
        local h,t,r = sensor.dht1x(dht11_pin, true)
        log.info("dht11", "湿度", h/100, "温度", t/100,r)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
