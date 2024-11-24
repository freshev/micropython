--[[
@module w5100s
@summary 集成wiznet w5100/w5200系列网卡驱动
@version 1.0
@date    2024.2.22
@author  wendal
@usage

local w5100s = require("w5100s")
spi_id = 1
spi.setup(spi_id, nil, 0, 0, 8, 1*1000*1000)
w5100s.init("w5100", {spi=spi_id, pin_cs=10, pin_int=nil})
]]
local w5100s = {}

--[[
硬件资料:
1. w5100s/w5100 https://www.w5100s.io/wp-content/uploads/w5100shome/Chip/W5100/Document/W5100_DS_V128E.pdf
2. w5200 https://www.w5100s.io/wp-content/uploads/w5100shome/Chip/W5200/Documents/W5200_DS_V140E.pdf

不同芯片之间的主要差异:
1. 通用缓冲区的长度不同, 但设置MAC的寄存器地址是一样的
2. 缓冲区总大小不同, 导致接收缓冲区的起始地址不同, 发送缓冲区的起始地址是相同的
3. LINK状态的寄存器位置有差异,但使用轮询式就不关注了

实现逻辑:
1. 初始化/复位芯片
2. 设置缓冲区大小,设置MAC,开启MACRAW模式
3. 轮询读取缓冲区和发送队列

错误检测:
1. 定时检测link状态,如果断掉则通知ulwip库
2. 定时检测新的当前模式是不是MACRAW,如果不是就重新初始化

TODO:
1. 控制txqueue的大小
2. 定时检测link状态,如果断掉则通知ulwip库
]]

local TAG = "w5100s"
w5100s.tx_queue = {}

function w5100s.init(tp, opts)
    --buffsize buffer size, model dependent

    if tp == "w5100" or tp == "w5100s" then
        opts.buffsize = 8192
    elseif tp == "w5200" then
        opts.buffsize = 16384
    else
        if not opts.buffsize then
            log.error(TAG, "型号未知且未定义缓冲区大小,无法初始化")
            return
        end
    end

    --Check if necessary libraries exist ulwip/zbuff/spi/gpio
    if not ulwip or not zbuff or not spi or not gpio then
        log.error(TAG, "缺少必要的库: ulwip/zbuff/spi/gpio")
        return
    end
    --The CS pin must be present, it can be a number or a GPIO callback
    if not opts.pin_cs then
        log.error("TAG", "CS脚(pin_cs)必须定义")
        return
    elseif type(opts.pin_cs) == "number" then
        opts.pin_cs = gpio.setup(opts.pin_cs, 1, gpio.PULLUP)
    end
    --SPI must be defined, the speed is defined by the customer
    if not opts.spi_id then
        log.error("TAG", "SPI(spi_id)必须定义")
        return
    end
    --TODO Check if the chip exists

    w5100s.opts = opts
    w5100s.cmdbuff = zbuff.create(4)
    w5100s.rxbuff = zbuff.create(1600)
    return true
end

--Functions for docking hardware
------------------------------------

--Encapsulated write function, the format of w5100s is 32bit write-at-a-time, and the valid data is only 1 byte
local function w5xxx_write(addr, data)
    local cmdbuff = w5100s.cmdbuff
    for i=1, #data do
        cmdbuff[0] = 0xF0
        cmdbuff[1] = (addr & 0xFF00) >> 8
        cmdbuff[2] = addr & 0x00FF
        cmdbuff[3] = data:byte(i)
        cmdbuff:seek(0)
        w5100s.opts.pin_cs(0)
        --local data = string.char(0xF0, (addr & 0xFF00) >> 8, addr & 0x00FF, data:byte(i))
        --spi.send(SPI_ID, data)
        spi.send(SPI_ID, cmdbuff, 4)
        addr = addr + 1
        w5100s.opts.pin_cs(1)
    end
end

--Encapsulated reading function, the format of w5100s is 32bit read at a time, the valid data is only 1 byte
local function w5xxx_read(addr, len, rxbuff, offset)
    local result = ""
    local cmdbuff = w5100s.cmdbuff
    for i=1, len do
        cmdbuff[0] = 0x0F
        cmdbuff[1] = (addr & 0xFF00) >> 8
        cmdbuff[2] = addr & 0x00FF
        cmdbuff:seek(0)
        w5100s.opts.pin_cs(0)
        --local data = string.char(0x0F, (addr & 0xFF00) >> 8, addr & 0x00FF)
        --spi.send(SPI_ID, data)
        spi.send(SPI_ID, cmdbuff, 3)
        --log.info("Send read command", data:toHex())
        local ival = spi.recv(SPI_ID, 1)
        if rxbuff then
            rxbuff[offset + i - 1] = #ival == 1 and ival:byte(1) or 0
        else
            result = result .. ival
        end
        addr = addr + 1
        w5100s.opts.pin_cs(1)
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
local function print_CR(data)
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

--Set/read mac address
function w5100s.mac(mac)
    if mac then
        w5xxx_write(0x09, mac)
    end
    return w5xxx_read(0x09, 6)
end

--Read the received data and take it from the buffer
local function w5xxx_read_data(len, update_mark, rawmode)
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
    local rxbuff = w5100s.rxbuff
    if rx_offset + len > 0x2000 then
        --Need circular reading
        --local data1 = w5xxx_read(0x6000 + rx_offset, 0x2000 - rx_offset)
        --local data2 = w5xxx_read(0x6000, len - #data1)
        --data = data1 .. data2
        w5xxx_read(0x6000 + rx_offset, 0x2000 - rx_offset, rxbuff, 0)
        w5xxx_read(0x6000, len - (0x2000 - rx_offset), rxbuff, 0x2000 - rx_offset)
    else
        --data = w5xxx_read(0x6000 + rx_offset, len)
        w5xxx_read(0x6000 + rx_offset, len, rxbuff, 0)
    end
    --log.info(TAG, "Read data", data:toHex())
    --Update read pointer position
    if update_mark then
        local t = (read_UINT16(0x0428) + len)
        --log.info(TAG, "Writeback pointer offset", string.format("0x%04X", t))
        w5xxx_write(0x0428, string.char((t & 0xFF00) >> 8, t & 0xFF))
        --Notify that reading has been completed
        w5xxx_write(0x0401, string.char(0x40))
    end
    if rawmode then
        return rxbuff
    end
    return rxbuff:toStr(0, len)
end

--Write data to the send buffer and execute the send
local function w5xxx_write_data(data)
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

function w5100s.ready()
    --At least you need to call init.
    if not w5100s.opts then
        log.info(TAG, "未初始化")
        return
    end
    --Read MACRAW status
    local macraw = w5xxx_read(0x0403, 1)
    if not macraw or #macraw ~= 1 or macraw ~= "\x42" then
        log.info(TAG, "MACRAW状态异常", string.toHex(macraw or ""))
        return
    end
    --Read MAC address
    local mac = w5xxx_read(0x09, 6)
    if not mac or #mac ~= 6 or mac == "\0\0\0\0\0\0" then
        log.info(TAG, "MAC未设置")
        return
    end
    return true
end

--[[
获取link的状态
@api w5100s.link()
@return boolean 返回true表示已连接, false表示未连接或其他错误
]]
function w5100s.link()
    if not w5100s.opts then
        log.info(TAG, "未初始化")
        return
    end
    local pyh = w5xxx_read(0x003C, 1)
    if not pyh or #pyh ~= 1 or ((pyh:byte(1) & 0x80) ~= 0) then
        log.info(TAG, "PYH状态值", string.toHex(pyh or ""))
        return
    end
    return true
end

function w5100s.do_init()
    --log.info(TAG, "w5100 started")
    w5xxx_write(0x0, string.char(0x80))
    --w5xxx_read(0x00, 0x10)
    sys.wait(200)
    local data = w5xxx_read(0x00, 0x30)
    --log.info(TAG, "w5xxx_read", data:toHex())
    -- print_CR(data)
    if #data == 0 then
        log.info("w5100通信失败!!!")
        return
    end

    --Write MAC address
    if w5100s.opts.mac then
        log.info(TAG, "写入MAC地址", w5100s.opts.mac)
        w5100s.mac(w5100s.opts.mac)
    end

    --Set TX/RX buffer size
    w5xxx_write(0x1A, string.char(0x03)) --All to S0, 8kb
    w5xxx_write(0x1B, string.char(0x03)) --All to S0, 8kb

    --Read the general register data again
    --data = w5xxx_read(0x00, 0x30)
    --log.info(TAG, "w5xxx_read", data:toHex())
    -- print_CR(data)

    --Set S0 to MACRAW mode
    w5xxx_write(0x0400, string.char(0x04 | 0x40))
    -- sys.wait(100)
    --w5xxx_write(0x0402, string.char(0x01))
    --Enable data sending and receiving
    w5xxx_write(0x0401, string.char(0x01))
    -- sys.wait(100)
    w5xxx_write(0x0402, string.char(0x01))
    sys.wait(50)
end

local function one_time( )
    --Handle receive queue
    local rx_size = read_UINT16(0x0426)
    --log.info(TAG, "length of data to be received", rx_size)
    if rx_size > 0 then
        local data = w5xxx_read_data(2)
        local frame_size = data:byte(1) * 256 + data:byte(2)
        if frame_size < 60 or frame_size > 1600 then
            log.info(TAG, "MAC帧大小异常", frame_size, "强制复位芯片")
            --w5xxx_read_data(frame_size, 1520) -- all discarded
            w5xxx_write(0x0, string.char(0x80))
            return
        else
            --log.info(TAG, "Input MAC frame, size", frame_size - 2)
            local mac_frame = w5xxx_read_data(frame_size, true, true)
            if mac_frame then
                --log.info(TAG, "MAC frame data (including 2-byte header)", mac_frame:toHex())
                --ulwip.input(w5100s.opts.adapter, mac_frame:sub(3))
                ulwip.input(w5100s.opts.adapter, mac_frame, frame_size - 2, 2)
            end
            mac_frame = nil --free memory
        end
    end

    --Handle send queue
    if #w5100s.tx_queue > 0 then
        local send_buff_remain = read_UINT16(0x0420)
        local tmpdata = w5100s.tx_queue[1]
        log.info(TAG, "发送队列", #w5100s.tx_queue, #tmpdata, send_buff_remain)
        if send_buff_remain >= #tmpdata then
            tmpdata = table.remove(w5100s.tx_queue, 1)
            w5xxx_write_data(tmpdata)
            sys.wait(5)
        end
        tmpdata = nil --free memory
    end

    --Is there any data to be received?
    rx_size = read_UINT16(0x0426)
    if rx_size == 0 and #w5100s.tx_queue == 0 then
        sys.wait(20)
    else
        sys.wait(5)
    end
    --sys.wait(100)
end

local function netif_write_out(adapter_index, data)
    --log.info(TAG, "mac data to be sent", data:toHex())
    if w5100s.ready() then
        --TODO Limit transmission queue size
        table.insert(w5100s.tx_queue, data)
    else
        log.info(TAG, "w5100s未就绪,丢弃数据", #data)
    end
end

function w5100s.main_loop()
    local adapter_index = w5100s.opts.adapter
    if not w5100s.ulwip_ready then
        local mac = w5100s.opts.mac or w5100s.mac()
        local ret = ulwip.setup(adapter_index, mac, netif_write_out)
        ulwip.reg(adapter_index)
        ulwip.updown(adapter_index, true)
        ulwip.dhcp(adapter_index, true)
        log.info(TAG, "ulwip初始化完成")
        if w5100s.opts.dft then
            ulwip.dft(adapter_index)
        end
        w5100s.ulwip_ready = true
    end
    ulwip.link(adapter_index, true)
    while w5100s.link() and w5100s.ready(true, true, true) do
        one_time()
    end
end

function w5100s.main_task()
    while 1 do
        if not w5100s.link() then
            log.info(TAG, "网线未连接,等待")
            if w5100s.ulwip_ready then
                ulwip.link(w5100s.opts.adapter, false)
            end
        else
            if not w5100s.ready(true, true, true) then
                log.info(TAG, "w5100s未就绪,执行初始化")
                w5100s.do_init()
            else
                log.info(TAG, "w5100s就绪,执行主逻辑")
                w5100s.tx_queue = {} --Clear the queue
                w5100s.main_loop()
            end
        end
        sys.wait(1000)
    end
end

function w5100s.start()
    if not w5100s.opts then
        log.info(TAG, "未初始化")
        return
    end
    if not w5100s.task_id then
        w5100s.task_id = sys.taskInit(w5100s.main_task)
    end
    return true
end


return w5100s
