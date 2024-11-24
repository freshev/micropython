
--LuaTools requires two pieces of information, PROJECT and VERSION
PROJECT = "ec618_w5500"
VERSION = "1.2"
--PRODUCT_KEY = "s1uUnY6KA06ifIjcutm5oNbG3MZf5aUv" --replace with your own

--sys library is standard
_G.sys = require("sys")
_G.sysplus = require("sysplus")
log.style(1)

--[[
本demo是 Air780E + w5500. 以 Air780E开发板为例, 接线如下:

Air780E            W5500
GND(任意)          GND
GPIO18             IRQ/INT,中断
GPIO01             RST, 复位
GPIO08/SPI0_CS     CS/SCS,片选    
GPIO11/SPI0_SLK    SLK,时钟
GPIO09/SPI0_MOSI   MOSI,主机输出,从机输入
GPIO10/SPI0_MISO   MISO,主机输入,从机输出

最后是供电, 这样要根据W5500的板子来选:
1. 如果是5V的, 那么接780E开发板的5V
2. 如果是3.3V的, 另外找一个3.3V, 例如CH340小板子, 额外供电

注意: 额外供电时候, W5500的GND和Air780E依然需要接起来.
]]


--Configure W5500
--0            -- SPI0
--25600000 -- 25.6M baud rate, the highest baud rate of Air780E
--8 -- CS chip selector
--18 -- INT/IRQ interrupt pin
--1 -- RST reset pin
w5500.init(0, 25600000, 8, 18, 1)
w5500.config()	--The default is DHCP mode. For other Moduless, please refer to the API of the w5500 library.
w5500.bind(socket.ETH0) --Fixed writing method

----------------------------------------
--Error information is automatically reported to the platform, the default is iot.openluat.com
--Supports customization, please refer to the API manual for detailed configuration
--After being turned on, the boot reason will be reported. This requires data consumption, please pay attention.
--if errDump then
--errDump.config(true, 600)
-- end
----------------------------------------


--If the operator's own DNS is not easy to use, you can use the following public DNS
-- socket.setDNS(nil,1,"223.5.5.5")	
-- socket.setDNS(nil,2,"114.114.114.114")

--NTP enabled on demand
-- socket.sntp()
--socket.sntp("ntp.aliyun.com") --Customize sntp server address
--socket.sntp({"ntp.aliyun.com","ntp1.aliyun.com","ntp2.aliyun.com"}) --sntp custom server address
--sys.subscribe("NTP_UPDATE", function()
--log.info("sntp", "time", os.date())
-- end)
--sys.subscribe("NTP_ERROR", function()
--log.info("socket", "sntp error")
--     socket.sntp()
-- end)

--Both 780E and W5500 have IP_READY/IP_LOSE messages, which can be distinguished by adapter.
sys.subscribe("IP_READY", function(ip, adapter)
    log.info("ipready", ip, adapter) 
end)
sys.subscribe("IP_LOSE", function(adapter)
    log.info("iplose", adapter)
end)

-----------------------------------------------------------------------------
--Open TCP on netlab.luatos.com, then modify the IP and port number, automatically reply to the data sent by netlab, and perform self-receiver and self-test
--The following port numbers are temporary ports and must be changed to your own values.
-----------------------------------------------------------------------------

server_ip = "152.70.80.204"
server_port = 55026    --TCP test port
UDP_port = 55026      --Port for UDP testing
ssl_port = 55026      --TCP-SSL test port

--The biggest difference from daily writing is that when creating socket/http/ftp/mqtt, you need to specify the network card socket.ETH0
require "async_socket_demo"
socketDemo()

--User code ended------------------------------------------------
--It always ends with this sentence
sys.run()
--Do not add any statements after sys.run()!!!!!
