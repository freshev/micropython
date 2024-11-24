
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "i2sdemo"
VERSION = "1.0.0"

--[[
本demo暂时只在air101/103/601测试过
对于EC618系列的模块,例如Air780E/Air700E,请使用audio库进行快捷播放

本demo需要外挂ES8311 codec芯片, 可使用海凌科的w800音频开发板进行测试

https://detail.tmall.com/item.htm?abbucket=2&id=670202333872
]]

--sys library is standard
sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

sys.taskInit(function()
    sys.wait(500)
    local multimedia_id = 0
    local i2c_id = 0
    local i2s_id = 0
    local i2s_mode = 0
    local i2s_sample_rate = 44100
    local i2s_bits_per_sample = 16
    local i2s_channel_format = 0
    local i2s_communication_format = 0
    local i2s_channel_bits = 32

    local pa_pin = 20
    local pa_on_level = 1
    local pa_delay = 20
    local power_pin = 255
    local power_delay = 0

    local voice_vol = 70

    i2c.setup(i2c_id,i2c.FAST)
    i2s.setup(i2s_id, i2s_mode, i2s_sample_rate, i2s_bits_per_sample, i2s_channel_format, i2s_communication_format,i2s_channel_bits)

    audio.config(multimedia_id, pa_pin, pa_on_level, power_delay, pa_delay, power_pin)
    audio.setBus(multimedia_id, audio.BUS_I2S,{chip = "es8311",i2cid = i2c_id , i2sid = i2s_id})	--The hardware output channel of channel 0 is set to I2S

    --Play parameter settings
    audio.start(multimedia_id, audio.PCM, 1, 16000, 16)
    --Volume settings
    audio.vol(multimedia_id, 50)
    audio.pm(multimedia_id,audio.RESUME)
    --PCM playback demonstration, 16k sampling rate, 16bit sampling depth
    local file_size = fs.fsize("/luadb/test.pcm")
    --print("/luadb/test.pcm size",file_size)
    local f = io.open("/luadb/test.pcm", "rb")
    if f then 
        while 1 do
            local data = f:read(4096)
            -- print("-------------")
            if not data or #data == 0 then
                break
            end
            audio.write(0, data)
            sys.wait(100)
        end
        f:close()
    end

    --mp3 test
    --It is recommended to use ffmpeg to preprocess mp3 files, which must be converted to mono.
    --ffmpeg -i abc.mp3 -ac 1 -map_metadata -1 -y out.mp3
    --Air101/Air103/Air601 supports sampling rate 8~44.1k, depth 8-16bit, the following is the conversion to 16k sampling rate
    --ffmpeg -i abc.mp3 -ac 1 -map_metadata -1 -ar 16000 -y out.mp3
    local path = "/luadb/out.mp3"
    local decoder = codec.create(codec.MP3)
    log.info("decoder", decoder)
    local result, audio_format, num_channels, sample_rate, bits_per_sample, is_signed= codec.info(decoder, path)
    log.info("info", result, audio_format, num_channels, sample_rate, bits_per_sample, is_signed)
    if result then
        --Switch the sampling rate and sampling depth according to the actual situation of mp3
        audio.start(0, audio.PCM, 1, sample_rate, bits_per_sample) 
        --Audio data buffer size
        local buff = zbuff.create(8*1024)
        --log.info("sys", rtos.meminfo("sys"))
        --log.info("buff", buff)
        while 1 do
            --log.info("Attempt to decode")
            local result = codec.data(decoder, buff, 4096)
            --log.info("parsing result", result)
            if result then
                while 1 do
                    --Determine whether the underlying buffer is free. If not, you need to wait.
                    local max, remain = i2s.txStat(0)
                    if max == 0 then
                        sys.wait(120)
                        break
                    end
                    if remain > (max / 2) then
                        sys.wait(10) --sys.waitUntil("AUDIO_INC", 10)
                    else
                        break --It's free enough, just write data
                    end
                end
                audio.write(0, buff)
                --log.info("Audio data has been written", buff:used())
            else
                break
            end
        end
    end
    --Release decoder resources
    codec.release(decoder)
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
