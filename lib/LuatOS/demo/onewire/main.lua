
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "onewire"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")


sys.taskInit(function()
    sys.wait(3000)

    --Here is the wiring of Air103 as an example
    --Other bsps may not have pin libraries, just fill in the specific GPIO number directly.
    local ds18b20_pin = pin.PB20
    local dht11_pin = pin.PB10
    while 1 do
        log.info("开始读取ds18b20", "单位0.1摄氏度")
        if onewire then
            local temp = onewire.ds18b20(ds18b20_pin, true, onewire.GPIO)
            log.info("温度值", "onewire库", "ds18b20", temp)
            sys.wait(1000)
        end
        
        if sensor then
            local temp = sensor.ds18b20(ds18b20_pin, true)
            log.info("温度值", "sensor库", "ds18b20", temp)
            sys.wait(1000)
        end

        
        log.info("开始读取dht11", "单位0.01摄氏度和0.01%相对湿度")

        if onewire then
            local hm,temp = onewire.dht1x(dht11_pin, true, onewire.GPIO)
            log.info("温度值", "onewire库", "dht11", temp)
            log.info("湿度", "onewire库", "dht11", hm)
            sys.wait(1000)
        end
        
        if sensor then
            local hm,temp = sensor.dht1x(dht11_pin, true)
            log.info("温度值", "sensor库", "dht11", temp)
            log.info("湿度", "sensor库", "dht11", hm)
            sys.wait(1000)
        end

        if not onewire and not sensor then
            log.warn("固件不含onewire库和sensor库,无法演示")
            sys.wait(1000)
        end
    end

end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
