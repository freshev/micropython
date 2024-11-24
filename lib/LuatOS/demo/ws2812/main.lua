--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ws2812demo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")
--[[

注意,使用前先看本注释每一句话！！！！！！！
注意,使用前先看本注释每一句话！！！！！！！
注意,使用前先看本注释每一句话！！！！！！！

说明：ws2812在Cat.1模组上挂载，如果在网络环境下使用会有干扰，
因为网络优先级是最高的，会导致时序干扰造成某个灯珠颜色异常，效果不是很好，
不推荐使用。如果认为影响较大，建议通过外挂MCU实现。

注意,使用前先看本注释！！！！！！！！！！！
注意,使用前先看本注释！！！！！！！！！！！
注意,使用前先看本注释！！！！！！！！！！！

]]

local rtos_bsp = rtos.bsp():lower()
if rtos_bsp=="air101" or rtos_bsp=="air103" then
    mcu.setClk(240)
end

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

--Optional pwm, gpio, spi mode driver, see wiki for API details https://wiki.luatos.com/api/sensor.html

--mode pin/pwm_id/spi_id T0H T0L T1H T1L
local function ws2812_conf()
    local rtos_bsp = rtos.bsp()
    if rtos_bsp == "AIR101" then
        return "pin",pin.PA7,0,20,20,0      --This is pin direct drive, please note that the main frequency of air101 is set to 240
    elseif rtos_bsp == "AIR103" then
        return "pin",pin.PA7,0,20,20,0      --This is a pin direct drive, please note that the main frequency of air103 is set to 240
    elseif rtos_bsp == "AIR105" then
        return "pin",pin.PD13,0,10,10,0     --This is pin direct drive
    elseif rtos_bsp == "ESP32C3" then
        return "pin",2,0,10,10,0            --This is pin direct drive
    elseif rtos_bsp == "ESP32S3" then
        return "pin",2,0,10,10,0            --This is pin direct drive
    elseif rtos_bsp == "EC618" then
        return "pin",24,10,0,10,0           --This is pin mode direct drive (requires firmware compiled after 2023.7.25, otherwise only spi mode can be used)
    elseif rtos_bsp == "EC718P" then
        return "pin",29,12,25,50,30
    else
        log.info("main", "bsp not support")
        return
    end
end

local show_520 = {
    {0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff},
    {0x0000ff,0x00ff00,0x00ff00,0x0000ff,0x0000ff,0x00ff00,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff},
    {0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff},
    {0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x00ff00,0x0000ff,0x00ff00,0x0000ff,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff},
    {0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x0000ff,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff},
    {0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x00ff00,0x0000ff,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x00ff00,0x0000ff,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff},
    {0x0000ff,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x00ff00,0x00ff00,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff},
    {0x0000ff,0x0000ff,0x0000ff,0x00ff00,0x00ff00,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff,0x0000ff},
}
local show_520_w = 24
local show_520_h = 8

local ws2812_w = 8
local ws2812_h = 8
local buff = zbuff.create({ws2812_w,ws2812_h,24},0x000000)

local function ws2812_roll_show(show_data,data_w)
    local m = 0
    while 1 do
        for j=0,ws2812_w-1 do
            if j%2==0 then
                for i=ws2812_w-1,0,-1 do
                    if m+ws2812_w-i>data_w then
                        buff:pixel(i,j,show_data[j+1][m+ws2812_w-i-data_w])
                    else
                        buff:pixel(i,j,show_data[j+1][m+ws2812_w-i])
                    end
                end
            else
                for i=0,ws2812_w-1 do
                    if m+i+1>data_w then
                        buff:pixel(i,j,show_data[j+1][m+i+1-data_w])
                    else
                        buff:pixel(i,j,show_data[j+1][m+i+1])
                    end
                end
            end
        end
        m = m+1
        if m==data_w then m=0 end

        --Optional pwm, gpio, spi mode driver, see wiki for API details https://wiki.luatos.com/api/sensor.html
        local mode = ws2812_conf()
        if mode == "pin" then
            local _,pin,T0H,T0L,T1H,T1L = ws2812_conf()
            sensor.ws2812b(pin,buff,T0H,T0L,T1H,T1L)
        elseif mode == "pwm" then
            local _,pwm_id = ws2812_conf()
            sensor.ws2812b_pwm(pwm_id,buff)
        elseif mode == "spi" then
            local _,spi_id = ws2812_conf()
            sensor.ws2812b_spi(spi_id,buff)
        else
            while 1 do
                sys.wait(1000)
                log.info("main", "bsp not support yet")
            end
        end
        sys.wait(300)
    end
end
sys.taskInit(function()
    sys.wait(500)
    ws2812_roll_show(show_520,show_520_w)
end)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
