--- Module function: remote upgrade of 101
module(..., package.seeall)
require "utils"
require "pm"
require "http"
local PRODUCT_KEY = "icqZ0TNzOnBrZGuCueVPJFRQ4cXNfvFJ"
--This demo implements a simple http request to upgrade the file, and sends the upgrade file from the serial port to upgrade 101. It is for reference only.
--Notice! ! ! The 724 serial port level is 1.8V and the Air101 serial port level is 3.3V, which requires level conversion.
--[[
    硬件连接
    Air724UG        Air101

    UART1_TX ------ U1_RX   (PB_07)

    UART1_RX ------ U1_TX   (PB_06)

    GND      ------ GND
]]
local function httpCbFnc(result, prompt, head, body)
    log.info("result", result)
    if result then
        log.info("文件GET成功")
        sys.publish("HTTP_GET_SUCCESS")
    else
        log.info("文件GET失败")
    end
end

local uartID = 1
local checkSwitch
sys.taskInit(function()
    uart.on(uartID, "sent", function() sys.publish("UART1_SENT101_OK") end)
    uart.on(uartID, "receive", function() sys.publish("UART1_DATA_INCOMING") end)
    uart.setup(uartID, 115200, 8, uart.PAR_NONE, uart.STOP_1, nil, 1)
    local _, ver, imei, bsp = sys.waitUntil("HTTP_GET_START")
    log.info("test", _, ver, imei, bsp)
    local url =
        "http://iot.openluat.com/api/site/firmware_upgrade?project_key=" ..
            PRODUCT_KEY .. "&imei=" .. imei .. "&firmware_name=" .. bsp ..
            "&version=" .. ver
    http.request("GET", url, nil, nil, nil, 30000, httpCbFnc, "/update.bin")
    sys.waitUntil("HTTP_GET_SUCCESS")
    local fileHandle = io.open("/update.bin", "rb")
    if not fileHandle then
        log.error("open updateFile error")
        return
    end
    local size = io.fileSize("/update.bin")
    log.info("打印大小", size)
    pm.wake("UART1_SENT101")
    uart.write(uartID, "###" .. size .. "&&&")
    sys.waitUntil("UART1_SENT101_OK")
    while true do
        local data = fileHandle:read(1460)
        if not data then break end
        uart.write(uartID, data)
        sys.waitUntil("UART1_SENT101_OK")
    end

    uart.close(uartID)
    pm.sleep("UART1_SENT101")
    fileHandle:close()
    os.remove("/update.bin")
    checkSwitch = false
end)
local rdbuf = ""

local function clearRdbuf() rdbuf = "" end

local function proc(s)
    rdbuf = rdbuf .. s
    if not rdbuf:find("###VER:.+BSP:.+IMEI:.+&&&") then
        sys.timerStart(clearRdbuf, 5000)
    else
        sys.timerStop(clearRdbuf)
        local ver, bsp, imei = string.match(rdbuf,
                                            "###VER:(.+)BSP:(.+)IMEI:(.+)&&&")
        rdbuf = ""
        if not checkSwitch then
            checkSwitch = true
            sys.publish("HTTP_GET_START", ver, imei, bsp)
        end
    end
end
sys.taskInit(function()
    while true do
        sys.waitUntil("UART1_DATA_INCOMING")
        while true do
            local s = uart.read(uartID, 8192)
            if s and #s > 0 then
                proc(s)
            else
                break
            end
        end
    end
end)

--Notification 101 is about to be upgraded, please send unique_id.
sys.timerStart(uart.write, 5000, uartID, "CHECK_UNIQUE_ID\r\n")
