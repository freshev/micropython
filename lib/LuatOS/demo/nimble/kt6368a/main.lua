
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "bletest"
VERSION = "1.0.0"

--[[
这是使用BLE功能模拟KT6368A的demo, 从机模式, UART1透传

支持的模块:
1. Air601
2. ESP32系列, 包括ESP32C3/ESP32S3
3. Air101/Air103 开发板天线未引出, 天线未校准, 能用但功耗高
]]

log.info("main", PROJECT, VERSION)

--Be sure to add sys.lua!!!!
sys = require("sys")

--Add a hard watchdog to prevent the program from freezing
if wdt then
    wdt.init(9000)--Initialize watchdog set to 9s
    sys.timerLoopStart(wdt.feed, 3000)--Feed the watchdog once every 3 seconds
end

function pinx() --According to different development boards, assign different gpio pin numbers to the LED
    local rtos_bsp = rtos.bsp()
    if rtos_bsp == "AIR101" then --Air101 development board LED pin number
        mcu.setClk(240)
        return pin.PB08, 1
    elseif rtos_bsp == "AIR103" then --Air103 development board LED pin number
        mcu.setClk(240)
        return pin.PB26, 1
    elseif rtos_bsp == "AIR601" then --Air601 development board LED pin number
        return pin.PB26, 1
    elseif rtos_bsp == "ESP32C3" then --ESP32C3 development board pins
        return 12, 0
    elseif rtos_bsp == "ESP32S3" then --ESP32C3 development board pins
        return 10, 0
    else
        log.info("main", "define led pin in main.lua")
        return 0, 1
    end
end
local ledpin, uart_id = pinx()
LED = gpio.setup(ledpin, 0, gpio.PULLUP)
uart.setup(uart_id, 9600)
uart.on(uart_id, "receive", function(id, len)
    gpio.toggle(ledpin)
    local s = ""
    repeat
        s = uart.read(uart_id, 128)
        if #s > 0 then --#s is to take the length of the string
            --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
            --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
            log.info("uart", "receive", id, #s, s)
            --TODO determines whether it is an AT command. If so, it needs to be parsed.
            if nimble.connok() then
                --nimble.send_msg(1, 0, s)
                --nimble.sendNotify(nil, string.fromHex("FF01"), string.char(0x31, 0x32, 0x33, 0x34, 0x35))
                nimble.sendNotify(nil, string.fromHex("FF01"), s)
            end
        end
    until s == ""
    gpio.toggle(ledpin)
end)

--Listen to the WRITE_CHR of the GATT server, which is the callback for collecting data
sys.subscribe("BLE_GATT_WRITE_CHR", function(info, data)
    --info is a table, but there is currently no data
    log.info("ble", "data got!!", data:toHex())
    uart.write(uart_id, data)
end)

sys.subscribe("BLE_SERVER_STATE_UPD", function(state)
    log.info("ble", "连接状态", nimble.connok() and "已连接" or "已断开")
    LED(nimble.connok() and 1 or 0)
end)

sys.taskInit(function()
    sys.wait(500)

    nimble.config(nimble.CFG_ADDR_ORDER, 1)

    nimble.setUUID("srv", string.fromHex("FF00"))      --Service master UUID
    nimble.setChr(0, string.fromHex("FF01"), nimble.CHR_F_WRITE_NO_RSP | nimble.CHR_F_NOTIFY)
    nimble.setChr(1, string.fromHex("FF02"), nimble.CHR_F_READ | nimble.CHR_F_NOTIFY)
    nimble.setChr(2, string.fromHex("FF03"), nimble.CHR_F_WRITE_NO_RSP)

    nimble.init("KT6368A-BLE-1.9", 1)
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
