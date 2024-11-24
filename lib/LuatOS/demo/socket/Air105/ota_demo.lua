--Use http method to download ota firmware and upgrade it
--Note that the download speed is much faster than writing to flash, and the ram is limited. The w5500 currently cannot control the download speed, so you need to erase the flash first and estimate a certain amount of time before you can start downloading.
--[[
--This is to use uart3 to directly send the upgrade package to the MCU. As a reference for local upgrades, the high-speed serial port should not send more than 4K in a single time.
local rbuff = zbuff.create(16384)
local function uartRx(id, len)
    uart.rx(id, rbuff)
    if rbuff:used() > 0 then 
        local succ,fotaDone,nCache = fota.run(rbuff)
        
        if succ then
            
        else
            log.error("fota写入异常，请至少在1秒后重试")
            fota.finish(false)
            return
        end
        rbuff:del()
        --log.info("Received server data, length", rbuff:used(), "fota result", succ, done, "total", filelen)
        if fotaDone then
            log.info("下载完成")
            sys.publish("downloadOK")
        end
    end
end

local function otaTask()
    local spiFlash = spi.deviceSetup(spi.SPI_1,pin.PA7,0,0,8,24*1000*1000,spi.MSB,1,1)
    fota.init(0, 0x00200000, spiFlash)
    while not fota.wait() do
        sys.wait(100)
    end
    
    uart.setup(3,1500000)
    uart.on(3, "receive", uartRx)
    uart.write(3,"ready")
    sys.waitUntil("downloadOK")                       
    while true do
        local succ,fotaDone  = fota.isDone()
        if fotaDone then
            fota.finish(true)
            log.info("FOTA完成")
            rtos.reboot()   --If you have other things to do, don’t reboot immediately
            break
        end
        sys.wait(100)
    end
end
function otaDemo()
    sys.taskInit(otaTask)
end
]]


local libnet = require "libnet"

local taskName = "OTA_TASK"
local function netCB(msg)
    log.info("未处理消息", msg[1], msg[2], msg[3], msg[4])
end

local function otaTask()
    local spiFlash = spi.deviceSetup(spi.SPI_1,pin.PA7,0,0,8,24*1000*1000,spi.MSB,1,1)
    fota.init(0, 0x00200000, spiFlash)
    while not fota.wait() do
        sys.wait(100)
    end
    --If you are sure to only download the script, you can download it to the file system
    --fota.init("/update.bin", 0, nil)
    -- os.remove("/update.bin")
    local succ, param, ip, port, total, findhead, filelen, rcvCache,d1,d2,statusCode,retry,rspHead,rcvChunked,done,fotaDone,nCache
    local tbuff = zbuff.create(512)
    local rbuff = zbuff.create(2048)
    local netc = socket.create(socket.ETH0, taskName)
    socket.config(netc, nil, nil, nil) --Ordinary TCP connection for http
    --For the IoT platform used, use the chip name + 24byte id (actually 12byte, 96bit)
    --Of course, you can also use the ID rules you set yourself.
    local imei,_ = mcu.unique_id():toHex()
    imei = rtos.bsp() .. string.sub(imei, 9, 32)
    log.info(imei, rtos.firmware())
    filelen = 0
    total = 0
    retry = 0
    done = false
    rspHead = {}
    local result = libnet.waitLink(taskName, 0, netc)
    while retry < 3 and not done do
        result = libnet.connect(taskName, 5000, netc, "iot.openluat.com", 80) --Later, when the http library was released, it was processed directly using http.
        tbuff:del()
        --The IoT platform is used, so the firmware name and version number need to be processed accordingly.
        --Using a self-built platform, you can customize the rules independently
        local v = rtos.version()
        v = tonumber(v:sub(2, 5))
        tbuff:copy(0, "GET /api/site/firmware_upgrade?project_key=" .. _G.PRODUCT_KEY .. "&imei=".. imei .. "&device_key=&firmware_name=" .. _G.PROJECT.. "_LuatOS-SoC_" .. rtos.bsp() .. "&version=" .. v .. "." .. _G.VERSION .. " HTTP/1.1\r\n")
        tbuff:copy(nil,"Host: iot.openluat.com:80\r\n")
        if filelen > 0 then --If you disconnect and reconnect, you only need to download the remaining parts.
            tbuff:copy(nil,"Range: bytes=" .. total .. "-\r\n") 
        end
        
        tbuff:copy(nil,"Accept: application/octet-stream\r\n\r\n")
        log.info(tbuff:query())
        result = libnet.tx(taskName, 5000, netc, tbuff)
        rbuff:del()
        findhead = false
        while result do
            succ, param, ip, port = socket.rx(netc, rbuff)
            if not succ then
                log.info("服务器断开了", succ, param, ip, port)
                break
            end
            if rbuff:used() > 0 then
                if findhead then
                    succ,fotaDone,nCache = fota.run(rbuff)
                    
                    if succ then
                        total = total + rbuff:used()
                    else
                        log.error("fota写入异常，请至少在1秒后重试")
                        fota.finish(false)
                        done = true
                        break
                    end
                    rbuff:del()
                    --log.info("Received server data, length", rbuff:used(), "fota result", succ, done, "total", filelen)
                    if fotaDone then
                        log.info("下载完成")
                        while true do
                            succ,fotaDone  = fota.isDone()
                            if fotaDone then
                                fota.finish(true)
                                log.info("FOTA完成")
                                done = true
                                rtos.reboot()   --If you have other things to do, don’t reboot immediately
                                break
                            end
                            sys.wait(100)
                        end
                        break
                    end
                else
                    rcvCache = rbuff:query()
                    d1,d2 = rcvCache:find("\r\n\r\n")
                    --Print out http response head
                    --log.info(rcvCache:sub(1, d2))
                    if d2 then
                        --status line
                        _,d1,statusCode = rcvCache:find("%s(%d+)%s.-\r\n")
                        if not statusCode then
                            log.info("http没有状态返回")
                            break
                        end
                        statusCode = tonumber(statusCode)
                        if statusCode ~= 200 and statusCode ~= 206 then
                            log.info("http应答不OK", statusCode)
                            done = true
                            break
                        end
                        --response header
                        for k,v in string.gmatch(rcvCache:sub(d1+1,d2-2),"(.-):%s*(.-)\r\n") do
                            rspHead[k] = v
                            if (string.upper(k)==string.upper("Transfer-Encoding")) and (string.upper(v)==string.upper("chunked")) then rcvChunked = true end
                        end
                        if filelen == 0 and not rcvChunked then 
                            if not rcvChunked then
                                filelen = tonumber(rspHead["Content-Length"] or "2147483647")
                            end
                        end
                        --Unprocessed body data
                        rbuff:del(0, d2)
                        succ,fotaDone,nCache = fota.run(rbuff)
                        if not succ then
                            total = total + rbuff:used()
                        else
                            log.error("fota写入异常，请至少在1秒后重试")
                            fota.finish(false)
                            done = true
                            break
                        end
                        rbuff:del()
                        --log.info("Received server data, length", rbuff:used(), "fota result", succ, done, "total", filelen)
                    else
                        break
                    end
                    findhead = true
                end
            end 
            result, param = libnet.wait(taskName, 5000, netc)
            if not result then
                log.info("服务器断开了", result, param)
                break
            elseif not param then
                log.info("服务器没有数据", result, param)
                break
            end
        end
        libnet.close(taskName, 5000, netc)
        retry = retry + 1
    end
    socket.release(netc)
    sysplus.taskDel(taskName)
    fota.finish(false)
end

function otaDemo()
    sysplus.taskInitEx(otaTask, taskName, netCB)
end
