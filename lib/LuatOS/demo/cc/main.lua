
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ccdemo"
VERSION = "1.0.0"
log.style(1)
--[[
    本demo暂时只在air780ep测试过
    本demo需要外挂ES8311 codec芯片
]]

--sys library is standard
sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    --wdt.init(9000)--Initialize watchdog set to 9s
    --sys.timerLoopStart(wdt.feed, 3000)--feed the watchdog once every 3 seconds
end
local up1 = zbuff.create(6400,0)
local up2 = zbuff.create(6400,0)
local down1 = zbuff.create(6400,0)
local down2 = zbuff.create(6400,0)
local cnt = 0

local function record(is_dl, point)
    if is_dl then
        log.info("下行数据，位于缓存", point+1, "缓存1数据量", down1:used(), "缓存2数据量", down2:used())
    else
        log.info("上行数据，位于缓存", point+1, "缓存1数据量", up1:used(), "缓存2数据量", up2:used())
    end
	log.info("通话质量", cc.quality())
    --After initializing the serial port, you can send the corresponding zbuff through uart.tx.
end

sys.subscribe("CC_IND", function(state)
    if state == "READY" then
        sys.publish("CC_READY")
    elseif state == "INCOMINGCALL" then
		cnt = cnt + 1
		if cnt > 1 then
			cc.accept(0)
		end
    elseif state == "HANGUP_CALL_DONE" or state == "MAKE_CALL_FAILED" or state == "DISCONNECTED" then
		audio.pm(0,audio.STANDBY)
		--audio.pm(0,audio.SHUTDOWN) --For low power consumption, you can choose SHUTDOWN or POWEROFF. If the codec cannot power off, use SHUTDOWN.
	end
end)

sys.taskInit(function()
    cc.on("record", record)
    cc.record(true, up1, up2, down1, down2)
    local multimedia_id = 0
    local i2c_id = 0
    local i2s_id = 0
    local i2s_mode = 0
    local i2s_sample_rate = 16000
    local i2s_bits_per_sample = 16
    local i2s_channel_format = i2s.MONO_R
    local i2s_communication_format = i2s.MODE_LSB
    local i2s_channel_bits = 16

    local pa_pin = 25
    local pa_on_level = 1
    local pa_delay = 100
    local power_pin = 255
    local power_on_level = 1
    local power_delay = 3
    local power_time_delay = 100

    local voice_vol = 70
    local mic_vol = 65

    local find_es8311 = false
    --Open the following code for the built-in 8311 Modules (such as air780epa, air780epvh)
    --gpio.setup(17,1,nil,nil,4)
	--Adaptive development board, if it is clearly I2C, it is not needed.
    i2c.setup(0, i2c.FAST)
	i2c.setup(1, i2c.FAST)
	pm.power(pm.LDO_CTL, false)  --The ES8311 on the development board is powered on and off controlled by LDO_CTL
    sys.wait(10)
    pm.power(pm.LDO_CTL, true)  --The ES8311 on the development board is powered on and off controlled by LDO_CTL
	sys.wait(10)
    if i2c.send(0, 0x18, 0xfd) == true then
        log.info("音频小板或内置ES8311", "codec on i2c0")
        i2c_id = 0
        find_es8311 = true
    else
        if i2c.send(1, 0x18, 0xfd) == true then
            log.info("云喇叭开发板", "codec on i2c1")
            find_es8311 = true
            power_pin = nil
            i2c_id = 1
        end
    end

    if not find_es8311 then
        while true do
            log.info("not find es8311")
            sys.wait(1000)
        end
    end

    i2s.setup(i2s_id, i2s_mode, i2s_sample_rate, i2s_bits_per_sample, i2s_channel_format, i2s_communication_format,i2s_channel_bits)

    audio.config(multimedia_id, pa_pin, pa_on_level, power_delay, pa_delay, power_pin, power_on_level, power_time_delay)
    audio.setBus(multimedia_id, audio.BUS_I2S,{chip = "es8311", i2cid = i2c_id, i2sid = i2s_id})	--The hardware output channel of channel 0 is set to I2S

    audio.vol(multimedia_id, voice_vol)
    audio.micVol(multimedia_id, mic_vol)

    cc.init(multimedia_id)
	audio.pm(0,audio.STANDBY)
    sys.waitUntil("CC_READY")
    sys.wait(100)   
    --cc.dial(0,"114") --make a call



end)

-- sys.taskInit(function()
--while 1 do
---- Print memory status, for debugging
--         sys.wait(1000)
--log.info("lua", rtos.meminfo())
--log.info("sys", rtos.meminfo("sys"))
--     end
-- end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
