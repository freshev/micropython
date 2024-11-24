local libnet = require "libnet"
local GC032A_InitReg =
{
	zbar_scan = 0,--Whether it is scanning code
    draw_lcd = 0,--Whether to output to LCD
    i2c_id = 0,
	i2c_addr = 0x21,
    pwm_id = 5;
    pwm_period  = 24*1000*1000,
    pwm_pulse = 0,
	sensor_width = 640,
	sensor_height = 480,
    color_bit = 16,
	init_cmd = "/luadb/GC032A_InitReg.txt"--This method writes the initialization instructions in an external file and supports the use of # for comments.

}
local MSG_NEW = "DataNew"  --A new frame arrives
local camera_pwdn = gpio.setup(pin.PD06, 1, gpio.PULLUP) --PD06 camera_pwdn pin
local camera_rst = gpio.setup(pin.PD07, 1, gpio.PULLUP) --PD07 camera_rst pin
local taskName = "CAM_TASK"

camera.on(0, "scanned", function(id, str)
    if type(str) == 'number' then   --It means that 1fps has been collected and the processing task can be notified for processing.
        sys_send(taskName, MSG_NEW, str)
        --camera.getRaw(0) --If you don't want to process it and just look at the speed, open this line and the reported speed is 15fps.
    elseif str == true then
    	log.info("拍照完成")
    elseif str == false then
        log.error("摄像头没有数据")
    end
end)


local function netCB(msg)
	log.info("未处理消息", msg[1], msg[2], msg[3], msg[4])
end
--The necessary annotations are the necessary operations for the camera to collect raw data.
local function camTask(ip, port)
	camera_rst(0)  --necessary
	local camera_id = camera.init(GC032A_InitReg) --necessary
    local w,h = 320,240
    local cbuff = zbuff.create(w * h *2)--necessary
    local blen = w * 16 * 2
    local tx_buff = zbuff.create(blen + 16)
	local netc 
	local result, param, is_err, rIP, rPort, vlen, start
    netc = socket.create(socket.ETH0, taskName)
    --socket.debug(netc, true)
    socket.config(netc, nil, true)
    result = libnet.waitLink(taskName, 0, netc)
    camera.startRaw(camera_id, w, h, cbuff)--necessary
    log.info("摄像头启动完成")
    while true do
        result = libnet.waitLink(taskName, 0, netc)
		result = libnet.connect(taskName, 5000, netc, ip, port)
        log.info(result, ip, port)
        while result do
            result = sys_wait(taskName, MSG_NEW, 200)--The message waiting for the collection to be completed is of course not limited to this form.
            if type(result) == 'table' then --After receiving the message that the collection is completed, you can start uploading. No matter which method you use, just upload all the image data in order.
                vlen = 0
                start = 0
                while result and vlen < h do
                    tx_buff:del()
                    tx_buff:pack("<AHHIHH", "VCAM", w, h, vlen, blen, blen) --Add a header to facilitate re-synthesis of images. Of course, you can also define your own protocol, no restrictions.
                    tx_buff:copy(nil, cbuff, start, blen)
                    start = start + blen
                    if (vlen + 16) >= h then
                        camera.getRaw(0)
                    end
                    result = libnet.tx(taskName, 100, netc, tx_buff)
                    vlen = vlen + 16
                end
                log.info("发送完成")
            else
                camera.getRaw(0)
                result = true
            end
        end
        libnet.close(taskName, 5000, netc)
		log.info(rtos.meminfo("sys"))
		sys.wait(1000)
    end
end

function camDemo(ip, port)
	sysplus.taskInitEx(camTask, taskName, netCB, ip, port)
end
