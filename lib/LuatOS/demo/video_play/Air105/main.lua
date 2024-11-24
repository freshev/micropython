--- Module function: video_play_demo
--@Modules video_play
--@author Dozingfiretruck
--@release 2021.09.06

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "video_play_demo"
VERSION = "1.0.1"

log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

sys.taskInit(function()
    --initialize lcd
    spi_lcd = spi.deviceSetup(5,pin.PC14,0,0,8,48*1000*1000,spi.MSB,1,1)
    log.info("lcd.init",
    lcd.init("st7735",{port = "device",pin_dc = pin.PE08 ,pin_rst = pin.PC12,pin_pwr = pin.PE09,direction = 3,w = 160,h = 128,xoffset = 1,yoffset = 2},spi_lcd))
    --initialize sd
    local spiId = 2
    local result = spi.setup(
        spiId,--serial port id
        255, --Do not use the default CS pin
        0,--CPHA
        0,--CPOL
        8,--data width
        400*1000  --Use lower frequency when initializing
    )
    local TF_CS = pin.PB3
    gpio.setup(TF_CS, 1)
    --fatfs.debug(1) -- If the mount fails, you can try to turn on debugging information to find out the reason.
    fatfs.mount(fatfs.SPI,"/sd", spiId, TF_CS, 24000000)
    local data, err = fatfs.getfree("SD")
    if data then
        log.info("fatfs", "getfree", json.encode(data))
    else
        log.info("fatfs", "err", err)
    end
    
    --Use ffmpeg.exe to convert the video into a byte stream file .rgb and put it into the TF card. For a tutorial, see https://wiki.luatos.com/appDevelopment/video_play/105/video_play.html
    local video_w = 160
    local video_h = 128
    local rgb_file = "mwsy.rgb"

    local buff_size = video_w*video_h*2
    local file_size = fs.fsize("/sd/"..rgb_file)
    print("/sd/"..rgb_file.." file_size",file_size)
    
    local file = io.open("/sd/"..rgb_file, "rb")
    if file then
        local file_cnt = 0
        local buff = zbuff.create(buff_size)
        repeat
            if file:fill(buff) then
                file_cnt = file_cnt + buff_size
                lcd.draw(0, 0, video_w-1, video_h-1, buff)
                sys.wait(20)
            end
        until( file_size - file_cnt < buff_size )
        local temp_data = file:fill(buff,0,file_size - file_cnt)
        lcd.draw(0, 0, video_w-1, video_h-1, buff)
        sys.wait(30)
        file:close()
    end
end)

--Main loop, must be added
sys.run()
