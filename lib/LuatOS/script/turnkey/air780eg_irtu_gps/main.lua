--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "irtu_gnss"
VERSION = "1.0.1"

--[[
这个demo名字含irtu,实际上没有使用iRTU.

网站是: https://gps.nutz.cn/ 连上之后,输入IMEI并回车,就能看到当前坐标
小程序的名字: iRTU寻物 打开后扫码模块上的二维码,就能显示当前位置和历史轨迹
服务器源码: https://gitee.com/wendal/irtu-gps 里面的硬件和配置描述是Air800的,可以无视.

本demo需要Air780EG及V1003固件, 2023-1-11之后编译的版本

所需要的库文件,在 script/libs里面, 全部加入就可以了
]]

--sys library is standard
local sys = require("sys")
local sysplus = require("sysplus")
local libnet = require("libnet")

local gps_uart_id = 2

--libgnss library initialization
libgnss.clear() --Clear data and initialize

--GNSS serial port initialization
uart.setup(gps_uart_id, 115200)

function exec_agnss()
    local url = "http://download.openluat.com/9501-xingli/HXXT_GPS_BDS_AGNSS_DATA.dat"
    if http then
        --AGNSS has been tuned
        while 1 do
            local code, headers, body = http.request("GET", url).wait()
            log.info("gnss", "AGNSS", code, body and #body or 0)
            if code == 200 and body and #body > 1024 then
                for offset = 1, #body, 512 do
                    log.info("gnss", "AGNSS", "write >>>", #body:sub(offset, offset + 511))
                    uart.write(gps_uart_id, body:sub(offset, offset + 511))
                    sys.wait(100) --Waiting for 100ms will be more successful
                end
                --sys.waitUntil("UART2_SEND", 1000)
                io.writeFile("/6228.bin", body)
                break
            end
            sys.wait(60 * 1000)
        end
    end
    sys.wait(20)
    -- "$AIDTIME,year,month,day,hour,minute,second,millisecond"
    local date = os.date("!*t")
    if date.year > 2022 then
        local str = string.format("$AIDTIME,%d,%d,%d,%d,%d,%d,000", date["year"], date["month"], date["day"],
            date["hour"], date["min"], date["sec"])
        log.info("gnss", str)
        uart.write(gps_uart_id, str .. "\r\n")
        sys.wait(20)
    end
    --Read previous location information
    local gnssloc = io.readFile("/gnssloc")
    if gnssloc then
        str = "$AIDPOS," .. gnssloc
        log.info("POS", str)
        uart.write(gps_uart_id, str .. "\r\n")
        str = nil
        gnssloc = nil
    else
        uart.write(gps_uart_id, "$AIDPOS,3432.70,N,10885.25,E,1.0\r\n")
    end
end

sys.taskInit(function()
    --The default baud rate of Air780EG is 115200
    log.info("GPS", "start")
    pm.power(pm.GPS, true)
    --Debug log, optional
    libgnss.debug(true)
    sys.wait(200) --It takes time for the GPNSS chip to start, about 150ms
    --Add displayed statements, optional
    uart.write(gps_uart_id, "$CFGMSG,0,1,1\r\n") -- GLL
    sys.wait(20)
    uart.write(gps_uart_id, "$CFGMSG,0,5,1\r\n") -- VTG
    sys.wait(20)
    uart.write(gps_uart_id, "$CFGMSG,0,6,1\r\n") -- ZDA
    sys.wait(20)
    --After successful positioning, use GNSS time to set RTC
    libgnss.rtcAuto(true)
    
    --Bind uart, the bottom layer automatically processes GNSS data
    libgnss.bind(gps_uart_id)
    exec_agnss()
end)

--Simply print the positioning information regularly
sys.taskInit(function()
    while 1 do
        sys.wait(5000)
        log.info("RMC", json.encode(libgnss.getRmc(4) or {}))
    end
end)

--Subscribe to GNSS status encoding
sys.subscribe("GNSS_STATE", function(event, ticks)
    --The event value is
    --FIXED Positioning successful
    --LOSE positioning lost
    --Ticks is the time when the event occurs and can generally be ignored.
    log.info("gnss", "state", event, ticks)
    if event == "FIXED" then
        local locStr = libgnss.locStr()
        log.info("gnss", "locStr", locStr)
        if locStr then
            --Save the file to facilitate quick positioning of AGNSS next time
            io.writeFile("/gnssloc", locStr)
        end
    end
end)

--docking server
local taskName = "gnsstask"
function gnsstask()
    local tx_buff = zbuff.create(1024)
	local rx_buff = zbuff.create(1024)
	local netc 
	local result, param, succ
    local host, port = "gps.nutz.cn", 19002
    local netc = socket.create(nil, taskName)
	--socket.debug(netc, true)
	socket.config(netc, nil, nil, nil)
    while 1 do
        result = libnet.waitLink(taskName, 0, netc)
		result = libnet.connect(taskName, 15000, netc, host, port)
        local jdata = {
            imei = mobile.imei(),
            iccid = mobile.iccid()
        }
        if result then
            local data = json.encode(jdata)
			log.info("服务器连上了", "上报注册注册信息", data)
			libnet.tx(taskName, 0, netc, data)
		end
        while result do
			succ, param = socket.rx(netc, rx_buff)
			if not succ then
				log.info("服务器断开了", succ, param, host, port)
				break
			end
			if rx_buff:used() > 0 then
				log.info("收到服务器数据，长度", rx_buff:used())
				rx_buff:del()
			end
			if rx_buff:len() > 1024 then
				rx_buff:resize(1024)
			end
			log.info("sys", rtos.meminfo("sys"))
			result, param = libnet.wait(taskName, 1000, netc)
			if not result then
				log.info("服务器断开了", result, param)
				break
			end

            --Send packet
            local msg = {libgnss.isFix(), os.time()}
            local rmc = libgnss.getRmc(1)
            local gsa = libgnss.getGsa()
            local vtg = libgnss.getVtg()
            table.insert(msg, rmc.lng)
            table.insert(msg, rmc.lat)
            local gll = libgnss.getGll()
            table.insert(msg, 0) -- altitude
            table.insert(msg, 0) -- azimuth
            table.insert(msg, (vtg and vtg.speed_kph) and vtg.speed_kph or 0) -- speed
            table.insert(msg, 0) -- sateCno
            table.insert(msg, 0) -- sateCnt
            --jdata.msg = msg
            local data = json.encode({msg=msg})
            log.info("report", data)
            if data then
                libnet.tx(taskName, 0, netc, data)
            end
		end
		d1Online = false
		libnet.close(taskName, 5000, netc)
		log.info("sys", rtos.meminfo("sys"))
		sys.wait(1000)
    end
end

local function netCB(msg)
	log.info("未处理消息", msg[1], msg[2], msg[3], msg[4])
end
sysplus.taskInitEx(gnsstask, "gnsstask", netCB)

sys.taskInit(function()
    while 1 do
        sys.wait(3600 * 1000) --Check once an hour
        local fixed, time_fixed = libgnss.isFix()
        if not fixed then
            exec_agnss()
        end
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
