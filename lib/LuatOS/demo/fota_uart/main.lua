--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "fotademo"
VERSION = "1.0.0"

--[[
演示用uart进行固件升级

本demo 适用于 Air780E/Air780EG/Air600E
1. 需要 V1107及以上的固件
2. 需要 LuaTools 2.1.89 及以上的升级文件生成

目录内文件的说明
1. main.lua 本文件, 是下载到模块的唯一文件
2. fota_uart.bin 演示用的脚本升级文件,不需要下载到模块
3. main.py 上位机发送升级包的程序, 不需要下载到模块

fota_uart.bin 使用 LuaTools 的 "量产文件" 按钮进行生成, .bin后缀的就是脚本OTA文件

用法:
1. 先把脚本和固件烧录到模块里, 并确认开机
2. 进入命令行程序, 执行 python main.py 升级
3. 观察luatools的输出和main.py的输出
4. 模块接收正确的升级数据后,会提示1秒后重启
5. 本demo自带的脚本升级包,升级后是GPIO闪灯的demo

注意: 本demo默认是走虚拟串口进行交互, 如需改成物理串口, 修改uart_id和main.py
]]

_G.sys = require "sys"

--Define the required UART number
--uart_id = 1 -- physical UART1, usually also MAIN_UART
uart_id = uart.VUART_0 --Virtual USB serial port

--Print the current version in a loop to visually indicate whether the upgrade is successful.
-- sys.taskInit(function()
--while 1 do
--         sys.wait(3000)
--log.info("fota", "version", VERSION)
--     end
-- end)

--Start initializing uart
uart_zbuff = zbuff.create(1024)
uart.setup(uart_id, 115200)

--Several state variables, used for debugging
local uart_fota_state = 0
local uart_rx_counter = 0
local uart_fota_writed = 0
uart.on(uart_id, "receive", function(id, len)
    --Defense against buffer overruns
    if uart_zbuff:used() > 8192 then
        log.warn("fota", "uart_zbuff待处理的数据太多了,强制清空")
        uart_zbuff:del()
    end
    while 1 do
        local len = uart.rx(id, uart_zbuff)
        if len <= 0 then
            break
        end
        --if #s > 0 then -- #s is the length of the string
        uart_rx_counter = uart_rx_counter + len
        log.info("uart", "收到数据", len, "累计", uart_rx_counter)
        if uart_fota_state == 0 then
            sys.publish("UART_FOTA")
        end
    end
end)

sys.taskInit(function()
    local fota_state = 0 --0 has not started yet, 1 is in progress
    while 1 do
        --Waiting for the upgrade data to arrive
        sys.waitUntil("UART_FOTA", 1000)
        local used = uart_zbuff:used()
        if used > 0 then
            if fota_state == 0 then
                --Waiting for FOTA status
                if used > 5 then
                    local data = uart_zbuff:query()
                    uart_zbuff:del()
                    --If #FOTA\n is received, it means data is coming.
                    if data:startsWith("#FOTA") and data:endsWith("\n") then
                        fota_state = 1
                        log.info("fota", "检测到fota起始标记,进入FOTA状态", data)
                        fota.init()
                        --The firmware data sender should start sending data after receiving #FOTA RDY\n
                        uart.write(uart_id, "#FOTA RDY\n")
                    end
                end
            else
                uart_fota_writed = uart_fota_writed + used
                log.info("准备写入fota包", used, "累计写入", uart_fota_writed)
                local result, isDone, cache = fota.run(uart_zbuff)
                log.debug("fota.run", result, isDone, cache)
                uart_zbuff:del() --Clear buffer
                if not result then
                    fota_state = 0
                    fota.isDone()
                    uart.write(uart_id, "#FOTA ERR\n")
                    log.info("fota", "出错了", result, isDone, cache)
                elseif isDone then
                    while true do
                        sys.wait(100)
                        local succ, fotaDone = fota.isDone()
                        if not succ then
                            fota_state = 0
                            uart.write(uart_id, "#FOTA ERR\n")
                            log.info("fota", "出错了")
                            break
                        end
                        if fotaDone then
                            uart_fota_state = 1
                            log.info("fota", "已完成,1s后重启")
                            --Feedback to the host computer
                            uart.write(uart_id, "#FOTA OK\n")
                            sys.wait(1000)
                            rtos.reboot()
                        end
                        sys.wait(100)
                    end
                else
                    log.info("fota", "单包写入完成", used, "等待下一个包")
                    uart.write(uart_id, "#FOTA NEXT\n")
                end
            end
        end
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
