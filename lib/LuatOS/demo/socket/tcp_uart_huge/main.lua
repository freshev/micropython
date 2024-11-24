--[[
socket客户端演示

包括但不限于以下模组
1. EC618系列 -- Air780E/Air780EG/Air600E/Air700E
2. ESP32系列 -- ESP32C3/ESP32S3/ESP32C2
3. Air105搭配W5500
4. 其他适配了socket层的bsp

支持的协议有: TCP/UDP/TLS-TCP/DTLS, 更高层级的协议,如http有单独的库

提示: 
1. socket支持多个连接的, 通常最多支持8个, 可通过不同的taskName进行区分
2. 支持与http/mqtt/websocket/ftp库同时使用, 互不干扰
3. 支持IP和域名, 域名是自动解析的, 但解析域名也需要耗时
4. 加密连接(TLS/SSL)需要更多内存, 这意味着能容纳的连接数会小很多, 同时也更慢

对于多个网络出口的场景, 例如Air780E+W5500组成4G+以太网:
1. 在socket.create函数设置网络适配器的id
2. 请到同级目录查阅更细致的demo

如需使用ipv6, 请查阅 demo/ipv6, 本demo只涉及ipv4
]]

--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "scdemo"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")
sysplus = require("sysplus")
libnet = require "libnet"

local uart_id = 1

--The AT firmware of Air780E will anti-shake the power button by default, which makes flashing the phone troublesome for some users.
if rtos.bsp() == "EC618" and pm and pm.PWK_MODE then
    pm.power(pm.PWK_MODE, false)
end

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end


--=============================================================
--Test website https://netlab.luatos.com/ Click to open TCP to obtain the test port number
--To be modified according to the actual situation
local host = "81.70.22.216" --Server IP or domain name, both are acceptable
local port = 9999          --Server port number
local is_udp = false        --If it is UDP, change it to true, false is TCP.
local is_tls = false        --Whether to encrypt or not depends on the actual situation of the server.
--=============================================================

--Handling unrecognized network messages
local function netCB(msg)
	--log.info("Unprocessed message", msg[1], msg[2], msg[3], msg[4])
end


uart.setup(uart_id, 115200) --Note, it is UART1, not a virtual serial port, for demonstration purposes
uart.on(uart_id, "sent", function(id, len)
    log.info("sent", id, len)
    --local start = mcu.ticks()
    uart.wait485(uart_id)
    --local done = mcu.ticks()
    --local used = done - start
    --log.info("time use", used)
    sys.publish("uart_tx_done")
end)

--Unified networking functions
sys.taskInit(function()
    -----------------------------
    --Unified networking functions, which can be deleted by yourself
    ----------------------------
    if wlan and wlan.connect then
        --WiFi networking is supported by the ESP32 series. Please modify the ssid and password according to the actual situation!!
        local ssid = "luatos1234"
        local password = "12341234"
        log.info("wifi", ssid, password)
        --TODO Change to automatic network distribution
        wlan.init()
        wlan.setMode(wlan.STATION) --This is also the mode by default, and you can do it without calling it.
        wlan.connect(ssid, password, 1)
    elseif mobile then
        --EC618 series, such as Air780E/Air600E/Air700E
        --mobile.simid(2) -- Automatically switch SIM cards, enable on demand
        --The Modules will automatically connect to the Internet by default, no additional operations are required.
    elseif w5500 then
        --w5500 Ethernet
        w5500.init(spi.HSPI_0, 24000000, pin.PC14, pin.PC01, pin.PC00)
        w5500.config() --The default is DHCP mode
        w5500.bind(socket.ETH0)
    elseif socket then
        --It’s OK if it’s adapted to the socket library. Just treat it as 1 second to connect to the Internet.
        sys.timerStart(sys.publish, 1000, "IP_READY")
    else
        --For other bsps that I don’t know, let me give you some hints in a loop.
        while 1 do
            sys.wait(1000)
            log.info("bsp", "本bsp可能未适配网络层, 请查证")
        end
    end
    --By default, it waits until the connection is successful
    sys.waitUntil("IP_READY")
    sys.publish("net_ready")
end)

--Demo task
local function sockettest()
    --Waiting for Internet connection
    sys.waitUntil("net_ready")

    socket.sntp()

    --Start the current logic, initiate socket connection, wait for data/report heartbeat
    local taskName = "sc"
    local topic = taskName .. "_txrx"
    log.info("topic", topic)
    local txqueue = {}
    sysplus.taskInitEx(sockettask, taskName, netCB, taskName, txqueue, topic)
    while 1 do
        local result, tp, data = sys.waitUntil(topic, 30000)
        --log.info("event", result, tp, data)
        if not result then
            --I've been waiting for a long time. No data has been uploaded/sent. Please send a heartbeat packet with the date.
            --table.insert(txqueue, string.char(0))
            --sys_send(taskName, socket.EVENT, 0)
        elseif tp == "uplink" then
            --Uplink data, data reported proactively, then send it
            --table.insert(txqueue, data)
            --sys_send(taskName, socket.EVENT, 0)
        elseif tp == "downlink" then
            --Downlink data, received data, comes from ipv6task
            --Other code can be accessed via sys.publish()
            log.info("socket", "收到下发的数据了", #data)
            uart.write(uart_id, data)
        end
    end
end



function sockettask(d1Name, txqueue, rxtopic)
    --Print server information ready to connect
    log.info("socket", host, port, is_udp and "UDP" or "TCP", is_tls and "TLS" or "RAW")

    --Prepare the required receive buffer
    local rx_buff = zbuff.create(10*1024)
    local netc = socket.create(nil, d1Name)
    socket.config(netc, nil, is_udp, is_tls)
    log.info("任务id", d1Name)
    while true do
        local data_counter = 0
        --Connect to server, 15 seconds timeout
        log.info("socket", "开始连接服务器")
        sysplus.cleanMsg(d1Name)
        local result = libnet.connect(d1Name, 15000, netc, host, port)
        if result then
			log.info("socket", "服务器连上了")
			--libnet.tx(d1Name, 0, netc, "helloworld")
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
			while rx_buff:used() > 0 do
				log.info("socket", "收到服务器数据，长度", rx_buff:used())
                data_counter = data_counter + rx_buff:used()
                --local data = rx_buff:query() -- Get data
                uart.tx(uart_id, rx_buff)
                log.info("数据统计", "累计接收字节数", data_counter)
                log.info("lua", rtos.meminfo())
                --log.info("sys", rtos.meminfo("sys"))
                if sys.waitUntil("uart_tx_done") then
                    --sys.wait(10) -- wait another 10ms
                end
				rx_buff:del()
                sysplus.cleanMsg(d1Name)
                socket.rx(netc, rx_buff)
			end
            --log.info("libnet", "Call wait to start waiting for messages")
            --Waiting for events, for example: the server delivers data, data is ready to be reported, the server disconnects
			result, param, param2 = libnet.wait(d1Name, 500, netc)
            --log.info("libnet", "wait", result, param, param2)
			if not result then
                --If the network is abnormal, disconnect it and perform cleanup work.
				log.info("socket", "服务器断开了", result, param)
				break
            end
            if #txqueue > 0 then
                --The data to be reported is processed
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
            --At the end of the loop, continue to the next cycle
		end
        --To get here, either the server is disconnected, or the reporting (tx) fails, or it actively exits.
		libnet.close(d1Name, 5000, netc)
		-- log.info(rtos.meminfo("sys"))
		sys.wait(300000) --This is the reconnection time, adjust it yourself
    end
end

sys.taskInit(sockettest)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!

