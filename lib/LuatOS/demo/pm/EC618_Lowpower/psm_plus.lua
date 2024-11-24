local server_ip = "112.125.89.8"    --If you use a TCP server, you currently need to disconnect the server before using extreme power consumption mode.
local server_port = 48810 --Replace it with your own
local period = 3* 60* 60 * 1000 --Wake up once every 3 hours

local reason, slp_state = pm.lastReson()  --Get wakeup reason
log.info("wakeup state", pm.lastReson())
local libnet = require "libnet"

local d1Name = "D1_TASK"
local function netCB(msg)
	log.info("未处理消息", msg[1], msg[2], msg[3], msg[4])
end

local function testTask(ip, port)
    local txData
    if reason == 0 then
        txData = "normal wakeup"
    elseif reason == 1 then
        txData = "timer wakeup"
    elseif reason == 2 then
        txData = "pad wakeup"
    elseif reason == 3 then
        txData = "uart1 wakeup"
    end
    if slp_state > 0 then
        mobile.flymode(0,false) --Exit airplane mode. Enter airplane mode before entering psm+. You need to actively exit after waking up.
    end

    gpio.close(32)

	local netc, needBreak
	local result, param, is_err
	netc = socket.create(nil, d1Name)
	socket.debug(netc, false)
	socket.config(netc) --The demo uses a TCP server. Currently, you need to disconnect the server first when using extreme power consumption mode.
    local retry = 0
	while retry < 3 do
		log.info(rtos.meminfo("sys"))
		result = libnet.waitLink(d1Name, 0, netc)
		result = libnet.connect(d1Name, 5000, netc, ip, port)
		if result then
			log.info("服务器连上了")
            result, param = libnet.tx(d1Name, 15000, netc, txData)
			if not result then
				log.info("服务器断开了", result, param)
				break
			else
                needBreak = true
            end
		else
            log.info("服务器连接失败")
        end
		libnet.close(d1Name, 5000, netc)
        retry = retry + 1
        if needBreak then
            break
        end
	end
   
    uart.setup(1, 9600)  --Configure uart1 for external wake-up
    gpio.setup(23,nil)
    gpio.close(35)  --Here the pwrkey is only needed if it is grounded, not if it is controlled by buttons without grounding.
    gpio.setup(32, function() --Configure wakeup interrupt, used for external wake-up
        log.info("gpio")
    end, gpio.PULLUP, gpio.FALLING)
    pm.dtimerStart(3, period)  --Start deep sleep timer
    mobile.flymode(0,true)     
    pm.power(pm.WORK_MODE,3)   --Enter extreme power consumption mode
    log.info(rtos.meminfo("sys"))
    sys.wait(15000)   --The demo shows that the wake-up time is one minute. If the Modules restarts after 15 seconds, it means that it fails to enter the extreme power consumption mode.
    log.info("进入极致功耗模式失败，尝试重启")
    rtos.reboot()
end
sysplus.taskInitEx(testTask, d1Name, netCB, server_ip, server_port)
