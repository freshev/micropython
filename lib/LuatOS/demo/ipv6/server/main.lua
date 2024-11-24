--[[
IPv6服务端演示, 仅EC618系列支持, 例如Air780E/Air600E/Air780UG/Air700E
]]

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ipv6_server"
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

LED = gpio.setup(27, 0)
GPIO1 = gpio.setup(1, 0)
GPIO24 = gpio.setup(24, 0)
HTTP_200_EMTRY = "HTTP/1.0 200 OK\r\nServer: LuatOS\r\nConnection: close\r\nContent-Length: 0\r\n\r\n"

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
    sys.wait(1000)

    --Print the local IP. Generally, only IPv6 can be a public IP, and IPv4 is basically impossible.
    --Moreover, ipv6 is different from the external network IP. This is decided by the operator, and the Modules cannot do anything about it.
    ip, mask, gw, ipv6 = socket.localIP()
    log.info("本地IP地址", ip, ipv6)
	if not ipv6 then
		log.info("没有IPV6地址，无法演示")
		-- return
	end

    log.info("shell", "telnet -6 " .. ipv6 .. " 14000")


    --Start the current logic, initiate socket connection, wait for data/report heartbeat
    local taskName = "ipv6server"
    local topic = taskName .. "_txrx"
    local txqueue = {}
    sysplus.taskInitEx(ipv6task, taskName, netCB, taskName, txqueue, topic)
    while 1 do
        local result, tp, data = sys.waitUntil(topic, 60000)
        if not result then
            --I've been waiting for a long time. No data has been uploaded/sent. Please send a heartbeat packet with the date.
            --table.insert(txqueue, string.char(0))
            --sys_send(taskName, socket.EVENT, 0)
        elseif tp == "uplink" then
            --Uplink data, data reported proactively, then send it
            table.insert(txqueue, data)
            sys_send(taskName, socket.EVENT, 0)
        elseif tp == "downlink" then
            --Downlink data, received data, comes from ipv6task
            --Other code can be accessed via sys.publish()
            log.info("socket", "收到下发的数据了", #data, data)
            --The following is a simulation of an http service. Because the httpsrv library is not ready yet, let’s use it first.
            if data:startsWith("GET / ") then
                local httpresp = "HTTP/1.0 200 OK\r\n"
                httpresp = httpresp .. "Server: LuatOS\r\nContent-Type: text/html\r\nConnection: close\r\n"
                local fdata = io.readFile("/luadb/index.html")
                httpresp = httpresp .. string.format("Content-Length: %d\r\n\r\n", #fdata)
                httpresp = httpresp .. fdata
                table.insert(txqueue, httpresp)
                table.insert(txqueue, "close")
                sys_send(taskName, socket.EVENT, 0)
            elseif  data:startsWith("GET /led/") then
                if data:startsWith("GET /led/1") then
                    log.info("led", "亮起")
                    LED(1)
                else
                    log.info("led", "熄灭")
                    LED(0)
                end
                table.insert(txqueue, HTTP_200_EMTRY)
                table.insert(txqueue, "close")
                sys_send(taskName, socket.EVENT, 0)
            elseif  data:startsWith("GET /gpio24/") then
                if data:startsWith("GET /gpio24/1") then
                    log.info("gpio24", "亮起")
                    GPIO24(1)
                else
                    log.info("gpio24", "熄灭")
                    GPIO24(0)
                end
                table.insert(txqueue, HTTP_200_EMTRY)
                table.insert(txqueue, "close")
                sys_send(taskName, socket.EVENT, 0)
            elseif  data:startsWith("GET /gpio1/") then
                if data:startsWith("GET /gpio1/1") then
                    log.info("gpio1", "亮起")
                    GPIO1(1)
                else
                    log.info("gpio1", "熄灭")
                    GPIO1(0)
                end
                table.insert(txqueue, HTTP_200_EMTRY)
                table.insert(txqueue, "close")
                sys_send(taskName, socket.EVENT, 0)
            elseif data:startsWith("GET ") or data:startsWith("POST ")  or data:startsWith("HEAD ") then
                table.insert(txqueue, HTTP_200_EMTRY)
                table.insert(txqueue, "close")
                sys_send(taskName, socket.EVENT, 0)
            end
        end
    end
end



function ipv6task(d1Name, txqueue, rxtopic)
    --Local listening port
    local port = 14000


    local rx_buff = zbuff.create(1024)
    local tx_buff = zbuff.create(4 * 1024)
    local netc = socket.create(nil, d1Name)
    socket.config(netc, 14000)
    log.info("任务id", d1Name)

    while true do
        log.info("socket", "开始监控")
        local result = libnet.listen(d1Name, 0, netc)
        if result then
			log.info("socket", "监听成功")
            result = socket.accept(netc, nil)    --Only supports 1 to 1
            if result then
			    log.info("客户端连上了")
            end
        else
            log.info("socket", "监听失败!!")
		end
		while result do
            --log.info("socket", "Call rx to receive data")
			local succ, param = socket.rx(netc, rx_buff)
			if not succ then
				log.info("客户端断开了", succ, param, ip, port)
				break
			end
			if rx_buff:used() > 0 then
				log.info("socket", "收到客户端数据，长度", rx_buff:used())
                local data = rx_buff:query() --Get data
                sys.publish(rxtopic, "downlink", data)
				rx_buff:del()
			end
            --log.info("libnet", "Call wait to start waiting for messages")
			result, param, param2 = libnet.wait(d1Name, 15000, netc)
            log.info("libnet", "wait", result, param, param2)
			if not result then
                --The network is abnormal
				log.info("socket", "客户端断开了", result, param)
				break
            elseif #txqueue > 0 then
                local force_close = false
                while #txqueue > 0 do
                    local data = table.remove(txqueue, 1)
                    if not data then
                        break
                    end
                    log.info("socket", "上行数据长度", #data)
                    if data == "close" then
                        --sys.wait(1000)
                        force_close = true
                        break
                    end
                    tx_buff:del()
                    tx_buff:copy(nil, data)
                    result,param = libnet.tx(d1Name, 15000, netc, tx_buff)
                    log.info("libnet", "发送数据的结果", result, param)
                    if not result then
                        log.info("socket", "数据发送异常", result, param)
                        break
                    end
                end
                if force_close then
                    break
                end
            end
		end
        log.info("socket", "连接已断开,继续下一个循环")
		libnet.close(d1Name, 5000, netc)
		-- log.info(rtos.meminfo("sys"))
		sys.wait(50)
    end
end

sys.taskInit(ipv6test)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!

