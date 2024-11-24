--Host computer source code: https://gitee.com/openLuat/luatos-soc-air105 Written in C#, you can use it, do not produce it
--Compiled tools: https://gitee.com/openLuat/luatos-soc-air105/attach_files
local GC032A_InitReg =
{
	zbar_scan = 0,--Whether it is scanning code, the scanning mode will display grayscale images!!!
    draw_lcd = 0,--Whether to output to LCD, if there is no LCD display, there is no need to set it to 1
    i2c_id = 0,
	i2c_addr = 0x21,
    pwm_id = 5;
    pwm_period  = 24*1000*1000, --The baud rate of the camera clock line
    pwm_pulse = 0,
	sensor_width = 640, --The actual resolution of the GC032A camera is 30w, but there is insufficient memory and the actual display is a 1/4 image in the center.
	sensor_height = 480,
    color_bit = 16, --Color space is RGB565
	init_cmd = "/luadb/GC032A_InitReg.txt"--This method writes the initialization instructions in an external file and supports the use of # for comments.

}

local camera_pwdn = gpio.setup(pin.PD06, 1, gpio.PULLUP) --PD06 camera_pwdn pin
local camera_rst = gpio.setup(pin.PD07, 1, gpio.PULLUP) --PD07 camera_rst pin

usbapp.start(0)

sys.taskInit(function()
	camera_rst(0)
    uart.setup(
        uart.VUART_0,--USB virtual serial port id
        115200,--baud rate
        8,--data bits
        1--Stop bit
    )
	--When taking pictures, naturally RGB output is used.
	local camera_id = camera.init(GC032A_InitReg)--Screen output rgb image

	log.info("摄像头启动")
    camera.video(camera_id, 320, 240, uart.VUART_0)
	log.info("摄像头启动完成")
end)

--The following is the callback for scanning the code. There will be a callback only when zbar_scan=1
camera.on(0, "scanned", function(id, str)
    if type(str) == 'string' then
        log.info("扫码结果", str)
        log.info(rtos.meminfo("sys"))
        --Air105 only takes 200ms to scan each code. When the target 1D code or QR code continues to be recognized, this function will be triggered repeatedly.
        --Since some scenes require interval output, the following code demonstrates interval output.
        --if mcu.ticks() - tick < 1000 then
        --     return
        -- end
        --tick_scan = mcu.ticks()
        --The output content can be output after processing, for example, with line feed (Enter key)
        usbapp.vhid_upload(0, str.."\r\n")
    elseif str == true then
    	log.info("拍照完成")
    elseif str == false then
        log.error("摄像头没有数据")
    end
end)
