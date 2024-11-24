--[[
连到gps.nutz.cn 19002 端口, irtu的自定义包格式
]]

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "scdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")
sysplus = require("sysplus")
libnet = require "libnet"

if pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

adc.open(adc.CH_VBAT)

--=============================================================
--This TCP demonstration is connected to gps.nutz.cn port 19002, irtu's custom packet format
--The webpage is https://gps.nutz.cn/. Enter the IMEI number to refer to the current location.
--The WeChat applet is irtu. Click on the IMEI number and scan the QR code of the Modules to view the current location and historical track.
local host = "gps.nutz.cn"  --Server IP or domain name, both are acceptable
local port = 19002          --Server port number
local is_udp = false        --If it is UDP, change it to true, false is TCP.
local is_tls = false        --Whether to encrypt or not depends on the actual situation of the server.
--=============================================================

--Handling unrecognized network messages
local function netCB(msg)
	log.info("未处理消息", msg[1], msg[2], msg[3], msg[4])
end

local socket_ready = false
local taskName = "sc"
local topic = taskName .. "_txrx"
log.info("socket", "event topic", topic)

--Demo task
local function sockettest()
    sys.waitUntil("IP_READY")

    --Start the current logic, initiate socket connection, wait for data/report heartbeat
    local txqueue = {}
    sysplus.taskInitEx(sockettask, taskName, netCB, taskName, txqueue, topic)
    while 1 do
        local result, tp, data = sys.waitUntil(topic, 30000)
        --log.info("event", result, tp, data)
        if not result then
            --I've been waiting for a long time. No data has been uploaded/sent. Please send a heartbeat packet with the date.
            table.insert(txqueue, string.char(0))
            sys_send(taskName, socket.EVENT, 0)
        elseif tp == "uplink" then
            --Uplink data, data reported proactively, then send it
            table.insert(txqueue, data)
            sys_send(taskName, socket.EVENT, 0)
        elseif tp == "downlink" then
            --Downlink data, received data, comes from ipv6task
            --Other code can be accessed via sys.publish()
            log.info("socket", "收到下发的数据了", #data)
        end
    end
end



function sockettask(d1Name, txqueue, rxtopic)
    --Print server information ready to connect
    log.info("socket", host, port, is_udp and "UDP" or "TCP", is_tls and "TLS" or "RAW")

    --Prepare the required receive buffer
    local rx_buff = zbuff.create(1024)
    local netc = socket.create(nil, d1Name)
    socket.config(netc, nil, is_udp, is_tls)
    log.info("任务id", d1Name)

    while true do
        --Connect to server, 15 seconds timeout
        log.info("socket", "开始连接服务器")
        sysplus.cleanMsg(d1Name)
        local result = libnet.connect(d1Name, 15000, netc, host, port)
        if result then
			log.info("socket", "服务器连上了")
            local tmp = {imei=mobile.imei(),iccid=mobile.iccid()}
			libnet.tx(d1Name, 0, netc, json.encode(tmp))
            socket_ready = true
        else
            log.info("socket", "服务器没连上了!!!")
		end
		while result do
            --After the connection is successful, first try to receive
            --log.info("socket", "Call rx to receive data")
			local succ, param = socket.rx(netc, rx_buff)
			if not succ then
				log.info("服务器断开了", succ, param, ip, port)
				break
			end
            --If the server has sent data, used() must be greater than 0 and will be processed.
			if rx_buff:used() > 0 then
				log.info("socket", "收到服务器数据，长度", rx_buff:used())
                local data = rx_buff:query() --Get data
                sys.publish(rxtopic, "downlink", data)
				rx_buff:del()
			end
            --log.info("libnet", "Call wait to start waiting for messages")
            --Waiting for events, for example: the server delivers data, data is ready to be reported, the server disconnects
			result, param, param2 = libnet.wait(d1Name, 15000, netc)
            log.info("libnet", "wait", result, param, param2)
			if not result then
                --If the network is abnormal, disconnect it and perform cleanup work.
				log.info("socket", "服务器断开了", result, param)
				break
            elseif #txqueue > 0 then
                --The data to be reported is processed
                while #txqueue > 0 do
                    local data = table.remove(txqueue, 1)
                    if not data then
                        break
                    end
                    result,param = libnet.tx(d1Name, 15000, netc,data)
                    --log.info("libnet", "Result of sending data", result, param)
                    if not result then
                        log.info("socket", "数据发送异常", result, param)
                        break
                    end
                end
            end
            --At the end of the loop, continue to the next cycle
		end
        socket_ready = false
        --To get here, either the server is disconnected, or the reporting (tx) fails, or it actively exits.
		libnet.close(d1Name, 5000, netc)
		-- log.info(rtos.meminfo("sys"))
		sys.wait(3000) --This is the reconnection time, adjust it yourself
    end
end

sys.taskInit(sockettest)


sys.taskInit(function()
    sys.waitUntil("IP_READY")
    local stat_t = 0
    local buff = zbuff.create(64)
    while true do
        if socket_ready then
            --Send device status >b7IHb == 1*7+4+2+1 = 14
            if os.time() - stat_t > 30 then
                --Report once every 30 seconds
                local vbat = adc.get(adc.CH_VBAT)
                buff:seek(0)
                buff:pack(">b7IHb", 0x55, 0, 0, 0, 0, 0, 0, 0, vbat, mobile.csq())
                sys.publish(topic, "uplink", buff:query())
                stat_t = os.time()
                sys.wait(100)
            end
            --Send location information >b2i3H2b3 == 1*2+4*3+2*2+1*3 == 2+12+4+3 = 21
            if true then
                local rmc = libgnss.getRmc(1)
                local gsa = libgnss.getGsa()
                local gsv = libgnss.getGsv()
                --log.info("socket", "rmc", rmc.lat, rmc.lng, rmc.alt, rmc.course, rmc.speed)
                buff:seek(0)
                buff:pack(">b2i3H2b3", 0xAA, libgnss.isFix() and 1 or 0,
                        os.time(),
                        rmc and rmc.lng or 0,
                        rmc and rmc.lat or 0,
                        0, --rmc and rmc.alt or 0,
                        math.floor(rmc and rmc.course or 0),
                        math.floor(rmc and rmc.speed or 0),
                        gsa and #gsa.sats or 0, -- msg.sateCno
                        gsv and gsv.total_sats or 0 -- msg.sateCnt
                )
                sys.publish(topic, "uplink", buff:query())
            end
            sys.wait(1000)
        else
            sys.wait(100)
        end
    end
end)
