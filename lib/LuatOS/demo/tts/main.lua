
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "testtts"
VERSION = "2.0.0"

--[[
本demo当前仅支持ec618系列和ec718系列

## 提醒:
1. 本demo需要2022.12.21及之后的源码所编译的LuatOS固件
2. 本demo若使用外置TTS资源的LuatOS固件, 就必须有外挂的SPI Flash, 起码1M字节, 后面有刷写说明
3. 下载脚本时, 把txt也加上一起下载
4. 本demo需要音频扩展板, 780E只有I2S输出, 需要codec和PA才能驱动喇叭
5. 内置TTS资源的LuatOS最低版本是V1104,且去掉了很多库, 尤其是UI方面的库

## 使用本demo前,如果是外置TTS资源的LuatOS固件, 必须先刷tts.binpkg进行SPI Flash的刷写
1. 下载链接:
    618系列:https://gitee.com/openLuat/luatos-soc-2022/attach_files
    718系列:https://gitee.com/openLuat/luatos-soc-2023/attach_files
2. 在LuaTools主界面, 用"下载固件"按钮进行下载.
3. 下载前需要接好SPI Flash!!
4. 下载前选日志模式 4G USB, luatools版本号2.1.85或以上

## SPI Flash布局, 以1M字节为例,供参考:

-----------------------------------
64 k 保留空间, 用户自行分配
-----------------------------------
704k TTS数据
-----------------------------------
剩余空间, 256k,用户自行分配
-----------------------------------

## 基本流程:
1. 初始化sfud, 本demo使用SPI0 + GPIO8
2. 使用 audio.tts播放文本
3. 等待 播放结束事件
4. 从第二步重新下一个循环

## 接线说明

以780E开发板为例, 需要1.5版本或以上,团购版本均为1.5或以上.
1.4版本SPI分布有所不同, 注意区分.

https://wiki.luatos.com/chips/air780e/board.html

 xx脚指开发板pinout图上的顺序编号, 非GPIO编号

Flash --Development board
GND   --16 pin, GND
VCC   --15 pins, 3.3V
CLK   --Pin 14, GPIO11/SPI0_CLK/LCD_CLK, clock. If it is a version 1.4 development board, connect to pin 05 GPIO11/UART2_TXD
MOSI  --Pin 13, GPIO09/SPI0_MOSI/LCD_OUT, main control data output
MISO  --Pin 11, GPIO10/SPI0_MISO/LCD_RS, main control data input. If it is a version 1.4 development board, connect to pin 06 GPIO10/UART2_RXD
CS    --Pin 10, GPIO08/SPI0_CS/LCD_CS, chip select.

注意: 12脚是跳过的, 接线完毕后请检查好再通电!!
]]

--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

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
		--The CORE+ audio small board has this configuration
        pm.power(pm.LDO_CTL, false)  --The ES8311 on the development board is powered on and off controlled by LDO_CTL
        sys.wait(100)
        pm.power(pm.LDO_CTL, true)  --The ES8311 on the development board is powered on and off controlled by LDO_CTL

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
        local mic_vol = 80
    
        i2c.setup(i2c_id,i2c.FAST)
        i2s.setup(i2s_id, i2s_mode, i2s_sample_rate, i2s_bits_per_sample, i2s_channel_format, i2s_communication_format,i2s_channel_bits)
    
        audio.config(multimedia_id, pa_pin, pa_on_level, power_delay, pa_delay, power_pin, power_on_level, power_time_delay)
        audio.setBus(multimedia_id, audio.BUS_I2S,{chip = "es8311",i2cid = i2c_id , i2sid = i2s_id})	--The hardware output channel of channel 0 is set to I2S

        audio.vol(multimedia_id, voice_vol)
        audio.micVol(multimedia_id, mic_vol)
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

local taskName = "task_audio"

local MSG_MD = "moreData"   --There is room in the playback cache
local MSG_PD = "playDone"   --Playback completes all data

audio.on(0, function(id, event)
    --When using play to play a file, there is only a playback completion callback.
    local succ,stop,file_cnt = audio.getError(0)
    if not succ then
        if stop then
            log.info("用户停止播放")
        else
            log.info("第", file_cnt, "个文件解码失败")
        end
    end
    sysplus.sendMsg(taskName, MSG_PD)
end)


local function audio_task()
    local result    
    sys.waitUntil("AUDIO_READY")
    --Initialize spi flash. If it is the extreme version TTS_ONCHIP, there is no need to initialize it.
    if sfud then
        spi_flash = spi.deviceSetup(0,8,0,0,8,25600000,spi.MSB,1,0)
        local ret = sfud.init(spi_flash)
        if ret then
            log.info("sfud.init ok")
        else
            log.info("sfud.init error", ret)
            return
        end
    else
        log.info("tts", "TTS_ONCHIP?? skip sfud")
    end

    --This example is to play "Thousand Character Essay" line by line. The text comes from Wikipedia
    local fd = nil
    local line = nil
    while true do
        log.info("开始播放")
        line = nil
        if not fd then
            fd = io.open("/luadb/qianzw.txt")
        end
        if fd then
            line = fd:read("*l")
            if line == nil then
                fd:close()
                fd = nil
            end
        end
        if line == nil then
            line = "一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十"
        end
        line = line:trim()
        log.info("播放内容", line)
        result = audio.tts(0, line)
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
        if audio.pm then
		    audio.pm(0,audio.STANDBY)
        end
		--audio.pm(0,audio.SHUTDOWN) --For low power consumption, you can choose SHUTDOWN or POWEROFF. If the codec cannot power off, use SHUTDOWN.
        log.info("mem", "sys", rtos.meminfo("sys"))
        log.info("mem", "lua", rtos.meminfo("lua"))
        sys.wait(1000)
    end
    sysplus.taskDel(taskName)
end

sysplus.taskInitEx(audio_task, taskName, task_cb)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
