
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "audiotest"
VERSION = "2.0.1"

--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

log.style(1)
local taskName = "task_audio"

local MSG_MD = "moreData"   --There is room in the playback cache
local MSG_PD = "playDone"   --Playback completes all data

--The amr data storage buffer should be as large as possible.
amr_buff = zbuff.create(20 * 1024)
--Create an amr encoder
encoder = nil

audio.on(0, function(id, event,buff)
    --When using play to play a file, there is only a playback completion callback.
    if event == audio.RECORD_DATA then --Recording data
        codec.encode(encoder, buff, amr_buff)
    elseif event == audio.RECORD_DONE then --Recording completed
        sys.publish("AUDIO_RECORD_DONE")
    else
        local succ,stop,file_cnt = audio.getError(0)
        if not succ then
            if stop then
                log.info("用户停止播放")
            else
                log.info("第", file_cnt, "个文件解码失败")
            end
        end
        --log.info("Complete playing an audio")
        sysplus.sendMsg(taskName, MSG_PD)
    end
end)

function audio_setup()
    local bsp = rtos.bsp()
    if bsp == "EC618" then
        --Air780E development board matching + audio expansion board. ES7149
        --Since the PA of the audio expansion board is long-term powered, it is normal to have Tata sound. If you are making a product, you will need additional reference designs.
        i2s.setup(0, 0, 0, 0, 0, i2s.MODE_I2S)

        --If using TM8211, open the comments below
        --i2s.setup(0, 0, 0, 0, 0, i2s.MODE_MSB)

        --If using software DAC, open the comments below
        --if audio.setBus then
        --audio.setBus(0, audio.BUS_SOFT_DAC)
        -- end
        audio.config(0, 25, 1, 3, 100)
    elseif bsp == "EC718P" then
		--The CORE+ audio small board has this configuration/the cloud speaker development board also has this configuration
        local multimedia_id = 0
        local i2c_id = 0	--The development version of Cloud Speaker is 1
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
        local power_pin = 16 --Air780EPVH Please change it to 17
        local power_on_level = 1
        local power_delay = 3
        local power_time_delay = 100

        local voice_vol = 70
        local mic_vol = 80
    
		local find_es8311 = false
        --Open the following code for the built-in 8311 Modules (such as air780epa, air780epvh)
        --power_pin = 255
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
        audio.setBus(multimedia_id, audio.BUS_I2S,{chip = "es8311",i2cid = i2c_id , i2sid = i2s_id})	--The hardware output channel of channel 0 is set to I2S

        audio.vol(multimedia_id, voice_vol)
        audio.micVol(multimedia_id, mic_vol)
        -- audio.debug(true)

	--For the cloud speaker development board with TM8211, please refer to the configuration below.
		--[[
		local multimedia_id = 0
        local i2s_id = 0
        local i2s_mode = 0
        local i2s_sample_rate = 0
        local i2s_bits_per_sample = 16
        local i2s_channel_format = i2s.STEREO
        local i2s_communication_format = i2s.MODE_MSB
        local i2s_channel_bits = 16
    
        local pa_pin = 25
        local pa_on_level = 1
        local pa_delay = 100
        local power_pin = nil
        local power_on_level = 1
        local power_delay = 3
        local power_time_delay = 0
		local voice_vol = 100
        --local voice_vol = 200 -- no amplification by default
        i2s.setup(i2s_id, i2s_mode, i2s_sample_rate, i2s_bits_per_sample, i2s_channel_format, i2s_communication_format,i2s_channel_bits)
        audio.config(multimedia_id, pa_pin, pa_on_level, power_delay, pa_delay, power_pin, power_on_level, power_time_delay)
        audio.setBus(multimedia_id, audio.BUS_I2S,{chip = "tm8211", i2sid = i2s_id})	--The hardware output channel of channel 0 is set to I2S
        --audio.vol(multimedia_id, voice_vol)
		]]
    elseif bsp == "AIR105" then
        --Air105 development board supports DAC direct output
        audio.config(0, 25, 1, 3, 100)
    else
        --Not supported by other boards
        while 1 do
            log.info("audio", "尚未支持的BSP")
            sys.wait(1000)
        end
    end
    sys.publish("AUDIO_READY")
end

--Configure the audio peripherals
sys.taskInit(audio_setup)

local function audio_task()
    sys.waitUntil("AUDIO_READY")
    local result

    --The following is a recording demo, which can be selectively enabled according to the adaptation situation.
    --local recordPath = "/record.amr"
    
    ---- Record directly to file
    --err = audio.record(0, audio.AMR, 5, 7, recordPath)
    -- sys.waitUntil("AUDIO_RECORD_DONE")
    --log.info("record","Recording ended")
    --result = audio.play(0, {recordPath})
    --while true do
    --msg = sysplus.waitMsg(taskName, nil)
    --if type(msg) == 'table' then
    --if msg[1] == MSG_PD then
    --log.info("Playback ended")
    --             break
    --         end
    --     else
    --log.error(type(msg), msg)
    --     end
    -- end

    ---- Record to memory and encode by yourself
    --encoder = codec.create(codec.AMR, false, 7)
    -- print("encoder",encoder)
    --err = audio.record(0, audio.AMR, 5, 7)
    -- sys.waitUntil("AUDIO_RECORD_DONE")
    --log.info("record","Recording ended")
    -- os.remove(recordPath)
    --io.writeFile(recordPath, "#!AMR\n")
	--io.writeFile(recordPath, amr_buff:query(), "a+b")

	--result = audio.play(0, {recordPath})
    --while true do
    --msg = sysplus.waitMsg(taskName, nil)
    --if type(msg) == 'table' then
    --if msg[1] == MSG_PD then
    --log.info("Playback ended")
    --             break
    --         end
    --     else
    --log.error(type(msg), msg)
    --     end
    -- end

    --amr playable sampling rate 8k/16k
    local amrs = {"/luadb/alipay.amr", "/luadb/2.amr", "/luadb/10.amr", "/luadb/yuan.amr"}
    --If you need to mix broadcast in the same table, you need to use the same sampling rate
    --This mp3 is a free file with no copyright issues. It is self-recorded by Hezhou. If you want to test the sound quality, please use other high-definition mp3s.
    --local mp3s = {"/luadb/test_32k.mp3"}
	--The firmware of ec618 requires a non-full version to play 44k MP3
    local mp3s = {"/luadb/test_44k.mp3"}	
    local counter = 0
    while true do
        log.info("开始播放")
        --Play two lists before and after
        if rtos.bsp() == "AIR105" then
            result = audio.play(0, "/luadb/test_32k.mp3")
        else
            result = audio.play(0, counter % 2 == 1 and amrs or mp3s)
        end
        counter = counter + 1
        if result then
        --Wait for the callback message of the audio channel, or the message of switching songs
            while true do
                msg = sysplus.waitMsg(taskName, nil)
                if type(msg) == 'table' then
                    if msg[1] == MSG_PD then
                        log.info("播放结束")
                        break
                    end
                else
                    log.error(type(msg), msg)
                end
            end
        else
            log.debug("解码失败!")
            sys.wait(1000)
        end
        if not audio.isEnd(0) then
            log.info("手动关闭")
            audio.playStop(0)
        end
		audio.pm(0,audio.SHUTDOWN)
		--Open the code below for low power testing
		--[[
		audio.pm(0,audio.POWEROFF)	--For low power consumption, you can choose SHUTDOWN or POWEROFF. If the codec cannot power off, use SHUTDOWN.
		pm.power(pm.USB, false)
		mobile.flymode(0, true)
		pm.request(pm.LIGHT)
		sys.wait(20000)
		mobile.flymode(0, false)
		]]
        log.info(rtos.meminfo("sys"))
        log.info(rtos.meminfo("lua"))
        sys.wait(1000)
    end
    sysplus.taskDel(taskName)
end

sysplus.taskInitEx(audio_task, taskName, task_cb)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
