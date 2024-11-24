
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ulwip"
VERSION = "1.0.0"

--[[
本demo是尝试对接W5100

W5100的文档
https://www.wiznet.io/wp-content/uploads/wiznethome/Chip/W5100/Document/W5100_DS_V128E.pdf
https://d1.amobbs.com/bbs_upload782111/files_29/ourdev_555431.pdf

接线方式:
1. 5V -> VCC
2. GND -> GND
3. SCK -> PB05
4. MISO -> PB06
5. MOSI -> PB07
6. CS -> PB04

本demo的现状:
1. 一定要为W5100s稳定供电
2. 确保w5100s与模块的物理连接是可靠的
3. 当前使用轮询方式读取W5100s的状态, 未使用中断模式
]]

--sys library is standard
_G.sys = require("sys")
require "sysplus"

SPI_ID = 0
spi.setup(SPI_ID, nil, 0, 0, 8, 1*1000*1000)
PIN_CS = gpio.setup(pin.PB04, 1, gpio.PULLUP)

TAG = "w5100s"
local mac = "0C1234567890"
local adapter_index = socket.LWIP_ETH

tx_queue = {}

--Encapsulated write function, the format of w5100s is 32bit write-at-a-time, and the valid data is only 1 byte
function w5xxx_write(addr, data)
    for i=1, #data do
        PIN_CS(0)
        local data = string.char(0xF0, (addr & 0xFF00) >> 8, addr & 0x00FF, data:byte(i))
        spi.send(SPI_ID, data)
        addr = addr + 1
        PIN_CS(1)
    end
    
end

--Encapsulated reading function, the format of w5100s is 32bit read at a time, the valid data is only 1 byte
function w5xxx_read(addr, len)
    local result = ""
    for i=1, len do
        PIN_CS(0)
        local data = string.char(0x0F, (addr & 0xFF00) >> 8, addr & 0x00FF)
        spi.send(SPI_ID, data)
        --log.info("Send read command", data:toHex())
        result = result .. spi.recv(SPI_ID, 1)
        addr = addr + 1
        PIN_CS(1)
    end
    return result
end

--Read a 16-bit unsigned integer
local function read_UINT16(addr)
    local data = w5xxx_read(addr, 2)
    --log.info(TAG, "Read register", string.format("0x%04X", addr), data:toHex())
    if #data ~= 2 then
        log.error(TAG, "读取寄存器失败", string.format("0x%0x4", addr))
        return 0
    end
    local ival = data:byte(1) *256 + data:byte(2)
    return ival
end

--Write a 16-bit unsigned integer
local function write_UINT16(addr, val)
    local data = pack.pack(">H", val)
    --log.info(TAG, "Write register", string.format("0x%04X", addr), data:toHex())
    w5xxx_write(addr, data)
end

--Print data from common registers
function print_CR(data)
    --Current mode
    local mode = data:byte(1)
    log.info("当前模式", mode)
    --Source Hardware Address. Should be the MAC address
    local mac = data:sub(0x0A, 0x0F)
    log.info("源MAC地址", mac:toHex())

    --Interrupt Register
    local irq = data:byte(0x15 + 1)
    log.info("中断状态", irq)

    --RX Memory Size
    local rx_mem = data:byte(0x1A + 1)
    log.info("RX内存大小", rx_mem >> 6, (rx_mem >> 4) & 0x3, (rx_mem >> 2) & 0x3, rx_mem & 0x3)
    --TX Memory Size
    local tx_mem = data:byte(0x1B + 1)
    log.info("TX内存大小", tx_mem >> 6, (tx_mem >> 4) & 0x3, (tx_mem >> 2) & 0x3, tx_mem & 0x3)
end

--Set mac address
function w5xxx_set_mac(mac)
    w5xxx_write(0x09, mac)
end

--Read the received data and take it from the buffer
function w5xxx_read_data(len, update_mark)
    --Read the data length first
    --local data = w5xxx_read(0x0426, 2)
    local remain_size = read_UINT16(0x0426)
    --Read pointer position
    --data = w5xxx_read(0x0428, 2)
    local rx_offset = read_UINT16(0x0428) & 0x1FFF
    --log.info(TAG, "RX register status", "remaining to be read", remain_size, "offset", string.format("0x%04X", rx_offset))

    if len > remain_size then
        log.info("请求读取的长度大于剩余长度", len, remain_size)
        len = remain_size
        -- return
    end
    local data = ""
    if rx_offset + len > 0x2000 then
        --Need circular reading
        local data1 = w5xxx_read(0x6000 + rx_offset, 0x2000 - rx_offset)
        local data2 = w5xxx_read(0x6000, len - #data1)
        data = data1 .. data2
    else
        data = w5xxx_read(0x6000 + rx_offset, len)
    end
    --Update read pointer position
    if update_mark then
        local t = (read_UINT16(0x0428) + len)
        --log.info(TAG, "Writeback pointer offset", string.format("0x%04X", t))
        w5xxx_write(0x0428, string.char((t & 0xFF00) >> 8, t & 0xFF))
        --Notify that reading has been completed
        w5xxx_write(0x0401, string.char(0x40))
    end
    return data
end

--Write data to the send buffer and execute the send
function w5xxx_write_data(data)
    --First, read the pointer position
    local tx_offset = read_UINT16(0x0422) & 0x1FFF
    log.info(TAG, "TX寄存器状态", "剩余可写", read_UINT16(0x0420), "偏移量", string.format("0x%04X", tx_offset))

    --data = string.char(#data >>8, #data & 0xFF) .. data
    if tx_offset + #data > 0x2000 then
        --Requires circular writing
        w5xxx_write(0x4000 + tx_offset, data:sub(1, 0x2000 - tx_offset))
        w5xxx_write(0x4000, data:sub(0x2000 - tx_offset + 1))
    else
        w5xxx_write(0x4000 + tx_offset, data)
    end
    --Update read pointer position
    --log.info(TAG, "Current TX pointer offset", string.format("0x%04X", read_UINT16(0x0424)), "Data length", #data)
    local t = read_UINT16(0x0424) + #data
    --log.info(TAG, "Target TX pointer offset", string.format("0x%04X", t))
    --w5xxx_write(0x0424, string.char((t & 0xFF00) >> 8, t & 0xFF))
    write_UINT16(0x0424, t)
    --Check it, read the RR/WR value, and calculate the difference (sending length)
    --local tx_start = read_UINT16(0x0422)
    --local tx_end = read_UINT16(0x0424)
    --log.info(TAG, "TX register status", "start pointer", tx_start, "end pointer", tx_end, "difference", (tx_end - tx_start) & 0x1FFF, #data)
    --Notify that writing has been completed
    w5xxx_write(0x0401, string.char(0x20))
end

--Link out of docking ulwip library
function netif_write_out(adapter_index, data)
    --log.info(TAG, "mac data to be sent", data:toHex())
    table.insert(tx_queue, data)
end

sys.taskInit(function()
    sys.wait(1000)
    if wlan and wlan.init then
        wlan.init()
    end
    ---------------------------
    log.info(TAG, "w5100开始搞")
    w5xxx_write(0x0, string.char(0x80))
    w5xxx_read(0x00, 0x10)
    sys.wait(200)
    local data = w5xxx_read(0x00, 0x30)
    log.info(TAG, "w5xxx_read", data:toHex())
    print_CR(data)
    if #data == 0 then
        log.info("w5100通信失败!!!")
        return
    end

    --Set mode to fully closed
    --local mode = w5xxx_read(0x0, 1)
    --log.info(TAG, "MR,before setting", mode:toHex())
    --w5xxx_write(0x0, string.char(0x0))
    --mode = w5xxx_read(0x0, 1)
    --log.info(TAG, "MR,after setting", mode:toHex())

    --Write MAC address
    log.info(TAG, "写入MAC地址", mac)
    w5xxx_set_mac((mac:fromHex()))

    --Set TX/RX buffer size
    w5xxx_write(0x1A, string.char(0x03)) --All to S0, 8kb
    w5xxx_write(0x1B, string.char(0x03)) --All to S0, 8kb

    --Read the general register data again
    data = w5xxx_read(0x00, 0x30)
    log.info(TAG, "w5xxx_read", data:toHex())
    print_CR(data)

    --Set S0 to MACRAW mode
    w5xxx_write(0x0400, string.char(0x04 | 0x40))
    -- sys.wait(100)
    --w5xxx_write(0x0402, string.char(0x01))
    --Enable data sending and receiving
    w5xxx_write(0x0401, string.char(0x01))
    sys.wait(100)
    w5xxx_write(0x0402, string.char(0x01))
    sys.wait(100)

    sys.publish("w5100_ready")

    while 1 do

        --Handle receive queue
        local rx_size = read_UINT16(0x0426)
        --log.info(TAG, "length of data to be received", rx_size)
        if rx_size > 0 then
            data = w5xxx_read_data(2)
            local frame_size = data:byte(1) * 256 + data:byte(2)
            if frame_size < 60 or frame_size > 1600 then
                log.info(TAG, "MAC帧大小异常", frame_size)
                w5xxx_read_data(frame_size, 8192) --discard all
            else
                --log.info(TAG, "MAC frame size", frame_size - 2)
                local mac_frame = w5xxx_read_data(frame_size, true)
                if mac_frame then
                    --log.info(TAG, "MAC frame data (including 2-byte header)", mac_frame:toHex())
                    ulwip.input(adapter_index, mac_frame:sub(3))
                end
            end
        end

        --Handle send queue
        if #tx_queue > 0 then
            local send_buff_remain = read_UINT16(0x0420)
            local tmpdata = tx_queue[1]
            log.info(TAG, "发送队列", #tx_queue, #tmpdata, send_buff_remain)
            if send_buff_remain >= #tmpdata then
                tmpdata = table.remove(tx_queue, 1)
                w5xxx_write_data(tmpdata)
                sys.wait(5)
            end
        end

        --Is there any data to be received?
        rx_size = read_UINT16(0x0426)
        if rx_size == 0 and #tx_queue == 0 then
            sys.wait(20)
        else
            sys.wait(5)
        end
        --sys.wait(100)
    end

    log.info("结束了..............")
end)

sys.taskInit(function()
    sys.waitUntil("w5100_ready")
    log.info("适配器索引是啥", adapter_index)

    --local mac = string.fromHex("0C1456060177")
    local ret = ulwip.setup(adapter_index, string.fromHex(mac), netif_write_out)
    log.info("ulwip.setup", ret)
    if ret then
        ulwip.reg(adapter_index)
        log.info("ulwip", "添加成功, 设置设备就绪")
        ulwip.updown(adapter_index, true)
        log.info("ulwip", "启动dhcp")
        ulwip.dhcp(adapter_index, true)
        --ulwip.ip(adapter_index, "192.168.1.199", "255.255.255.0", "192.168.1.1")
        sys.wait(100)
        log.info("ulwip", "设置设备已经在线")
        ulwip.link(adapter_index, true)
        while ulwip.ip(adapter_index) == "0.0.0.0" do
            sys.wait(1000)
            log.info("等待IP就绪")
        end
        log.info("ulwip", "IP地址", ulwip.ip(adapter_index))
        --In order to access the external network normally, the network card needs to be set as the default route.
        ulwip.dft(adapter_index)
        -- sys.publish("net_ready")
        sys.wait(1000)
        log.info("发起http请求")
        local code, headers, body = http.request("GET", "http://httpbin.air32.cn/get", nil, nil, {adapter=adapter_index, timeout=5000, debug=true}).wait()
        --local code, headers, body = http.request("GET", "http://192.168.1.6:8000/get", nil, nil, {adapter=adapter_index, timeout=5000, debug=true}).wait()
        log.info("ulwip", "http", code, json.encode(headers or {}), body)
    end
end)

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
