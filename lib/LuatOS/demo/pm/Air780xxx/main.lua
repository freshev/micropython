
--LuaTools requires two pieces of information, PROJECT and VERSION
--[[
--Demonstrate the functions of pm related APIs
PROJECT = "pmdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")
log.style(1)
--Note: This demo uses luatools to download!!!
--Note: This demo uses luatools to download!!!
--Note: This demo uses luatools to download!!!


--Judge and initialize rtc at startup
local reason, slp_state = pm.lastReson()
log.info("wakeup state", pm.lastReson())
if reason > 0 then
    pm.request(pm.LIGHT)
    log.info("已经从深度休眠唤醒，测试结束")
    mobile.flymode(0, false)
    sys.taskInit(function() 
        sys.wait(10000)
    end)
else
    log.info("普通复位，开始测试")
    --To test the lowest power consumption, the following 3 GPIO operations are required
    gpio.setup(23,nil)
    gpio.close(33)
    gpio.close(35) --Here the pwrkey is only needed if it is grounded, not if it is controlled by buttons without grounding.
    sys.taskInit(function()
        pm.power(pm.GPS, true) --Turn on the internal GPS power of 780EG. Note that if you really use GPS, you need to initialize UART2.
        pm.power(pm.GPS_ANT, true) --Turn on the power of the 780EG internal GPS antenna. Note that if you really use GPS, you need to initialize UART2
        log.info("等联网完成")
        sys.wait(20000)
        pm.power(pm.GPS, false) --Turn on the internal GPS power of 780EG. Note that if you really use GPS, you need to initialize UART2.
        pm.power(pm.GPS_ANT, false) --Turn on the power of the 780EG internal GPS antenna. Note that if you really use GPS, you need to initialize UART2
        --lvgl refreshes too fast. If there is lvgl.init operation, you need to stop it first.
        if lvgl then
            lvgl.sleep(true)
        end
        pm.power(pm.USB, false)--If you are testing with the USB plugged in, you need to turn off the USB
        pm.force(pm.LIGHT)
        --If you want to test the power consumption while maintaining network connection, you need iotpower to test https://wiki.luatos.com/iotpower/power/index.html purchase link https://item.taobao.com/item.htm?id =679899121798
        --If you just want to see the bottom current in normal sleep state, you need to enter flight mode.
        --log.info("Normal sleep test, enter airplane mode to maintain stable current")
        --mobile.flymode(0, true)
        log.info("普通休眠测试，普通定时器就能唤醒，10秒后唤醒一下")
        sys.wait(10000)
        pm.force(pm.IDLE)
        pm.power(pm.USB, true)
        sys.wait(1000)
        log.info("普通休眠测试成功，接下来深度休眠，进入飞行模式或者PSM模式来保持稳定的电流，不开飞行模式会周期性唤醒，大幅度增加功耗")
        mobile.flymode(0, true)
        sys.wait(10000)
        log.info("深度休眠测试用DTIMER来唤醒")
        --On EC618, 0 and 1 can only last up to 2.5 hours, and 2~6 can last up to 750 hours.
        pm.dtimerStart(0, 10000)
        pm.force(pm.DEEP)   --You can also use pm.HIB mode
        pm.power(pm.USB, false) --If you are testing with the USB plugged in, you need to turn off the USB
        log.info("开始深度休眠测试")
        sys.wait(3000)
        log.info("深度休眠测试失败")
    end)
end
]]
--Demonstrate periodic work-hibernation demo
PROJECT = "sleepdemo"
VERSION = "1.0.1"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")
log.style(1)
--Note: This demo uses luatools to download!!!
--Note: This demo uses luatools to download!!!
--Note: This demo uses luatools to download!!!


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end


--Judge and initialize rtc at startup
local reason, slp_state = pm.lastReson()
log.info("wakeup state", pm.lastReson())
if reason > 0 then
    pm.request(pm.LIGHT)
	mobile.flymode(0, false)
    log.info("已经从深度休眠唤醒")
end
function io_init()
    local bsp = rtos.bsp()
    --To test the lowest power consumption, the following 3 GPIO operations are required
    if bsp == "EC618" then
        log.info("EC618")
        gpio.setup(23,nil)
        --gpio.close(33) --If the power consumption is high, start trying to close WAKEUPPAD1
        gpio.close(35) --Here the pwrkey is only needed if it is grounded, not if it is controlled by buttons without grounding.
    end
    if string.find(bsp,"EC718") then
        log.info("EC718")
        gpio.close(23) 
        gpio.close(45) 
        gpio.close(46) --Here the pwrkey is only needed if it is grounded, not if it is controlled by buttons without grounding.
        --The full IO development board has the lowest power consumption according to the following configuration. Configure your own board with wakeuppad according to the actual situation.
        gpio.setup(39,nil,gpio.PULLUP) 
        gpio.setup(40,nil,gpio.PULLDOWN) 
        gpio.setup(41,nil,gpio.PULLDOWN) 
        gpio.setup(42,nil,gpio.PULLUP) 
        gpio.setup(43,nil,gpio.PULLUP) 
        gpio.setup(44,nil,gpio.PULLDOWN) 
    end
end




sys.taskInit(function()
    io_init()
	log.info("工作14秒后进入深度休眠")
	sys.wait(14000)
	mobile.flymode(0, true)
	log.info("深度休眠测试用DTIMER来唤醒")
	sys.wait(100)
	pm.power(pm.USB, false) --If you are testing with the USB plugged in, you need to turn off the USB
	pm.force(pm.HIB)
	pm.dtimerStart(3, 40000)
	sys.wait(5000)
    pm.force(pm.IDLE) --Running to this point shows that it is useless to enter deep sleep and the test fails.
	pm.power(pm.USB, true) 
	log.info("深度休眠测试失败")
	mobile.flymode(0, false)
	while true do
		sys.wait(5000)
		log.info("深度休眠测试失败")
	end
end)




--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
