--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "usb_uart"
VERSION = "1.0.0"

log.info("main", PROJECT, VERSION)

--Introduce necessary library files (written in Lua), internal libraries do not require require
sys = require("sys")


--[[
注意: 若使用普通串口调试功能, 需要以下条件之一才能收到数据
1. 把DTR设置为高电平
2. 先发送一次数据

参考用的上位机, 用py演示, 需要pyserial库

import os, sys, serial.tools.list_ports, time

for item in serial.tools.list_ports.comports():
    if not item.pid or not item.location :
        continue
    if item.vid == 0x19d1 and item.pid == 0x0001 and "x.6" in item.location :
        print(dir(item))
        print(item.name)
        with serial.Serial(item.name, 115200, timeout=1) as ser:
            while 1:
                data = ser.read(128)
                if data :
                    print( str(time.time()) + ">> " + str(data))
                else :
                    ser.write("Hi from PC".encode())
]]

log.info("main", "usb uart demo")

local uartid = uart.VUART_0 --Fixed id of USB virtual serial port

--initialization
local result = uart.setup(
    uartid,--serial port id
    115200,--The baud rate actually doesn't matter, it's a pure virtual serial port
    8,--data bits
    1--Stop bit
)


--Receiving data will trigger a callback, where "receive" is a fixed value
uart.on(uartid, "receive", function(id, len)
    local s = ""
    repeat
        --s = uart.read(id, 1024)
        s = uart.read(id, len)
        if s and #s > 0 then --#s is to take the length of the string
            --If binary/hexadecimal data is transmitted, some characters are not visible, which does not mean that they were not received.
            --For information about sending and receiving hex values, please refer to https://doc.openluat.com/article/583
            log.info("uart", "receive", id, #s, s)
            uart.write(uart.VUART_0, s)
            --log.info("uart", "receive", id, #s, s:toHex())
        end
    until s == ""
end)
local tx_buff = zbuff.create(24)
tx_buff:set(0, 0x31)
--Not all devices support the sent event
uart.on(uartid, "sent", function(id)
    log.info("uart", "sent", id)
end)

sys.taskInit(function()

    while 1 do
        --uart.write(uart.VUART_0, "hello test usb-uart\r\n")
        uart.tx(uart.VUART_0, tx_buff,0, tx_buff:len())
        sys.wait(1000)
    end
end)


--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
