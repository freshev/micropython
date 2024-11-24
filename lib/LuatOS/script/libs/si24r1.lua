--[[
@module si24r1
@summary si24r1 驱动
@version 1.0
@date    2022.06.17
@author  Dozingfiretruck
@usage
--Note: Due to the use of sys.wait(), all APIs need to be used in coroutines.
--Usage examples
local si24r1 = require "si24r1"

sys.taskInit(function()
    spi_si24r1 = spi.setup(0,nil,0,0,8,10*1000*1000,spi.MSB,1,1)
    si24r1.init(0,pin.PB04,pin.PB01,pin.PB00)
    if si24r1.chip_check() then
        si24r1.set()
    end

    --Send example
    --si24r1.set_mode( si24r1.MODE_TX ); --Send mode
    --while 1 do
    --     sys.wait(1000)
    --local a = si24r1.txpacket("si24r1test")
    --     print("a",a)
    -- end

    --receive examples
    si24r1.set_mode( si24r1.MODE_RX );		--receive mode
    while 1 do
        local i,data = si24r1.rxpacket( );		--Receive bytes
        print("rxbuf",i,data)
    end
end)
]]


local si24r1 = {}
local sys = require "sys"

local si24r1_device
local si24r1_spi
local si24r1_ce
local si24r1_cs
local si24r1_irq

local si24r1_cspin,si24r1_cepin,si24r1_irqpin

local LM75_ADDRESS_ADR         =   0x48

---The address used by the device
local    REPEAT_CNT          =  15		--Repeat times
local    INIT_ADDR           =  string.char(0x34,0x43,0x10,0x10,0x01)

si24r1.MODE_TX = 0
si24r1.MODE_RX = 1

local    NRF_READ_REG       =   0x00	--Read the configuration register, the lower 5 bits are the register address
local    NRF_WRITE_REG      =   0x20	--Write the configuration register, the lower 5 bits are the register address
local    RD_RX_PLOAD        =   0x61	--Read RX valid data, 1~32 bytes
local    WR_TX_PLOAD        =   0xA0	--Write TX valid data, 1~32 bytes
local    FLUSH_TX           =   0xE1	--Clear TX FIFO register, used in transmit mode
local    FLUSH_RX           =   0xE2	--Clear the RX FIFO register, used in receive mode
local    REUSE_TX_PL        =   0xE3	--Reuse the previous packet of data, CE is high, and data packets are sent continuously.
local    R_RX_PL_WID        =   0x60
local    NOP                =   0xFF	--No operation, can be used to read the status register
local    W_ACK_PLOAD	    =   0xA8
local    WR_TX_PLOAD_NACK   =   0xB0

local    CONFIG             =   0x00	--Configuration register address, bit0: 1 receiving mode, 0 transmitting mode; bit1: electrical selection; bit2: CRC mode; bit3: CRC enable;
                                        --bit4: Interrupt MAX_RT (interrupt when reaching the maximum number of retransmissions) is enabled; bit5: Interrupt TX_DS is enabled; bit6: Interrupt RX_DR is enabled
local    EN_AA              =   0x01	--Enable automatic answer function bit0~5 corresponding to channel 0~5
local    EN_RXADDR          =   0x02	--The receiving address allows bit0~5 to correspond to channel 0~5
local    SETUP_AW           =   0x03	--Set address width (all data channels) bit0~1: 00,3 bytes, 01,4 bytes, 02,5 bytes
local    SETUP_RETR         =   0x04	--Establish automatic retransmission; bit0~3: automatic retransmission counter; bit4~7: automatic retransmission delay 250*x+86us
local    RF_CH              =   0x05	--RF channel, bit0~6 working channel frequency
local    RF_SETUP           =   0x06	--RF register, bit3: transmission rate (0:1M 1:2M); bit1~2: transmit power; bit0: noise amplifier gain
local    STATUS             =   0x07	--Status register; bit0: TX FIFO full flag; bit1~3: receiving data channel number (maximum: 6); bit4: reaching the maximum number of retransmissions
                                        --bit5: data sending completion interrupt; bit6: receiving data interrupt
local    MAX_TX  		    =   0x10	--Interrupt when the maximum number of sending times is reached
local    TX_OK   		    =   0x20	--TX transmission completed interrupt
local    RX_OK   		    =   0x40	--Data received interrupt

local    OBSERVE_TX         =   0x08	--Transmission detection register, bit7~4: packet loss counter; bit3~0: retransmission counter
local    CD                 =   0x09	--Carrier detection register, bit0: carrier detection
local    RX_ADDR_P0         =   0x0A	--Data channel 0 receiving address, maximum length 5 bytes, low byte first
local    RX_ADDR_P1         =   0x0B	--Data channel 1 receiving address, maximum length 5 bytes, low byte first
local    RX_ADDR_P2         =   0x0C	--Data channel 2 receiving address, the lowest byte can be set, the high byte must be equal to RX_ADDR_P1[39:8]
local    RX_ADDR_P3         =   0x0D	--Data channel 3 receiving address, the lowest byte can be set, the high byte must be equal to RX_ADDR_P1[39:8]
local    RX_ADDR_P4         =   0x0E	--Data channel 4 receiving address, the lowest byte can be set, the high byte must be equal to RX_ADDR_P1[39:8]
local    RX_ADDR_P5         =   0x0F	--Data channel 5 receiving address, the lowest byte can be set, the high byte must be equal to RX_ADDR_P1[39:8]
local    TX_ADDR            =   0x10	--Send address (low byte first), in ShockBurstTM mode, RX_ADDR_P0 is equal to the address
local    RX_PW_P0           =   0x11	--Effective data width of receive data channel 0 (1~32 bytes), setting it to 0 is illegal
local    RX_PW_P1           =   0x12	--Effective data width of receive data channel 1 (1~32 bytes), setting it to 0 is illegal
local    RX_PW_P2           =   0x13	--Effective data width of receive data channel 2 (1~32 bytes), setting it to 0 is illegal
local    RX_PW_P3           =   0x14	--Effective data width of receive data channel 3 (1~32 bytes), setting it to 0 is illegal
local    RX_PW_P4           =   0x15	--Effective data width of receive data channel 4 (1~32 bytes), setting it to 0 is illegal
local    RX_PW_P5           =   0x16	--Effective data width of receive data channel 5 (1~32 bytes), setting it to 0 is illegal
local    NRF_FIFO_STATUS    =   0x17	--FIFO status register; bit0: RX FIFO register empty flag; bit1: RX FIFO full flag; bit2~3 reserved
                                        --bit4: TX FIFO empty flag; bit5: TX FIFO full flag; bit6: 1, send the previous data packet cyclically. 0, no loop
local    DYNPD			    =   0x1C
local    FEATRUE			=   0x1D

local    MASK_RX_DR   	    =   6 
local    MASK_TX_DS   	    =   5 
local    MASK_MAX_RT  	    =   4 
local    EN_CRC       	    =   3 
local    CRCO         	    =   2 
local    PWR_UP       	    =   1 
local    PRIM_RX      	    =   0 

local    ENAA_P5      	    =   5 
local    ENAA_P4      	    =   4 
local    ENAA_P3      	    =   3 
local    ENAA_P2      	    =   2 
local    ENAA_P1      	    =   1 
local    ENAA_P0      	    =   0 

local    ERX_P5       	    =   5 
local    ERX_P4       	    =   4 
local    ERX_P3       	    =   3 
local    ERX_P2      	    =   2 
local    ERX_P1       	    =   1 
local    ERX_P0       	    =   0 

local    AW_RERSERVED 	    =   0x0 
local    AW_3BYTES    	    =   0x1
local    AW_4BYTES    	    =   0x2
local    AW_5BYTES    	    =   0x3

local    ARD_250US    	    =   (0x00<<4)
local    ARD_500US    	    =   (0x01<<4)
local    ARD_750US    	    =   (0x02<<4)
local    ARD_1000US   	    =   (0x03<<4)
local    ARD_2000US   	    =   (0x07<<4)
local    ARD_4000US   	    =   (0x0F<<4)
local    ARC_DISABLE   	    =   0x00
local    ARC_15        	    =   0x0F

local    CONT_WAVE     	    =   7 
local    RF_DR_LOW     	    =   5 
local    PLL_LOCK      	    =   4 
local    RF_DR_HIGH    	    =   3 
-- bit2-bit1:
local    PWR_18DB  		    =   (0x00<<1)
local    PWR_12DB  		    =   (0x01<<1)
local    PWR_6DB   		    =   (0x02<<1)
local    PWR_0DB   		    =   (0x03<<1)

local    RX_DR         	    =   6 
local    TX_DS         	    =   5 
local    MAX_RT        	    =   4 
--for bit3-bit1,
local    TX_FULL_0     	    =   0 

local    RPD           	    =   0 

local    TX_REUSE      	    =   6 
local    TX_FULL_1     	    =   5 
local    TX_EMPTY      	    =   4 
--bit3-bit2, reserved, only '00'
local    RX_FULL       	    =   1 
local    RX_EMPTY      	    =   0 

local    DPL_P5        	    =   5 
local    DPL_P4        	    =   4 
local    DPL_P3        	    =   3 
local    DPL_P2        	    =   2 
local    DPL_P1        	    =   1 
local    DPL_P0        	    =   0 

local    EN_DPL        	    =   2 
local    EN_ACK_PAY    	    =   1 
local    EN_DYN_ACK    	    =   0 
local    IRQ_ALL            =   ( (1<<RX_DR) | (1<<TX_DS) | (1<<MAX_RT) )

local check_string = string.char(0X11, 0X22, 0X33, 0X44, 0X55)

local function write_reg(address, value)
    si24r1_cs(0)
    if value then
        spi.send(si24r1_spi,string.char(NRF_WRITE_REG|address).. value)
    else
        spi.send(si24r1_spi,string.char(NRF_WRITE_REG|address))
    end
    si24r1_cs(1)
end

local function read_reg(address,len)
    si24r1_cs(0)
    spi.send(si24r1_spi, string.char(NRF_READ_REG|address))
    local val = spi.recv(si24r1_spi,len or 1)
    si24r1_cs(1)
    return val
end

--[[
si24r1 器件检测
@api si24r1.chip_check()
@return bool   成功返回true
@usage
if si24r1.chip_check() then
    si24r1.set()
end
]]
function si24r1.chip_check()
    write_reg(TX_ADDR, check_string)
    local recv_string = read_reg(TX_ADDR,5)
    if recv_string == check_string then
        return true
    end
    log.info("si24r1","Can't find si24r1 device")
    return false
end

local function read_status_register()
    return read_reg(NRF_READ_REG + STATUS);
end

local function clear_iqr_flag(IRQ_Source)
    local btmp = 0;
    IRQ_Source = IRQ_Source & ( 1 << RX_DR ) | ( 1 << TX_DS ) | ( 1 << MAX_RT );	--Interrupt flag handling
    btmp = read_status_register():byte(1);			--Read status register
    write_reg(NRF_WRITE_REG + STATUS)
    write_reg(IRQ_Source | btmp)
    return ( read_status_register():byte(1))			--Return status register status
end

local function set_txaddr( pAddr )
    write_reg( TX_ADDR, pAddr)	--write address
end

local function set_rxaddr( PipeNum,pAddr )
    write_reg( RX_ADDR_P0 + PipeNum, pAddr)	--write address
end

--[[
si24r1 设置模式
@api si24r1.set_mode( Mode )
@number Mode si24r1.MODE_TX si24r1.MODE_RX
@usage
si24r1.set_mode( si24r1.MODE_TX )
]]
function si24r1.set_mode( Mode )
    local controlreg = 0;
	controlreg = read_reg( CONFIG ):byte(1);
    if ( Mode == si24r1.MODE_TX ) then       
		controlreg =controlreg & ~( 1<< PRIM_RX );
    elseif ( Mode == si24r1.MODE_RX ) then 
		controlreg = controlreg|( 1<< PRIM_RX ); 
	end
    write_reg( CONFIG, string.char(controlreg) );
end

--[[
si24r1 发送
@api si24r1.txpacket(buff)
@string buff 
@return number 0x20:发送成功 0x10:达到最大发送次数中断 0xff:发送失败
@usage
local a = si24r1.txpacket("si24r1test")
]]
function si24r1.txpacket(buff)
    local l_Status = 0
    local l_RxLength = 0
    local l_10MsTimes = 0
    
    spi.send(si24r1_spi,string.char(FLUSH_TX))
    si24r1_ce(0)
    write_reg(WR_TX_PLOAD, buff)
    si24r1_ce(1)

	while( 0 ~= si24r1_irq())do
		sys.wait(10)
        if( 50 == l_10MsTimes )then		
            si24r1.init(si24r1_spi,si24r1_cspin,si24r1_cepin,si24r1_irqpin)
            si24r1.set()
			si24r1.set_mode( si24r1.MODE_TX )
			break;
        end
        l_10MsTimes = l_10MsTimes+1
	end
    l_Status = read_reg( STATUS )		--Read status register
	write_reg( STATUS,l_Status )		--clear interrupt flag

	if( l_Status:byte(1) & MAX_TX )~=0then	--data received
		write_reg( FLUSH_TX,string.char(0xff) )				--Clear TX FIFO
		return MAX_TX
    end

    if( l_Status:byte(1) & TX_OK ~=0 )~=0then	--data received
		return TX_OK
    end
	return 0xFF
end

--[[
si24r1 接收
@api si24r1.rxpacket()
@return number len,buff 长度 数据
@usage
local i,data = si24r1.rxpacket()		--Receive bytes
print("rxbuf",i,data)
]]
function si24r1.rxpacket()
	local l_Status = 0
    local l_RxLength = 0
    local l_100MsTimes = 0

    spi.send(si24r1_spi,string.char(FLUSH_RX))
	
	while( 0 ~= si24r1_irq())do
		sys.wait( 100 )
		if( 30 == l_100MsTimes )then		--No data received for 3 seconds, reinitialize the Modules
            si24r1.init(si24r1_spi,si24r1_cspin,si24r1_cepin,si24r1_irqpin)
            si24r1.set()
			si24r1.set_mode( si24r1.MODE_RX )
			break;
        end
        l_100MsTimes = l_100MsTimes+1
	end
	l_Status = read_reg( STATUS )		--Read status register
	write_reg( STATUS,l_Status )		--clear interrupt flag
	if( l_Status:byte(1) & RX_OK ~=0 )~=0then	--data received
		l_RxLength = read_reg( R_RX_PL_WID )		--Read the number of data received
		local rxbuf = read_reg( RD_RX_PLOAD,l_RxLength:byte(1) )	--data received
		write_reg( FLUSH_RX,string.char(0xff) )				--Clear RX FIFO
		return l_RxLength:byte(1),rxbuf
    end
	return 0;				--No data received
end

--[[
si24r1 配置参数
@api si24r1.set()
@usage
si24r1.set()
]]
function si24r1.set()
    si24r1_ce(1)
    clear_iqr_flag(IRQ_ALL)

    write_reg( DYNPD, string.char( 1 << 0 ) )	--Enable channel 1 dynamic data length
    write_reg( FEATRUE, string.char(0x07) )
    write_reg( DYNPD )
    write_reg( FEATRUE )

    write_reg( CONFIG,string.char(( 1 << EN_CRC ) |   ( 1 << PWR_UP )) )
    write_reg( EN_AA, string.char( 1 << ENAA_P0 ) )   		--Channel 0 auto answer
    write_reg( EN_RXADDR, string.char( 1 << ERX_P0 ) )		--channel 0 receive
    write_reg( SETUP_AW, string.char(AW_5BYTES) )     			--Address width 5 bytes
    write_reg( SETUP_RETR, string.char(ARD_4000US | ( REPEAT_CNT & 0x0F )) )         	--Repeat waiting time 250us
    write_reg( RF_CH, string.char(60) )             			--Initialize channel
    write_reg( RF_SETUP, string.char(0x26) )

    set_txaddr( INIT_ADDR)                      --Set TX address
    set_rxaddr( 0, INIT_ADDR)                   --Set RX address
end

--[[
si24r1 初始化
@api si24r1.init(spi_id,cs,ce,irq)
@number spi_id spi_id
@return bool   成功返回true
@usage
lm75_data.init(0)
]]
function si24r1.init(spi_id,cs,ce,irq)
    --si24r1_device = spi_device
    si24r1_spi = spi_id
    si24r1_cspin = cs
    si24r1_cepin = ce
    si24r1_irqpin = irq

    si24r1_cs = gpio.setup(si24r1_cspin, 0, gpio.PULLUP) 
    si24r1_cs(1)
    si24r1_irq= gpio.setup(si24r1_irqpin, nil,gpio.PULLUP)
    si24r1_ce= gpio.setup(si24r1_cepin, 0)
    si24r1_ce(0)
end

return si24r1


