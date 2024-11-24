--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "101_UPDATE"
VERSION = "0.0.1"

--The ota process is to put update.bin in the root directory (esp is "/spiffs/" and the rest are "/"), and it will be automatically upgraded after restarting.
--Update.bin production method: Click to generate the mass production file in luatools, and rename the XXX.ota file in the SOC mass production and remote upgrade file directory to update.bin.
log.info("main", PROJECT, VERSION)

--sys library is standard
_G.sys = require("sys")

if wdt then
    --Add a hard watchdog to prevent the program from freezing. Enable this feature on supported devices
    wdt.init(9000) --Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 10000) --Feed the watchdog once every 10 seconds
end

--This demo implements a simple serial port data reception to upgrade 101. It does not distinguish other data from the same serial port. It is for reference only.
--Notice! ! ! The 724 serial port level is 1.8V and the Air101 serial port level is 3.3V, which requires level conversion.
--[[
    硬件连接
    Air724UG        Air101

    UART1_TX ------ U1_RX   (PB_07)

    UART1_RX ------ U1_TX   (PB_06)

    GND      ------ GND
]]


local updatePath = "/update.bin" --Upgrade data written to directory

local uartid = 1 --Select different uartid according to the actual device

--initialization
local result = uart.setup(uartid, 115200, 8, 1)

--Receiving data will trigger a callback, where "receive" is a fixed value
local updateSwitch --Determine whether the upgrade file identification has been received
local checkUniqueId = true --Waiting for upgrade to be triggered
local rdbuf = "" --A temporary cache of serial port data
local totalSize, stepSize = 0, 0 --Total length of upgrade files and length of upgrade files received each time
local updateDataTb = {} --Upgrade file data temporary storage table
sys.taskInit(function()
    while true do
        sys.wait(1000)
        log.info("测试", checkUniqueId)
    end
end)
local function clearRdbuf() rdbuf = "" end
uart.on(uartid, "receive", function(id, len)
    local s = ""
    repeat
        --If it is air302, len is not trustworthy, pass 1024
        --s = uart.read(id, 1024)
        s = uart.read(id, len)
        if #s > 0 then --#s is to take the length of the string
            --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
            --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
            sys.timerStopAll(clearRdbuf)
            log.info("uart", "receive", id, #s, s)
            if checkUniqueId then
                rdbuf = rdbuf .. s
                local h, t = string.find(rdbuf, "CHECK_UNIQUE_ID\r\n")
                if h then
                    checkUniqueId = false
                    log.info("触发升级，将当前版本号、imei和bsp发送给Air724")
                    uart.write(uartid,"###VER:" .. VERSION .. "BSP:" .. PROJECT.. "_LuatOS-SoC_"..string.upper(rtos.version().."_"..rtos.bsp()) .."IMEI:" .. mcu.unique_id():toHex() .. "&&&")
                    rdbuf = rdbuf:sub(t + 1)
                else
                    rdbuf = rdbuf .. s
                    sys.timerStart(clearRdbuf, 5000)
                end
            else
                if not updateSwitch then
                    rdbuf = rdbuf .. s
                    --A simple receiving data format is implemented. The upgrade file sent by 724 has the special characters ### and &&& at the beginning, and the total length of the upgrade file is in the middle of the special characters.
                    local h, t = string.find(rdbuf, "###%d+&&&")
                    if h then
                        totalSize = string.match(rdbuf, "###(%d+)&&&")
                        totalSize = tonumber(totalSize)
                        updateSwitch = true
                        rdbuf = rdbuf:sub(t + 1)
                        if rdbuf ~= "" then
                            table.insert(updateDataTb, rdbuf)
                            stepSize = stepSize + #rdbuf
                        end
                        rdbuf = ""
                    else
                        sys.timerStart(clearRdbuf, 5000)
                    end
                else
                    if stepSize < totalSize then
                        if stepSize + #s >= totalSize then
                            table.insert(updateDataTb,
                                         s:sub(1, totalSize - stepSize))
                            stepSize = stepSize +
                                           #s:sub(1, totalSize - stepSize)
                            updateSwitch = false
                            checkUniqueId = true
                            sys.publish("UPDATE_RECV_END")
                        else
                            table.insert(updateDataTb, s)
                            stepSize = stepSize + #s
                        end
                    elseif stepSize >= totalSize then
                        table.insert(updateDataTb,
                                     s:sub(1, stepSize - totalSize))
                        --s = s:sub(stepSize + 1)
                        checkUniqueId = true
                        updateSwitch = false
                        sys.publish("UPDATE_RECV")
                    end
                end
                log.info("uart recv", s)
            end
        end
    until s == ""
end)

--Subscribe to the upgrade file reception completion message, write the upgrade file to the / directory, and then restart
sys.subscribe("UPDATE_RECV_END", function()
    --Write the received upgrade data to a file and then restart
    local file = io.open(updatePath, "wb")
    file:write(table.concat(updateDataTb))
    file:close()
    log.info("重启")
    rtos.reboot()
end)

--Not all devices support the sent event
uart.on(uartid, "sent", function(id) log.info("uart", "sent", id) end)
--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
