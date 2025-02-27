
local lbsLoc2 = require "lbsLoc2"

local hdgnss = {}

function hdcrc(str, len)
    if not len then
        len = #str
    end
    local crc1 = 0
    local crc2 = 0
    for i = 3, len do
        crc1 = crc1 + str:byte(i)
        crc2 = crc2 + (crc1 & 0xff)
    end
    --print(string.format("%02X%02X", crc1 & 0xFF, crc2 & 0xFF))
    return string.char(crc1 & 0xFF, crc2 & 0xFF)
end

function hdgnss.aideph()
    local url = "http://download.openluat.com/9501-xingli/HD_GPS_BDS.hdb"
    local eph_body = nil
    if io.fileSize("/hdgnss.bin") > 1024 then
        local date = os.date("!*t")
        --log.info("gnss", "Current system time", os.date())
        if date.year < 2024 then
            date = os.date("!*t")
        end
        if date.year > 2023 then
            local tm = io.readFile("/hdgnss_tm")
            if tm then
                local t = tonumber(tm)
                if t and (os.time() - t < 3600*2) then
                    log.info("hdgnss", "重用星历文件")
                    eph_body = io.readFile("/hdgnss.bin")
                else
                    log.info("hdgnss", "星历过期了")
                    os.remove("/hdgnss.bin")
                end
            else
                log.info("hdgnss", "星历时间有问题")
            end
        else
            log.info("hdgnss", "系统时间有问题")
        end
    end
    if http and not eph_body then
        --AGNSS has been tuned
        for i = 1, 3 do
            local code, _, body = http.request("GET", url, nil, nil, {timeout=3000}).wait()
            log.info("hdgnss", "AGNSS", code, body and #body or 0)
            if code == 200 and body and #body > 1024 then
                io.writeFile("/hdgnss.bin", body)
                sys.wait(1000)
                local date = os.date("!*t")
                if date.year > 2022 then
                    io.writeFile("/hdgnss_tm", tostring(os.time()))
                end
                eph_body = body
                break
            end
            sys.wait(6 * 1000)
        end
    end
    if eph_body and #eph_body > 1024 then
        local body = eph_body
        --for offset = 1, #body, 512 do
        ---- log.info("hdgnss", "AGNSS", "write >>>", #body:sub(offset, offset + 511))
        ---- uart.write(gps_uart_id, body:sub(offset, offset + 511))
        ---- sys.waitUntil("UART2_SEND", 100)
        --sys.wait(100) -- Waiting for 100ms will be more successful
        -- end
        --uart.write(gps_uart_id, body)
        local offset = 1
        local size = #body
        while offset < size do
            --The first two characters are F1D9
            if body:byte(offset) == 0xF1 and body:byte(offset + 1) == 0xD9 then
                --log.info("hdgnss", "ephemeris data", body:sub(1, 16):toHex())
                local len = body:byte(offset + 4) + body:byte(offset + 5) * 256
                local tmp = body:sub(offset, offset + len + 7)
                --log.info("hdgnss", "Write ephemeris fragment", offset, len, #tmp, tmp:toHex())
                uart.write(gps_uart_id, tmp)
                offset = offset + len + 8
                sys.wait(20)
            else
                log.warn("hdgnss", "分隔出错了", offset, size)
                break
            end
        end
    end
    sys.wait(20)
end

local function exec_agnss()
    for i = 1, 3 do
        local ip = socket.localIP()
        if ip and ip ~= "0.0.0.0" then
            break
        end
        sys.waitUntil("IP_READY", 3000)
    end

    socket.sntp()
    sys.waitUntil("NTP_UPDATE", 1000)

    --Inject the current time
    if os.date("*t").year > 2023 then
        hdgnss.aidtime()
        sys.wait(100)
    end

    --Read previous location information
    local lat, lng
    local gnssloc = io.readFile("/hdloc")
    if gnssloc then
        log.info("hdgnss", "最后已知位置信息", gnssloc)
        local tmp = gnssloc:split(",")
        if tmp and #tmp == 2 then
            lat = tonumber(tmp[1])
            lng = tonumber(tmp[2])
        end
    end

    --Write reference position
    if lat and lng and lat ~= 0 and lng ~= 0 then
        hdgnss.aidpos(lat, lng)
        sys.wait(100)
    else
        log.info("hdgnss", "当前无辅助位置信息")
    end

    --Initiate base station positioning, temporarily disabled
    if mobile and false then
        mobile.reqCellInfo(6)
        sys.waitUntil("CELL_INFO_UPDATE", 6000)
        local lat2, lng2, t = lbsLoc2.request(5000)
        log.info("hdgnss", "基站定位结果", lat2, lng2)
        if lat2 and lng2 then
            lat2 = tonumber(lat2:sub(1, 3) .. lat2:sub(5))
            lng2 = tonumber(lng2:sub(1, 3) .. lng2:sub(5))
            if lat2 and lng2 then
                lat = lat
                lng = lng
                log.info("hdgnss", "基站定位结果", lat, lng)
                hdgnss.aidpos(lat, lng)
            end
        end
    end

    hdgnss.aideph()

    -- sys.wait(1000)
end

function hdgnss.aidpos(lat, lng)
    local gps_uart_id = hdgnss.opts.uart_id
    -- AID-POS
    local str = string.char(
        0xF1, 0xD9,                       --Frame header
        0x0B, 0x10, 0x11, 0x00,           --Command ID: 0x0B 0x11, payload 17 bytes
        0x01                              --LLA 1, ECEF 0
    )
    --lat =  234068458 1132310388
    --        234072543	1132310160
    log.info("hdgnss", "参考坐标(单位1/10000000度)", lat, lng)
    str = str .. pack.pack("<IIII", lat, lng, 5000, 0)
    str = str .. hdcrc(str)
    log.info("hdgnss", "写入AID-POS", str:toHex())
    uart.write(gps_uart_id, str)
end

function hdgnss.aidtime()
    local gps_uart_id = hdgnss.opts.uart_id
    local date = os.date("!*t")
    if date.year > 2023 then
        --AID-TIME, the time accuracy must be within 3 seconds
        log.info("hdgnss", "当前时间", date.year, date.month, date.day, date.hour, date.min, date.sec)
        local str = string.char(
            0xF1, 0xD9,                       --Frame header
            0x0B, 0x11, 0x14, 0x00,           --Command ID: 0x0B 0x11, payload 20 bytes
            0x00,                             --UTC 0, TNOW 1
            0x00,                             --Keep the parameters and fill in 0
            0x11,                             --leap second
            date.year & 0xFF, date.year >> 8, --Year
            date.month,                       --moon
            date.day,                         --day
            date.hour,                        --Hour
            date.min,                         --minute
            date.sec,                         --Second
            0x00, 0x00, 0x00, 0x00,           --Sub-millisecond
            0x00, 0x00,                       --Time precision (seconds)
            0x00, 0x00, 0x00, 0x00            --Time accuracy (sub-millisecond)
        )
        str = str .. hdcrc(str)
        log.info("hdgnss", "写入AID-TIME", str:toHex())
        uart.write(gps_uart_id, str)
        -- sys.wait(20)
    end
end

function hdgnss.setup(opts)
    if not opts then
        opts = {}
    end
    if not opts.uart_id then
        opts.uart_id = 2
    end
    hdgnss.opts = opts
end

function hdgnss.start()
    log.debug("提醒", "室内无GNSS信号,定位不会成功, 要到空旷的室外,起码要看得到天空")
    local gps_uart_id = hdgnss.opts.uart_id
    local uart_forward = hdgnss.opts.uart_forward
    log.info("hdgnss", "start")
    libgnss.on("raw", function(data)
        --sys.publish("uplink", nmea_topic, data, 1)
        if data:byte(1) == 0xF1 then
            log.info("hdgnss", "二进制协议数据", data:toHex())
        end
    end)
    uart.setup(gps_uart_id, 9600)
    if uart_forward then
        uart.setup(uart_forward, 115200)
    end

    pm.power(pm.GPS, true)
    sys.wait(250) --It takes time for the GPNSS chip to start, about 250ms
    --Serial port configuration, switch to 115200
    uart.write(gps_uart_id, (string.fromHex("F1D9060008000000000000C20100D1E0")))
    sys.wait(20)
    uart.close(gps_uart_id)
    sys.wait(10)
    uart.setup(gps_uart_id, 115200)
    --Debug log, optional
    libgnss.debug(hdgnss.opts.debug)
    libgnss.bind(gps_uart_id, uart_forward)

    if not hdgnss.opts.no_agps then
        exec_agnss()
    end
    hdgnss.running = true
end

function hdgnss.stop()
    hdgnss.running = false
    if hdgnss.opts and hdgnss.opts.uart_id then
        uart.close(hdgnss.opts.uart_id)
    end
end

function hdgnss.saveloc()
    local rmc = libgnss.getRmc(1)
    log.info("hdgnss", "rmc", rmc.lat, rmc.lng)
    io.writeFile("/hdloc", string.format("%d,%d", rmc.lat, rmc.lng))
end

--Subscribe to GNSS status encoding
sys.subscribe("GNSS_STATE", function(event, ticks)
    --The event value is
    --FIXED Positioning successful
    --LOSE positioning lost
    --Ticks is the time when the event occurs and can generally be ignored.
    log.info("hdgnss", "state", event, ticks)
    if event == "FIXED" then
        hdgnss.saveloc()
    end
end)

sys.taskInit(function()
    while 1 do
        sys.wait(600 * 1000) --Check every 10 minutes
        local fixed = libgnss.isFix()
        if not fixed and not hdgnss.opts.no_agps and hdgnss.running then
            exec_agnss()
        end
        if fixed then
            hdgnss.saveloc()
        end
    end
end)

return hdgnss
