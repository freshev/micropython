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
--wdt.init(9000)--Initialize watchdog set to 9s
--sys.timerLoopStart(wdt.feed, 3000)--feed the watchdog once every 3 seconds

sys.taskInit(function()
    sdio.init(0)
    sdio.sd_mount(fatfs.SDIO, "/sd", 0)

    spi_lcd = spi.deviceSetup(0,pin.PB04,0,0,8,20*1000*1000,spi.MSB,1,1)
    lcd.init("st7735s",{port = "device",pin_dc = pin.PB01, pin_pwr = pin.PB00, pin_rst = pin.PB03,direction = 2,w = 160,h = 80,xoffset = 1,yoffset = 26},spi_lcd)

    --Use ffmpeg.exe to convert the video into a byte stream file sxd.rgb and put it into the TF card
    --Scale to target size first
    --ffmpeg -i sxd.mp4 -vf scale=160:80 sxd.avi
    --Then convert rbg565ble byte stream
    --ffmpeg -i sxd.avi -pix_fmt rgb565be -vcodec rawvideo sxd.rgb

    --Use ffmpeg.exe to convert the video into a byte stream file .rgb and put it into the TF card. For a tutorial, see https://wiki.luatos.com/appDevelopment/video_play/105/video_play.html
    local video_w = 160
    local video_h = 80
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
    while true do
        sys.wait(1000)
    end
end)

--Main loop, must be added
sys.run()
