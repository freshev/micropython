--[[
@module necir
@summary necir NEC协议红外接收
@version 3.5
@date    2023.09.03
@author  lulipro
@usage
--Notice:
--1. This library has passed the test on Air101 and Air103. Air105 is not supported due to the gap between SPI transmission frames.
--2. This library implements NEC infrared data reception. To send, please use the ir.sendNEC() function that comes with the LuatOS underlying firmware.
--3. Since this library is implemented based on the standard four-wire SPI interface, although only the MISO pin is used, the other three SPI related pins are used during use.
--It cannot be used for other purposes. Unless you execute necir.close() and wait for necir.isClosed() to be true, the SPI will be completely released before it can be used for other purposes.
--Hardware Modules: VS1838 and its compatible integrated receiver
--Wiring diagram:
--1. Support single IO mode, that is, only one SPI_MISO pin is used. At this time, the irq_pin parameter of necir.init must be the pin where SPI_MISO is located.
--2. Supports multiple slaves. If other slave devices need to be connected to the SPI bus, in order to avoid VS1838 interfering with the MISO of the bus when infrared communication is not used, you can use
--An NMOS (such as AO3400) controls the power ground of VS1838. As shown below, the GPIO acts as the chip select signal of VS1838. When the GPIO output is low
--Then disable VS1838. At this time, VS1838 does not work. Its OUT pin is high impedance and will not interfere with MISO. When the GPIO output is high, VS1838 is enabled.
--At this time, its OUT pin will output an infrared communication signal to the microcontroller. Since the general SPI slave chip select logic is low enabled, this can be used
--The same chip select GPIO controls VS1838 and another SPI slave because the chip select logic is reversed. Cooperate with necir.close() of necir library
--and necir.isClosed() can maximize the reuse of the SPI interface and avoid the waste of SPI exclusive use.
--____________________                ____________________
--|                    |    单IO |                    |
--|           SPI_MISO |--------------| OUT                |
--| Air10x             |              |       VS1838       |
--| | | Integrated receiving head |
--|               GPIO |----      ----| GND                |
--|____________________|   |      |   |____________________|
--|      |
--|  ____|________
--| |    D        |
----| G      NMOS |
--                            |____S________|
--                                 |
--                                GND
--Usage example: Demonstrates using the same SPI interface to drive VS1838 and W25QXX
--Usage examples:
local necir = require("necir")

--Define user callback function
local function my_ir_cb(frameTab)
    log.info('get ir msg','addr=',frameTab[1],frameTab[2],'data=',frameTab[3])
end

sys.taskInit(function()
    local CS = gpio.setup(pin.PA07,0)  --Chip select pin shared by VS1838 (NMOS controls its GND) and W25QXX
    necir.init(spi.SPI_0,pin.PB03,my_ir_cb)

    while 1 do
        --===============================
        log.info('------necir start------')
        CS(1)     --Enable VS1838
        necir.start()  --Start the necir data receiving process
        sys.wait(10000)
        log.info('necir request to close')
        necir.close()   --Request to close necir
        while not (necir.isClosed()) do
            sys.wait(200)
        end
        CS(0)    --Disable VS1838
        log.info('necir closed')
        sys.wait(1000)

        --===============================
        log.info('------setup to read w25qxx chip id------')
        spi.setup(spi.SPI_0,nil,
            0,--CPHA
            0,--CPOL
            8,--data width
            100000,--frequency
            spi.MSB,--High and low order
            spi.master,--main mode
            spi.full--full duplex
        )
        --Read the W25QXX chi id, 0XEF15, which means the chip model is W25Q32, 0XEF16, which means the chip model is W25Q64
        CS(0)   --Film selection W25QXX
        spi.send(spi.SPI_0,string.char(0x90)..string.char(0x00)..string.char(0x00)..string.char(0x00))
        local chip_id = spi.recv(spi.SPI_0,2)
        log.info('w25qxx id=',chip_id:toHex())
        CS(1)   --Cancel movie selection W25QXX
        sys.wait(1000)
    end
end)
]]

local necir = {}

local sys = require "sys"

local NECIR_IRQ_PIN               --Rising edge interrupt detection pin
local NECIR_SPI_ID                --ID of the SPI interface used
local NECIR_SPI_BAUDRATE       = (14222*4)        --SPI clock frequency, unit HZ
local NECIR_SPI_RECV_BUFF_LEN  = (32+192+192)     --The length of SPI received data

local recvBuff              --SPI receive data buffer
local recvNECFrame={}       --Store in sequence: address code, address code inversion, data code, data code inversion

local recvCallback          --User callback function after successful reception of NEC message
local isNeedTaskFlag        --Receives a flag indicating whether the task needs to be run
local isClosedFlag          --A sign indicating whether necir has been completely closed
--[[
==============实现原理================================================
NEC协议中无论是引导信号，逻辑0还是逻辑1，都由若干个562.5us的周期组成。
例如
  引导信号: 16个562.5us的低电平+8个562.5us的高电平组成  
  逻 辑 1 ：1个562.5us的低电平+3个562.5us的高电平组成 
  逻 辑 0 ：1个562.5us的低电平+1个562.5us的高电平组成

采样定理告诉我们，一切数字信号，都可以通过高倍速采样还原。
我们使用SPI的MISO引脚对红外接收管的输出进行【连续采样】。
我们使用4个SPI字节去采样一个562.5us的红外信号周期，因此SPI的时钟频率设置为 (14222*4) Hz
则
  引导信号: 16*4个0x00 + 8*4个0xff组成
  逻 辑 1 ：1*4个0x00 + 3*4个0xff组成，共16字节
  逻 辑 0 ：1*4个0x00 + 1*4个0xff组成 ，共8字节

NEC的引导信号由一段低电平+一段高电平组成，为了降低采样深度，避免空间占用，我们选择
从后面的高电平产生的上升沿开始进行SPI接收采样 而不是从第一个下降沿就开始。

确定采样深度。NEC协议中，地址码和数据码都是连续传输2次，连续的2个字节是相互取反的关系，
因此这2个字节的总的传输时间是固定，因为前一个字节的某个位是1，则必定后一个字节对应位是0，
则总传输时间就是 = （逻辑1传输时间+逻辑0传输时间）* 8，
则（地址码+ 地址码取反） 和 （数据码+数据码取反）的采样深度都是 (16+8)*8 = 192字节。
引导码高电平部分则是8*4字节。这样我们就确定了SPI的总传输字节数= 32 + 192 + 192=416字节。

理想情况下中断检测到引导码产生的上升沿就会立刻开始SPI数据采集，这样SPI传输 416 字节
能刚好采集引导码的4.5ms高电平加后面的4字节数据，但由于LuatOS中断响应存在一定延迟，导致真正开始
SPI采集会在引导码上升沿开始后的若干个毫秒延迟后才开始，经过测试一般延迟在1ms以内，但具体延迟取决于
LuatOS的行为，只要这个延迟能让SPI采集到完整的引导码后面的4字节数据就都是可以接受的，即延迟不能超过
4.5ms，一般这个情况是能满足的。

本方法需要的SPI频率为56888Hz，而Air101和Air103的SPI频率 = 40MHz/(2*(div_REG+1))，
则div_REG = 350即可实现。如果芯片的SPI无法配置频率也将不支持此方案。

考虑到LuatOS中断响应存在一定延迟，导致SPI接收的信号与实际输出的信号存在一定的
滞后导致字节错位。因此对接收到的SPI字节数据 依次向后 采用16字节长度的窗口切出子串，
对这个子串进行模式匹配，来确定当前片段对应是NEC的逻辑1还是逻辑0。
即：
  在当前位置开始的后面的16个字节形成的子串中，如果存在连续的9个0xff，
  则认为当前位置是逻辑1，否则认为是逻辑0。
  为什什么是9个，考虑下面的情形：
        
逻辑1: 0x00 0x00 0x00 0x00  0xff 0xff 0xff 0xff ... 0xff 
       [     4  个       ]  [         12 个            ]
                           
逻辑0: 0x00 0x00 0x00 0x00  0xff 0xff 0xff 0xff  0x00 0x00 0x00 0x00 0xff 0xff 0xff 0xff
       [     4  个       ]  [       4 个      ]  [    下一个逻辑位（部分）               ]

可以发现，如果是逻辑0，则连续的16个字节窗口中最多出现4个连续的0xff，因此可以作为区分。
lua中的string.find()可以方便的实现字符串模式匹配，因此代码实现很简单。
]]


--Parse NEC messages from received SPI data
local function parseRecvData()
    local fs,fe
    local si

    --Data reinitialization
    recvNECFrame[1] = 0
    recvNECFrame[2] = 0
    recvNECFrame[3] = 0
    recvNECFrame[4] = 0

    --Try to find the first position that is not 0xff, which is used to skip the boot signal byte. This part is up to 32 bytes.
    si=1
    while si <= 32+5 do
        if 0xff ~= string.byte(string.sub(recvBuff,si,si)) then
            break
        end
        si = si + 1
    end
    
    --There is still no address data after 32+5 0xff. This is an abnormal signal. Exit directly.
    if 0xff == string.byte(string.sub(recvBuff,si,si)) then
        return
    end

    --Traverse the subsequent signals of the boot signal, and perform NEC data analysis and restoration on the entire received SPI data.
    while si<=NECIR_SPI_RECV_BUFF_LEN do

        for k = 1, 4, 1 do  --Parse out the 4 bytes of the NEC message
            for i = 0, 7, 1 do  --8 bits per byte
                --In the next 16 bytes starting from the current position, if there are 9 consecutive 0xff values,
                --It is considered that the current position is logical 1
                fs,fe = string.find(
                    string.sub(recvBuff,si,si+16-1),
                    '\xff\xff\xff\xff\xff\xff\xff\xff\xff'
                )
                if fs and fe then --find this pattern
                    --This bit is 1 (NEC protocol big and small endian is LSB First)
                    recvNECFrame[k] = (recvNECFrame[k] | (1<<i))
                    si = si + 16
                else  --No such pattern found
                    --This bit is 0
                    si = si + 8
                end
            end  --8 bits per byte
        end --Parse out the 4 bytes of the NEC message

        break  --Break out of the loop after successfully parsing 4 bytes

    end --Traverse the entire received SPI data

    --Verify the received infrared data and call the user callback function
    --The two address bytes of some remote controls are not inverse of each other, so the address code is not verified here.
    if ((recvNECFrame[3]+recvNECFrame[4]) == 255) then
        --log.info('necir','DataValid,go CallBack')
        if recvCallback then recvCallback(recvNECFrame) end
    end
    --log.info('necir',recvNECFrame[1],recvNECFrame[2],recvNECFrame[3],recvNECFrame[4])
end


--Interrupt function that detects the rising edge generated by the boot
local function irq_func()    
    gpio.close(NECIR_IRQ_PIN)  --Turn off the GPIO function to prevent interrupts from being triggered repeatedly
    spi.setup(NECIR_SPI_ID,nil,0,0,8,NECIR_SPI_BAUDRATE,spi.MSB,spi.master,spi.full)--Reopen the SPI interface
    
    recvBuff =  spi.recv(NECIR_SPI_ID, NECIR_SPI_RECV_BUFF_LEN) --Receive the demodulated data output by the infrared receiver through SPI
    sys.publish('NECIR_SPI_DONE')  --Publish a message and let the task analyze and process the received SPI data
end


local function recvTaskFunc()

    while true do
        sys.waitUntil('NECIR_START',5000)

        while isNeedTaskFlag do
            spi.close(NECIR_SPI_ID)  --Close the SPI interface so that MISO can be freed for interrupt detection.
            gpio.setup(NECIR_IRQ_PIN,irq_func,gpio.PULLUP ,gpio.RISING)--Turn on GPIO interrupt detection function
    
            local result, _ = sys.waitUntil('NECIR_SPI_DONE',1000)
            if result then  --SPI completes acquisition and starts parsing data
                parseRecvData()
            end
        end
        --Do cleanup work when closing the receiving process
        if not isClosedFlag then
            gpio.close(NECIR_IRQ_PIN)  --Turn off GPIO function
            spi.close(NECIR_SPI_ID)   --Close SPI interface
            isClosedFlag = true
            --log.info('necir','recv task closed')
        end
    end --task main loop
end

--[[
necir初始化
@api necir.init(spi_id,irq_pin,recv_cb)
@number spi_id,使用的SPI接口的ID
@number irq_pin,使用的中断引脚，在单IO模式下这个引脚必须是SPI的MISO引脚
@function recv_cb,红外数据接收完成后的回调函数，回调函数有1个table类型参数，分别存储了地址码，地址码取反，数据码，数据码取反
@usage
local function my_ir_cb(frameTab)
    log.info('get ir msg','addr=',frameTab[1],frameTab[2],'data=',frameTab[3])
end

necir.init(spi.SPI_0,pin.PB03,my_ir_cb)
]]
function necir.init(spi_id,irq_pin,recv_cb)
    NECIR_SPI_ID     = spi_id
    NECIR_IRQ_PIN    = irq_pin
    recvCallback     = recv_cb

    isNeedTaskFlag = false      --Receives a flag indicating whether the task needs to be run
    isClosedFlag   = true       --A sign indicating whether necir has been completely closed
    --Start the infrared data receiving task
    sys.taskInit(recvTaskFunc)
end

--[[
开启necir数据接收过程
@api necir.start()
@usage
necir.start()
]]
function necir.start()
    isNeedTaskFlag     = true
    isClosedFlag       = false
    sys.publish('NECIR_START')
end

--[[
请求关闭necir数据接收过程。此函数执行后并不能保证立刻关闭，具体是否已经关闭需要使用necir.isClosed()来查询。
@api necir.close()
@usage
necir.close()
]]
function necir.close()
    isNeedTaskFlag = false
end

--[[
判断necir是否已经完全关闭，关闭后所使用的SPI接口将释放，可以复用为其他功能。如需再次开启，则需要再次调用necir.start()
@api necir.isClosed()
@return bool   关闭成功返回true
@usage
necir.isClosed()
]]
function necir.isClosed()
    return isClosedFlag
end

return necir
