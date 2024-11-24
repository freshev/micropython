
--[[
本demo当前仅支持Ar780E和Air600E
## 提醒:
1. 本demo需要2022.12.21及之后的源码所编译的LuatOS固件
2. 本demo若使用外置TTS资源的LuatOS固件, 就必须有外挂的SPI Flash, 起码1M字节, 后面有刷写说明
3. 下载脚本时, 把txt也加上一起下载
4. 本demo需要音频扩展板, 780E只有I2S输出, 需要codec和PA才能驱动喇叭
5. 内置TTS资源的LuatOS最低版本是V1104,且去掉了很多库, 尤其是UI方面的库

## 使用本demo前,如果是外置TTS资源的LuatOS固件, 必须先刷tts.binpkg进行SPI Flash的刷写
1. 下载链接 https://gitee.com/openLuat/luatos-soc-2022/attach_files
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

local taskName = "task_audio"
local MSG_MD = "moreData" --There is room in the playback cache
local MSG_PD = "playDone" --Playback completes all data

audio.on(0, function(id, event)
    --When using play to play a file, there is only a playback completion callback.
    local succ, stop, file_cnt = audio.getError(0)
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
    --Air780E development board matching + audio expansion board. ES7149
    --Since the PA of the audio expansion board is long-term powered, it is normal to have Tata sound. If you are making a product, you will need additional reference designs.
    i2s.setup(0, 0, 0, 0, 0, i2s.MODE_I2S)

    --If using TM8211, open the comments below
    --i2s.setup(0, 0, 0, 0, 0, i2s.MODE_MSB)

    --If using software DAC, open the comments below
    --if audio.setBus then
    --     audio.setBus(audio.BUS_SOFT_DAC)
    -- end
    audio.config(0, 25, 1, 6, 200)
    gpio.setup(24, 0)
    gpio.setup(23, 0)
    gpio.setup(27, 0)
    gpio.setup(2, 0)

    --Initialize spi flash. If it is the extreme version TTS_ONCHIP, there is no need to initialize it.
    --if sfud then
    --spi_flash = spi.deviceSetup(0,8,0,0,8,25600000,spi.MSB,1,0)
    --local ret = sfud.init(spi_flash)
    --if ret then
    --log.info("sfud.init ok")
    --     else
    --log.info("sfud.init error", ret)
    --         return
    --     end
    -- end

    log.info("开始播放音频")
    result = audio.play(0, {"/audio.mp3"})
    if result then
        --Wait for the callback message of the audio channel, or the message of switching songs
        while true do
            msg = sysplus.waitMsg(taskName, nil)
            if type(msg) == 'table' then
                if msg[1] == MSG_PD then
                    log.info("音频播放结束")
                    sys.publish("audio_end")
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
    -- sys.wait(1000)
    sysplus.taskDel(taskName)
end


local function tts_task()
    --Air780E development board matching + audio expansion board. ES7149
    --Since the PA of the audio expansion board is long-term powered, it is normal to have Tata sound. If you are making a product, you will need additional reference designs.
    i2s.setup(0, 0, 0, 0, 0, i2s.MODE_I2S)

    --If using TM8211, open the comments below
    --i2s.setup(0, 0, 0, 0, 0, i2s.MODE_MSB)


    --If using software DAC, open the comments below.  Note that this function is adapted to the pwm audio playback development board. Since the MP3 file is 32K, PWM does not currently support it, and the played sound will be slower.
    --if audio.setBus then
    --     audio.setBus(audio.BUS_SOFT_DAC)
    -- end

    audio.config(0, 25, 1, 6, 200)
    gpio.setup(24, 0)
    gpio.setup(23, 0)
    gpio.setup(27, 0)
    gpio.setup(2, 0)
    --Initialize spi flash. If it is the extreme version TTS_ONCHIP, there is no need to initialize it.
    --if sfud then
    --spi_flash = spi.deviceSetup(0,8,0,0,8,25600000,spi.MSB,1,0)
    --local ret = sfud.init(spi_flash)
    --if ret then
    --log.info("sfud.init ok")
    --     else
    --log.info("sfud.init error", ret)
    --         return
    --     end
    -- end

    log.info("开始播放TTS")
    local result, data = sys.waitUntil("TTS_msg")
    result = audio.tts(0, data)
    if result then
        --Wait for the callback message of the audio channel, or the message of switching songs
        while true do
            msg = sysplus.waitMsg(taskName, nil)
            if type(msg) == 'table' then
                if msg[1] == MSG_PD then
                    log.info("TTS播放结束")
                    sys.publish("audio_end")
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
    -- sys.wait(1000)
    sysplus.taskDel(taskName)
end
sys.subscribe("Audio_msg",function() sysplus.taskInitEx(audio_task, taskName, task_cb) end)

sys.subscribe("TTS_msg",function() sysplus.taskInitEx(tts_task, taskName, task_cb) end)

