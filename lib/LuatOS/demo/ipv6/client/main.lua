--[[
IPv6客户端演示, 仅EC618系列支持, 例如Air780E/Air600E/Air780UG/Air700E
]]

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ipv6_client"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")
sysplus = require("sysplus")
libnet = require "libnet"


--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

--Handling unrecognized network messages
local function netCB(msg)
	log.info("未处理消息", msg[1], msg[2], msg[3], msg[4])
end

--Demo task
function ipv6test()
    --Only supported by EC618 series, such as Air780E/Air600E/Air780UG/Air700E
    if rtos.bsp() ~= "EC618" and rtos.bsp() ~= "EC718P" then
        while 1 do
            log.info("ipv6", "only Air780E/Air600E/Air780UG/Air700E/Air780EP supported")
            sys.wait(1000)
        end
    end

    --Enable IPv6. It is turned off by default and must be turned on before setting up the network.
    --Note that if IPv6 is enabled, the Internet speed will be 2~3 seconds slower.
    mobile.ipv6(true)

    log.info("ipv6", "等待联网")
    sys.waitUntil("IP_READY")
    log.info("ipv6", "联网完成")
    sys.wait(100)

    socket.setDNS(nil, 1, "119.29.29.29")
    socket.setDNS(nil, 2, "114.114.114.114")

    --Start the current logic, initiate socket connection, wait for data/report heartbeat
    local taskName = "ipv6client"
    local topic = taskName .. "_txrx"
    local txqueue = {}
    sysplus.taskInitEx(ipv6task, taskName, netCB, taskName, txqueue, topic)
    while 1 do
        local result, tp, data = sys.waitUntil(topic, 30000)
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



function ipv6task(d1Name, txqueue, rxtopic)
    --Test method 1, connected to netlab external network version, with ipv6
    --Note that you need to log in to the netlab on the external network to have ipv6.
    --Website link: https://netlab.luatos.org/
    --local host = "2603:c023:1:5fcc:c028:8ed:49a7:6e08"
    --local port = 55389 -- Click "Open TCP" on the page to get the actual port

    --Test method 2, connect another 780e device
    local host = "864040064024194.dyndns.u8g2.com"
    --local host = "mirrors6.tuna.tsinghua.edu.cn"
    --local host = "2408:8456:e37:95d8::1"
    local port = 14000

    local rx_buff = zbuff.create(1024)
    local netc = socket.create(nil, d1Name)
    socket.config(netc)
    log.info("任务id", d1Name)

    while true do
        log.info("socket", "开始连接服务器")
        local result = libnet.connect(d1Name, 15000, netc, host, port, true)
        if result then
			log.info("socket", "服务器连上了")
			libnet.tx(d1Name, 0, netc, "helloworld")
        else
            log.info("socket", "服务器没连上了!!!")
		end
		while result do
            --log.info("socket", "Call rx to receive data")
			local succ, param = socket.rx(netc, rx_buff)
			if not succ then
				log.info("服务器断开了", succ, param, ip, port)
				break
			end
			if rx_buff:used() > 0 then
				log.info("socket", "收到服务器数据，长度", rx_buff:used())
                local data = rx_buff:query() --Get data
                sys.publish(rxtopic, "downlink", data)
				rx_buff:del()
			end
            --log.info("libnet", "Call wait to start waiting for messages")
			result, param, param2 = libnet.wait(d1Name, 15000, netc)
            log.info("libnet", "wait", result, param, param2)
			if not result then
                --The network is abnormal
				log.info("socket", "服务器断开了", result, param)
				break
            elseif #txqueue > 0 then
                while #txqueue > 0 do
                    local data = table.remove(txqueue, 1)
                    if not data then
                        break
                    end
                    result,param = libnet.tx(d1Name, 15000, netc,data)
                    log.info("libnet", "发送数据的结果", result, param)
                    if not result then
                        log.info("socket", "数据发送异常", result, param)
                        break
                    end
                end
            end
		end
		libnet.close(d1Name, 5000, netc)
		-- log.info(rtos.meminfo("sys"))
		sys.wait(5000)
    end
end

sys.taskInit(ipv6test)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!

